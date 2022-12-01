#pragma once
namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_base{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    //typedef typename traits::pos_type pos_type;

    typedef aex_node_base<key_type, value_type, traits> self;

    typedef self node;

    typedef self* node_ptr;

    size_type size, slot_size;
    
    long long read_write_diff;

    unsigned int prop, level;

    node_ptr prev, next, parent;
};

template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_data_node;

/*
    memory layout:
    meta(const size): size, prop, level, Model, slot size
    key array(variable size)
    pointer array(variable size)
    bitmap array(variable size)
*/

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node : public aex_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef aex_inner_node<_Key, _Val, traits> self;

    typedef aex_node_base<key_type, value_type, traits> node;
    
    typedef node* node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    typedef linear_model<_Key> Model;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;
    
    typedef inner_node* inner_node_ptr;
    
    // clear bitmap
    inline void clear_bitmap(){
        memset(bitmap_ptr, 0, BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear_key_array(){
        memset(key_ptr, 0, KEY_MEMORY_USED(this->slot_size));
    }

    // construct a node with key array, don't check model is fit, check_rewired first
    void construct(const key_type* const key, const node_ptr* const child, const unsigned int n){
        size_type start = 0, pos;
        bitmap bm = this->bitmap_ptr;
        key_type* node_key = this->key_ptr;
        node_ptr* node_child = this->child_ptr;
        this->clear_bitmap();
        this->clear_key_array();
        this->size = n;
        if (this->prop & ML_NODE){
            /* train the model, and insert data */
            this->model.train(key, n, this->real_slot_size());
            for (size_type i = 0; i < n; ++i){
                pos = this->predict(key[i]);
                while (start < pos){
                    node_key[start] = key[i];
                    node_child[start] = child[i];
                    ++start;
                }
                node_key[start] = key[i];
                node_child[start] = child[i];
                bitmap_impl::set_one(bm, start);
                ++start;
            }
        }
        else{
            /* only copy data */
            memcpy(node_key, key, n * sizeof(key_type));
            memcpy(node_child, child, n * sizeof(node_ptr));
        }
    }


    // construct a node with key array and model
    void construct(const key_type* const key, const node_ptr* const child, const size_type n, const Model &m){
        size_type start=0, pos;
        bitmap bm = this->bitmap_ptr;
        key_type* node_key = this->key_ptr;
        node_ptr* node_child = this->child_ptr;
        this->clear_bitmap();
        this->clear_key_array();
        this->size = n;
        if (this->prop & ML_NODE){
            for (size_type i = 0; i < n; ++i){
                this->model = m;
                pos = this->predict(key[i]);
                while (start < pos){
                    node_key[start] = key[i];
                    node_child[start] = child[i];
                    ++start;
                }
                node_key[start] = key[i];
                node_child[start] = child[i];
                bitmap_impl::set_one(bm, start);
                ++start;
            }
        }
        else{
            memcpy(node_key, key, n * sizeof(key_type));
            memcpy(node_child, child, n * sizeof(node_ptr));
        }
    }

    // copy a node
    void copy(const self* const node){
        memcpy(this, node, self::memory_used);
        memcpy(this->key_ptr, node->key_ptr, KEY_MEMORY_USED(this->slot_size));
        memcpy(this->child_ptr, node->child_ptr, PTR_MEMORY_USED(this->slot_size));
        memcpy(this->bitmap_ptr, node->bitmap_ptr, BITMAP_MEMORY_USED(node->slot_size));
    }

    // return the slot of child node
    inline size_type at(const node_ptr node) const {
        bitmap bm = this->bitmap_ptr;
        node_ptr* child = this->child_ptr;
        if (node == nullptr) return this->slot_size;
        if (this->prop & ML_NODE){
            /* TODO: predict */
            /*
            for (size_type i = 0; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node)
                return i;
            */
            key_type key = (node->prop & LEAF) ? (static_cast<data_node_ptr>(node)->key[node->size - 1]) : (static_cast<inner_node_ptr>(node)->key_ptr[static_cast<inner_node_ptr>(node)->last()]);
            size_type pos = std::min(this->predict(key), this->slot_size - 1);
            size_type upper = std::min(pos + traits::ERROR_BOUND + 1, this->slot_size);
            for (size_type i = pos; i < upper; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node) 
                return i;
            AEX_ASSERT(false);

        }
        else{
            for (size_type i = 0; i < this->size; ++i){
                if (child[i] == node)
                    return i;
            }
        }
        return this->slot_size;
    }

    // return the first item position.
    inline size_type first() const {
        if (this->prop & ML_NODE){
            bitmap bm = bitmap_ptr;
            for (size_type i = 0; i < this->slot_size; ++i)
                if (bitmap_impl::at(bm, i)) 
                    return i;
        }
        else{
            return 0;
        }
    }

    // return the last item position.
    inline size_type last() const {
        if (this->prop & ML_NODE){
            bitmap bm = bitmap_ptr;
            size_type i = this->slot_size;
            do {
                --i;
                if (bitmap_impl::at(bm, i))
                    return i;
            }while (i > 0);
        }
        else{
            return this->size - 1;
        }

        AEX_ASSERT(1 == 2);
        return 0;
    }
    
    // return the prev item position. If none, return slot_size
    inline size_type prev(size_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (pos == 0) return this->slot_size;
        if (this->prop & ML_NODE){
            //for (size_type i = pos - 1; i >= 0; --i)
            //if (bitmap_impl::at(bm, i))
            //    return i;
            size_type i = pos;
            do{
                --i;
                if (bitmap_impl::at(bm, i))
                    return i;
            }while (i > 0);
            return this->slot_size;
        }
        else 
            return pos - 1;
    }

    // return the next item position. If none, return slot_size
    inline size_type next(size_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (this->prop & ML_NODE){
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
        return (this->prop & ML_NODE) ? (this->slot_size - traits::ERROR_BOUND) : this->slot_size;
    }

    // only ML_NODE can use it. check if ML_NODE first
    // position range [0, slot_size)
    inline size_type predict(const key_type& key) const {
        return std::max(0, std::min(model.predict(key) * this->slot_size, this->slot_size - 1));
        //return model.predict(key);
    }

public:
    Model model;

    size_type slot_size;

    key_type* key_ptr;

    node_ptr child;
    
    bitmap bitmap_ptr;

    class stats{
        size_type size;
        size_type write_times;
    }m_stats;

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
    
    key_type *key;

    value_type *data;

    class aex_RO_model;

    union Model
    {
        /* data */
        aex_RO_model* complex_model;
        linear_model<key_type> easy_model;
    }model;

    //inline size_type predict(key_type &k){
    //    return std::max(std::min(model.predict() * this->size, this->size - 1), 0);
    //}

    void construct(std::pair<key_type, value_type>* _data, size_type nums){
        assert(this->slot_size >= nums);
        for (size_type j = 0; j < nums; ++j){
            key[j] = _data[j].first;
            data[j] = _data[j].second;
        }
        this->size = nums;
        this->train_model();
    }

    void construct(const key_type *_key, const value_type *_data, size_type nums){
        memcpy(key, _key, nums * sizeof(key_type));
        data_memmove(data, _data, nums * sizeof(value_type));
        this->size = nums;
    }

    inline void train_model(){
        if (this->prop & ML_NODE){
            if (!(this->prop & COMPLEX_MODEL)){
                model.easy_model.train(this->key, this->size, this->size);
            }
            else{
                this->complex_model->construct(this);
            }
        }
    }
    
};

template<typename _Key,
        typename _Val, 
        typename traits>
class aex_data_node<_Key, _Val, traits>::aex_RO_model{
public:
    typedef aex_RO_model self;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef aex_data_node<key_type, value_type, traits> data_node;
    typedef data_node* data_node_ptr;

    typedef linear_model<key_type> model;
    
    aex_RO_model(){}

    inline size_type lower_bound(key_type &x){
        size_type predict_block_pos = max(0, min(block_num - 1, block_num * segments[0]->predict(x)));
        size_type block_pos = exponential_search_lower_bound(segments + 1, segments + block_num, predict_block_pos, 
        [](model &a, key_type &x)->bool{
            return a.inter < x;
        }) - segments;


        if (block_pos == 0) return 0;
        --block_pos;
        key_type* key;
        size_type inner_pos = exponential_search_lower_bound(key + offset[block_pos], key + offset[block_pos + 1], segments[block_pos]->predict()) - key;
        return inner_pos;
    }

    inline size_type upper_bound(key_type &x){
        size_type predict_block_pos = max(0, min(block_num - 1, block_num * segments[0]->predict(x)));
        size_type block_pos = exponential_search_upper_bound(segments + 1, segments + block_num, predict_block_pos, [](model &a, key_type &x)->bool{
            return a.inter < x;
        }) - segments;
        if (block_pos == 0) return 0;
        --block_pos;
        key_type* key;
        size_type inner_pos = exponential_search_upper_bound(key + offset[block_pos], key + offset[block_pos + 1], segments[block_pos]->predict()) - key;
        return inner_pos;
    }

    void construct(data_node_ptr data_node){
        this->_M_node = data_node;
        size_type size = _M_node->size;
        size_type max_block_size = sqrt(size);
        for (block_size = traits::MIN_BLOCK_SIZE; block_size > max_block_size; block_size <<= 1);
        block_num = (size - 1) / block_size + 1;
        char* data = (char*)malloc(align_8bits((block_num + 1)* sizeof(model)) + (block_num + 2) * sizeof(size_type)); 
        segments = static_cast<model*>(data); 
        offset = static_cast<size_type*>(data + align_8bits((block_num + 1)* sizeof(model)));
        size_type st, i;
        for (st = 0, i = 0; st < size; st += block_size, ++i){
            size_type now_block_size = std::min(block_size, size - st);
            offset[i] += st;
            segments[i].train(key + st, now_block_size, now_block_size);
        }
        offset[i] = size;
    }

    inline double error(){
        size_type size = _M_node->size;
        key_type *key = _M_node->key;
        double err = 0;
        for (size_type i = 0; i < size; ++i)
            err += sqr(this->lower_bound(key[i]) - i);
        err /= _M_node->size;
        err = sqrt(err);
        return err;
    }

    inline void insert(size_type pos){
        size_type offset_size = offset.size();
        size_type block_pos = std::lower_bound(offset.data(), offset.data() + offset_size, pos) - offset.data();
        for (size_type i = block_pos; i < offset_size; ++i) ++offset[i];
    }

    void free(){
        //allocator::free(segments);
        //allocator::free(offset);
        free(segments);
        free(offset);
    }
    
private:
    model* segments;
    size_type* offset;
    size_type block_size, block_num;
    data_node_ptr _M_node;
};

}