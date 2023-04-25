#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_tree;
template<typename _Key, typename _Val, typename traits> class aex_node_allocator;

template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_data_node;

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_base{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef aex_node_base<key_type, value_type, traits> self;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;


    typedef self node;

    typedef self* node_ptr;

    // size: the child node of the node(inner node); the data of the node(data node)
    // slot_size: the slot of the node
    size_type size, slot_size;

    // prop
    // level: node height
    unsigned int prop, level;

    node_ptr prev, next;

    //virtual size_type& data_size() = 0;
    //
    //virtual size_type data_node_size() = 0;
//
    //virtual key_type max_key() = 0;

    struct balance_stats{
        size_type recent_update_timestamp;
        double write_times, train_times, read_times;
    }base_stats;

    inline size_type& data_size(){
        return (this->prop & node_property::LEAF) ? this->size : static_cast<inner_node_ptr>(this)->size;
    }

    inline size_type data_node_size(){
        return (this->prop & node_property::LEAF) ? 1 : static_cast<inner_node_ptr>(this)->m_stats.data_node;
    }

    inline key_type node_max_key(){
        return (this->prop & node_property::LEAF) ? static_cast<data_node_ptr>(this)->key[this->size - 1] : static_cast<inner_node_ptr>(this)->key_ptr[static_cast<inner_node_ptr>(this)->last()];
    }

};


/*
    memory layout:
    meta(const size): size, prop, level, Model, slot size
    key array(variable size)
    pointer array(variable size)
    bitmap array(variable size)
    - spinlock array (if multithread)
    - version array (if multithread)
*/

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node : public aex_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef aex_tree<_Key, _Val, traits> Tree;

    typedef aex_node_allocator<_Key, _Val, traits> node_allocator;

    typedef aex_inner_node<_Key, _Val, traits> self;

    typedef aex_node_base<key_type, value_type, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    //typedef linear_model<key_type> Model;
    typedef aex_model<key_type> Model;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;
    
    typedef inner_node* inner_node_ptr;

    aex_inner_node(){}

    ~aex_inner_node(){}

    inline void free(){}

    // clear bitmap
    inline void clear_bitmap(){
        memset(this->bitmap_ptr, 0, node_allocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear_key_array(){
        memset(this->key_ptr, 0, node_allocator::KEY_MEMORY_USED(this->slot_size));
    }

    // Construct a node with key array, don't check model is fit. 
    // Please check_rewired(key, n) first!!!
    inline void construct(const key_type* const key, const node_ptr* const child, const unsigned int n){
        if (Tree::check_rewired(key, n, this->slot_size, this->model)){
            this->prop |= node_property::ML_NODE;
        }
        else{
            this->prop &= ~node_property::ML_NODE;
        }
        this->construct(key, child, n, this->model);
    }

    // construct a node with key array and model
    inline void construct(const key_type* const key, const node_ptr* const child, const size_type n, const Model &m){
        AEX_ASSERT(n > 0);
        size_type start=0, pos;
        bitmap bm = this->bitmap_ptr;
        key_type* node_key = this->key_ptr;
        node_ptr* node_child = this->child_ptr;
        this->clear_bitmap();
        this->clear_key_array();
        this->model = m;
        // meta
        {
            this->m_stats.data_node = this->m_stats.data_size = this->base_stats.write_times = 0;
            this->size = n;
            for (size_type i = 0; i < n; ++i){
                this->m_stats.data_size += child[i]->data_size();

            }
            if (this->level == 1){
                AEX_ASSERT((child[0]->prop & LEAF) != 0);
                this->m_stats.data_node = n;
            }
            else{
                for (size_type i = 0; i < n; ++i)
                    this->m_stats.data_node += static_cast<inner_node_ptr>(child[i])->m_stats.data_node;
            }
            this->m_stats.rewired_cnt = static_cast<size_type>(traits::INIT_REWIRED_CNT * log(this->slot_size));
        }
        if (this->prop & node_property::ML_NODE){
            for (size_type i = 0; i < n; ++i){
                pos = this->predict(key[i]);
                AEX_ASSERT(start - pos >= traits::ERROR_BOUND);
                while (start < pos){
                    node_key[start] = key[i];
                    node_child[start] = child[i];
                    ++start;
                }
                node_key[start] = key[i];
                node_child[start] = child[i];
                bitmap_impl::set_one(bm, start);
                ++start;
                this->slot_bound = start;
            }
        }
        else{
            memcpy(node_key, key, n * sizeof(key_type));
            memcpy(node_child, child, n * sizeof(node_ptr));
            this->slot_bound = n;
        }
    }

    // copy a node
    inline void copy(const self* const node){
        AEX_ASSERT(node->slot_size != this->slot_size);
        memcpy(this, node, sizeof(self));
        memcpy(this->key_ptr, node->key_ptr, node_allocator::KEY_MEMORY_USED(this->slot_size));
        memcpy(this->child_ptr, node->child_ptr, node_allocator::PTR_MEMORY_USED(this->slot_size));
        memcpy(this->bitmap_ptr, node->bitmap_ptr, node_allocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    // return the slot of child node
    inline size_type at(const node_ptr node) const {
        bitmap bm = this->bitmap_ptr;
        node_ptr* child = this->child_ptr;
        if (node == nullptr) return this->slot_size;
        if (this->prop & node_property::ML_NODE){
            key_type key = (node->prop & node_property::LEAF) ? (static_cast<data_node_ptr>(node)->key[node->size - 1]) : (static_cast<inner_node_ptr>(node)->key_ptr[static_cast<inner_node_ptr>(node)->last()]);
            size_type pos = std::min(this->predict(key), this->slot_bound - 1);
            size_type upper = std::min(pos + traits::ERROR_BOUND + 1, this->slot_bound);
            for (size_type i = pos; i < upper; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node) 
                return i;
        }
        else{
            if (this->size > traits::BINEARY_SEARCH_SIZE){
                //key_type node_key = (node->prop & LEAF) ? (static_cast<data_node_ptr>(node)->max_key()) : (static_cast<inner_node_ptr>(node)->max_key());
                key_type node_key = node->node_max_key();
                size_type pos = std::lower_bound(this->key_ptr, this->key_ptr + this->size, node_key) - this->key_ptr;
                return (pos == this->size) ? this->slot_size : pos;
            }
            else{
                for (size_type i = 0; i < this->size; ++i){
                    if (child[i] == node)
                        return i;
                }
            }
        }
        return this->slot_size;
    }

    // return the first item position.
    inline size_type first() const {
        return 0;
    }

    // return the last item position.
    inline size_type last() const {
        return this->slot_bound - 1;
    }
    
    // return the prev item position. If none, return slot_size
    inline size_type prev_item(size_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (pos == 0) return this->slot_size;
        /* TODO: use __buitlin_clzll */
        if (this->prop & node_property::ML_NODE){
            for (size_type i = pos; i != 0; --i)
            if (bitmap_impl::at(bm, i - 1))
                return i - 1;
            return this->slot_size;
        }
        else 
            return (pos == 0) ? this->slot_size: (pos - 1);
    }

    // return the next item position. If none, return slot_size
    inline size_type next_item(size_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (this->prop & node_property::ML_NODE){
            for (size_type i = pos + 1; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i))
                return i;
            return this->slot_size;
        }
        else 
            return (pos >= this->size) ? this->slot_size : pos + 1;
    }

    // real_slot_size() mean slot size minus error bound
    inline size_type real_slot_size() const{
        return (this->prop & node_property::ML_NODE) ? (this->slot_size - traits::ERROR_BOUND) : this->slot_size;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline size_type predict(const key_type& key) const {
        return std::max((size_type)0, std::min((size_type)(model.predict(key) * this->real_slot_size()), this->slot_size - 1));
        //return model.predict(key);
    }

    // if no item greater than or equal x, return node->slot_size
    inline size_type find_lower_pos(const key_type& x) const{
        size_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            size_type pred_pos = this->predict(x);
            size_type upper_bound = std::min(pred_pos + traits::ERROR_BOUND + 1, this->slot_bound);
            for (size_type i = pos; i < upper_bound; ++i)
            if (key_ptr[i] >= x){
                pos = i;
                break;
            }
        }
        else{
            if (this->slot_size < traits::BINEARY_SEARCH_SIZE){
                for (size_type i = 0; i < this->slot_bound; ++i)
                if (key_ptr[i] >= x){
                    pos = i;
                    break;
                }
            }
            else{
                pos = std::lower_bound(key_ptr, key_ptr + this->size, x) - key_ptr;
            }
        }
        return pos;
    }

    // if no item greater than x, return slot_size
    inline size_type find_upper_pos(const key_type& x) const{
        size_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            pos = this->predict(x);
            size_type upper_bound = std::min(pos + traits::ERROR_BOUND + 1, this->slot_bound);
            for (size_type i = pos; i < upper_bound; ++i)
            if (key_ptr[i] > x){
                pos = i;
                break;
            }
        }
        else{
            if (this->slot_size < traits::BINEARY_SEARCH_SIZE){
                for (size_type i = 0; i < this->slot_bound; ++i)
                if (key_ptr[i] > x){
                    pos = i;
                    break;
                }
            }
            else
                pos = std::upper_bound(key_ptr, key_ptr + this->size, x) - key_ptr;
        }
        return pos;
    }

    inline size_type& data_size(){return this->m_stats.data_size;}

    inline size_type data_node_size(){return this->m_stats.data_node;}

    key_type node_max_key(){return key_ptr[this->last()];}


public:

    // meta:
    struct stats{
        size_type data_node, data_size, rewired_cnt;
    }m_stats;

    // slot_bound: the max used slot position
    size_type slot_bound;

    Model model;

    key_type* __restrict__ key_ptr;

    node_ptr* __restrict__ child_ptr;
    
    bitmap __restrict__ bitmap_ptr;
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_data_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef aex_data_node<_Key, _Val, traits> self;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;
    
    key_type __restrict__ *key;

    value_type __restrict__ *data;

    //union Model
    //{
    //    /* model */
    //    two_layer_model<key_type, linear_model, traits>* complex_model;
    //    linear_model<key_type> easy_model;
    //}model;
    //typedef linear_model<key_type> Model;
    typedef aex_model<key_type> Model;

    Model model;

    aex_data_node(){
        //model.complex_model = nullptr;
    }
    ~aex_data_node(){
        //if ((this->prop & COMPLEX_MODEL) & this->model.complex_model == nullptr){
        //    delete this->model.complex_model;
        //}
    }

    inline void free(){
        //if ((this->prop & COMPLEX_MODEL) & this->model.complex_model == nullptr){
        //    delete this->model.complex_model;
        //}
    }

    //inline size_type predict(key_type &k){
    //    return std::max(std::min(model.predict() * this->size, this->size - 1), 0);
    //}

    void construct(const std::pair<key_type, value_type>* _data, size_type nums){
        assert(this->slot_size >= nums);
        for (size_type j = 0; j < nums; ++j){
            key[j] = _data[j].first;
            data[j] = _data[j].second;
        }
        this->size = nums;
        this->base_stats.write_times = this->base_stats.train_times = this->base_stats.read_times = 0;
        this->train_model();
    }

    void construct(const key_type *_key, const value_type *_data, size_type nums){
        memcpy(key, _key, nums * sizeof(key_type));
        memcpy(data, _data, nums * sizeof(value_type));
        this->size = nums;
        this->base_stats.write_times = this->base_stats.train_times = this->base_stats.read_times = 0;
        this->train_model();
    }

    void construct(const key_type *_key, const value_type *_data, size_type nums, Model &m){
        memcpy(key, _key, nums * sizeof(key_type));
        memcpy(data, _data, nums * sizeof(value_type));
        this->size = nums;
        this->base_stats.write_times = this->base_stats.train_times = this->base_stats.read_times = 0;
        this->model = m;
    }

    // if no item greater than or equal x, return slot_size
    inline size_type find_lower_pos(const key_type &x){
        size_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            //if (this->prop & COMPLEX_MODEL){
            //    pos = this->model.complex_model.predict(x);
            //    pos = exponential_search_lower_bound(this->key, this->key + this->size, pos, x) - this->key;
            //}
            //else{
                pos = this->model.predict(x);
                pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pos, x) - this->key;
            //}
        }
        else{
            if (this->size < traits::BINEARY_SEARCH_SIZE){
                pos = this->size;
                for (size_type i = 0; i < this->size; ++i)
                    if (this->key[i] >= x){
                        pos = i;
                        break;
                    }
            }
            else
                pos = std::lower_bound(this->key, this->key + this->size, x) - this->key;
        }
        return pos;
    }

    // if no item greater than x, return slot_size.
    inline size_type find_upper_pos(const key_type &x){
        size_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            //if (this->prop & COMPLEX_MODEL){
            //    pos = this->model.complex_model.predict(x);
            //    pos = exponential_search_upper_bound(this->key, this->key + this->size, pos, x) - this->key;
            //}
            //else{
                pos = this->model.predict(x);
                pos = aex::exponential_search_upper_bound(this->key, this->key + this->size, pos, x) - this->key;
            //}
        }
        else{
            if (this->size < traits::BINEARY_SEARCH_SIZE){
                pos = this->size;
                for (size_type i = 0; i < this->size; ++i)
                    if (this->key[i] > x){
                        pos = i;
                        break;
                    }
            }
            else{
                pos = std::upper_bound(this->key, this->key + this->size, x) - this->key;
            }
        }
        return pos;
    }

    inline void insert(size_type pos){
        //if (this->prop & COMPLEX_MODEL)
        //    this->model.complex_model.insert(pos);
    }

    inline void erase(size_type pos){
        //if (this->prop & COMPLEX_MODEL)
        //    this->model.complex_model.erase(pos);
    }

    // if the data node can be trained, return true. Else return false.
    inline bool train_model(){
        if (this->prop & node_property::ML_NODE){
            model.train(this->key, this->size);
            if (this->RMSE() * this->slot_size > traits::MAX_ALLOW_ERROR * log(this->size)){
                this->prop ^= node_property::ML_NODE;
                return false;
            }
            return true;
        }
        return false;
    }

    inline double RMSE(){
        if (this->prop & node_property::ML_NODE){
            //if (this->prop & COMPLEX_MODEL)
            //    return this->model.complex_model.RMSE(this->key, this->size);
            //else
            return this->model.RMSE(this->key, this->size);
        }
        else return 0;
    }

    inline size_type& data_size(){return this->size;}
    
    inline size_type data_node_size(){return 1;}

    key_type node_max_key(){return key[this->size - 1];}

    
};

}