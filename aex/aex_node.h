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

    node_ptr prev, next;

    // size: the child node of the node(inner node); the data of the node(data node)
    // slot_size: the slot of the node
    size_type size, slot_size;

    // prop
    // level: node height
    unsigned int prop;
    int level;

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

    typedef aex_node_base<key_type, value_type, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    //typedef linear_model<key_type> Model;
    //typedef aex_model<key_type, traits> Model;

    typedef gap_array_linear_model<key_type, traits> Model;

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
        //AEX_PRINT("inner node construct");
        if (Tree::check_rewired(key, n, this->real_slot_size(), this->model)){
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
        size_type start = 0, pos;
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
            key_type now_key = key[0];
            node_ptr now_child = child[0];
            for (size_type i = 0; i < n; ++i){
                pos = this->predict(key[i]);
                AEX_ASSERT(start - pos < traits::ERROR_BOUND);
                while (start < pos){
                    node_key[start] = now_key;
                    node_child[start] = now_child;
                    ++start;
                }
                this->key_ptr[start] = now_key = key[i];
                this->child_ptr[start] = now_child = child[i];
                bitmap_impl::set_one(bm, start);
                ++start;
            }
            while (start < this->slot_size){
                node_key[start] = now_key;
                node_child[start] = now_child;
                ++start;
            }
        }
        else{
            memcpy(node_key, key, n * sizeof(key_type));
            memcpy(node_child, child, n * sizeof(node_ptr));
        }
    }

    // insert a node
    bool insert(const key_type &key, const node_ptr child){
        if (!(this->prop & node_property::ML_NODE)) {
            size_type pos = this->find_lower_pos(key);
            memmove(this->key_ptr + pos + 1, this->key_ptr + pos, (this->size - pos) * sizeof(key_type));
            memmove(this->child_ptr + pos + 1, this->child_ptr + pos, (this->size - pos) * sizeof(node_ptr));
            ++this->size;
            this->m_stats.data_size += child->data_size();
            this->m_stats.data_node += child->data_node_size();
            return true;
        }
        else{
            size_type pred_pos = this->predict(key);
            size_type inserted_pos = pred_pos;
            for (; inserted_pos < this->slot_size && inserted_pos - pred_pos < traits::ERROR_BOUND; ++inserted_pos)
            if (this->key_ptr[inserted_pos] > key || !bitmap_impl::at(this->bitmap_ptr, inserted_pos)){
                break;
            }
            if (inserted_pos >= this->slot_size)
                return false;
            // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
            if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
                return false;

            #ifdef AEX_DEBUG
            //if (Tree::debug_level >= 1){
            //    for (size_type i = 0; i < this->slot_size; ++i){
            //        AEX_PRINT("pos=" << i << " key=" << this->key_ptr[i] << " child=" << this->child_ptr[i]);
            //    }
            //    AEX_PRINT("key=" << key << " pos=" << inserted_pos << " predict=" << this->predict(key));
            //}
            #endif
            // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
            size_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size);
            for (size_type i = inserted_pos; i < max_slot; ++i){
                if (bitmap_impl::at(this->bitmap_ptr, i)){
                    size_type shift_pos = this->predict(this->key_ptr[i]);
                    if (i + 1 - shift_pos >= traits::ERROR_BOUND){
                        //AEX_PRINT("i=" << i << ", ori_pos=" << shift_pos);
                        return false;
                    }
                }
                else{
                    memmove(this->key_ptr + inserted_pos + 1, this->key_ptr + inserted_pos, (i - inserted_pos) * sizeof(key_type));
                    memmove(this->child_ptr + inserted_pos + 1, this->child_ptr + inserted_pos, (i - inserted_pos) * sizeof(node_ptr));
                    bitmap_impl::set_one(this->bitmap_ptr, i);
                    size_type next_pos = this->next_item(inserted_pos);
                    for (size_type i = inserted_pos; i < next_pos; ++i){
                        this->key_ptr[i] = key;
                        this->child_ptr[i] = child;
                    }
                    ++this->size; 
                    this->m_stats.data_size += child->data_size();
                    this->m_stats.data_node += child->data_node_size();
                    return true;
                }
            }
            // if need shift move more than ERROR_BOUND item, return false
            //AEX_PRINT("inserted_pos=" << inserted_pos << ", pred_pos=" << pred_pos << ", max_slot=" << max_slot);
            //for (size_type i = pred_pos; i < max_slot; ++i){
            //    AEX_PRINT("key_ptr[" << i << "]=" << key_ptr[i] << ", b" << (bitmap_impl::at(this->bitmap_ptr, i) > 0));
            //}
            return false;
        }
    }

    // erase a node
    bool erase(node_ptr node){
        size_type pos = this->at(node);
        if (pos == this->slot_size)
            return false;
        key_type* key = this->key_ptr;
        node_ptr* child = this->child_ptr;
        --this->size;
        if (this->prop & node_property::ML_NODE){
            bitmap_impl::set_zero(this->bitmap_ptr, pos);
            size_type next_pos = this->next_item(pos);
            if (pos > 0){
                for (size_type i = pos; i < next_pos; ++i){
                    key[i] = key[pos - 1];
                    child[i] = child[pos - 1];
                }
            }
            else{
                AEX_ASSERT(0);
            }
        }
        else{
            memmove(key + pos, key + pos + 1, (this->size - pos - 1) * sizeof(key_type));
            memmove(child + pos, child + pos + 1, (this->size - pos - 1) * sizeof(node_ptr));
        }
        return true;
    }

    // copy a node
    inline void copy(const inner_node_ptr node){
        AEX_ASSERT(node->slot_size != this->slot_size);
        memcpy(this, node, sizeof(inner_node));
        memcpy(this->key_ptr, node->key_ptr, node_allocator::KEY_MEMORY_USED(this->slot_size));
        memcpy(this->child_ptr, node->child_ptr, node_allocator::PTR_MEMORY_USED(this->slot_size));
        memcpy(this->bitmap_ptr, node->bitmap_ptr, node_allocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    // return the slot of child node
    inline size_type at(const node_ptr node) const {
        bitmap bm = this->bitmap_ptr;
        node_ptr* child = this->child_ptr;
        key_type node_key = (node->prop & node_property::LEAF) ? (static_cast<data_node_ptr>(node)->key[0]) : (static_cast<inner_node_ptr>(node)->key_ptr[0]); 
        if (node == nullptr) return this->slot_size;
        if (this->prop & node_property::ML_NODE){
            size_type pos = this->predict(node_key);
            size_type upper = std::min(pos + traits::ERROR_BOUND, this->slot_size);
            for (size_type i = pos; i < upper; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node) 
                return i;
        }
        else{
            if (this->size > traits::BINEARY_SEARCH_SIZE){
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
        if (this->prop & ML_NODE){
            for (size_type i = this->slot_size - 1; i >= 0; --i)
            if (bitmap_impl::at(this->bitmap_ptr, i))
                return i;
            return this->slot_size;
        }
        else 
            return this->size - 1;
    }
    
    // return the prev item position. If none, return slot_size
    inline size_type prev_item(size_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (pos == 0) return this->slot_size;
        /* TODO: use __buitlin_clzll */
        if (this->prop & node_property::ML_NODE){
            for (size_type i = pos - 1; i >= 0; --i)
            if (bitmap_impl::at(bm, i))
                return i;
            return -1;
        }
        else 
            return pos - 1;
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
        return std::max((size_type)0, std::min(static_cast<size_type>(round(model.predict(key) * this->real_slot_size())), this->slot_size - 1));
    }

    // (X) if no item greater than or equal x, return node->slot_size
    // find key pos in which slot. If not, return node->slot_size
    inline size_type find_lower_pos(const key_type& x) const{
        size_type pos = -1;
        if (this->prop & node_property::ML_NODE){
            size_type pred_pos = this->predict(x);
            size_type upper_bound = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size);
            for (size_type i = pred_pos; i < upper_bound; ++i)
            if (key_ptr[i] > x){
                return i - 1;
            }
            AEX_ASSERT(next_item(upper_bound - 1) == this->slot_size);
            //if (key_ptr[next_item(pos)] <= x){
            //    AEX_PRINT("next key=" << key[next_item(pos)])
            //}
            AEX_ASSERT(key_ptr[next_item(upper_bound - 1)] > x);
            if (pos == -1) 
                pos = upper_bound - 1;
            return pos;
        }
        else{
            if (this->slot_size < traits::BINEARY_SEARCH_SIZE){
                for (size_type i = 0; i < this->size; ++i)
                if (key_ptr[i] > x){
                    return i - 1;
                }
                return this->size - 1;
            }
            else{
                pos = std::upper_bound(key_ptr, key_ptr + this->size, x) - key_ptr;
                if (pos == -1) 
                    pos = this->size - 1;
                return pos;
            }
        }
    }

    

    inline size_type& data_size(){return this->m_stats.data_size;}

    inline size_type data_node_size(){return this->m_stats.data_node;}


public:

    // meta:
    struct stats{
        size_type data_node, data_size, rewired_cnt;
    }m_stats;

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
    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef linear_model<_Key, traits> Model;

    Model model;
    
    key_type __restrict__ *key;

    value_type __restrict__ *data;

    //typedef linear_model<key_type, traits> Model;

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

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline size_type predict(const key_type& key) const {
        
        return std::max((size_type)0, std::min(static_cast<size_type>(model.predict(key) * this->size), this->size - 1));
    }

    void construct(const std::pair<key_type, value_type>* _data, size_type nums){
        AEX_ASSERT(this->slot_size >= nums);
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

    // insert a item
    inline size_type insert(const key_type &x, const value_type &data){
        size_type pos = this->find_lower_pos(x);
        memmove(this->key + pos + 1, this->key + pos, (this->size - pos) * sizeof(key_type));
        memmove(this->data + pos + 1, this->data + pos, (this->size - pos) * sizeof(value_type));
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
        return pos;
    }

    // if no item greater than or equal x, return slot_size
    inline size_type find_lower_pos(const key_type &x){
        size_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            size_type pred_pos = this->predict(x);
            pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
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

    // if the data node can be trained, return true. Else return false.
    inline bool train_model(){
        if (this->prop & node_property::ML_NODE){
            model.train(this->key, this->size);
            //AEX_PRINT(traits::MAX_ALLOW_ERROR * log(this->size) << " " << this->RMSE());
            if (this->RMSE() > traits::MAX_ALLOW_ERROR * log(this->size)){
                this->prop ^= node_property::ML_NODE;
                return false;
            }
            return true;
        }
        return false;
    }

    inline double RMSE(){
        if (this->prop & node_property::ML_NODE){
            return this->model.RMSE(this->key, this->size);
        }
        else return 0;
    }

    inline size_type& data_size(){return this->size;}
    
    inline size_type data_node_size(){return 1;}

};

}