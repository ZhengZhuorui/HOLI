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

    size_type size;

    unsigned int prop, level;
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

    /* used memory size of meta data */
    const static size_type memory_used = sizeof(self);

    /* used memory size of bitmap, align 8 bytes */
    inline static size_t BITMAP_MEMORY_USED(size_type slot_size){
        return align_8bytes(((slot_size >> 6) + ((slot_size & 63) > 0)) * sizeof(unsigned long long));
    }

    /* used memory size of key array, align 8 bytes */
    inline static size_type KEY_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(key_type));
    }

    /* used memory size of pointer array, align 8 bytes */
    inline static size_type PTR_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(char*));
    }


    inline static size_type DATA_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(_Val));
    }
    
    inline static size_t ML_INNER_NODE_MEMORY_USED(size_type slot_size){ 
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) +
        align_8bytes(sizeof(aex_inner_node<key_type, value_type, traits>)) ;
    }

    inline static size_t INNER_NODE_MEMORY_USED(size_type slot_size){
        return KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + 
        align_8bytes(sizeof(aex_inner_node<key_type, value_type, traits>));
    }

    /*
    inline key_type* key_ptr(){
        return static_cast<key_type*>(align_8bytes(static_cast<char*>(this) + memory_used));
    }

    inline node_ptr* child_ptr(){
        return static_cast<node_ptr*>(align_8bytes(static_cast<char*>(this) + memory_used) + 
        KEY_MEMORY_USED(this->slot_size));
    }

    inline bitmap bitmap_ptr(){
        return static_cast<bitmap>(align_8bytes(static_cast<char*>(this) + memory_used) + 
        KEY_MEMORY_USED(this->slot_size) + PTR_MEMORY_USED(this->slot_size));
    }
    */

    /* offset: meta data*/
    inline key_type* key_ptr() const {
        return reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(this) + memory_used));
    }
    
    /* offset: meta data + key array */
    inline node_ptr* child_ptr() const {
        return reinterpret_cast<node_ptr*>(align_8bytes(reinterpret_cast<size_t>(this) + memory_used) + KEY_MEMORY_USED(this->slot_size));
    }

    /* offset: meta data + key array + pointer array*/
    inline bitmap bitmap_ptr() const {
        return reinterpret_cast<bitmap>(align_8bytes(reinterpret_cast<size_t>(this) + memory_used) + 
        KEY_MEMORY_USED(this->slot_size) + PTR_MEMORY_USED(this->slot_size));
    }
    
    // clear bitmap
    inline void clear_bitmap(){
        memset(bitmap_ptr(), 0, BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear_key_array(){
        memset(key_ptr(), 0, KEY_MEMORY_USED(this->slot_size));
    }

    // construct a node with key array, don't check model is fit, check_rewired first
    void construct(const key_type* const key, const node_ptr* const child, const unsigned int n){
        size_type start = 0, pos;
        bitmap bm = this->bitmap_ptr();
        key_type* node_key = this->key_ptr();
        node_ptr* node_child = this->child_ptr();
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
        bitmap bm = this->bitmap_ptr();
        key_type* node_key = this->key_ptr();
        node_ptr* node_child = this->child_ptr();
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
        memcpy(this->key_ptr(), node->key_ptr(), KEY_MEMORY_USED(this->slot_size));
        memcpy(this->child_ptr(), node->child_ptr(), PTR_MEMORY_USED(this->slot_size));
        memcpy(this->bitmap_ptr(), node->bitmap_ptr(), BITMAP_MEMORY_USED(node->slot_size));
    }

    // return the slot of child node
    inline size_type at(const node_ptr node) const {
        bitmap bm = this->bitmap_ptr();
        node_ptr* child = this->child_ptr();
        if (node == nullptr) return this->slot_size;
        if (this->prop & ML_NODE){
            /* TODO: predict */
            /*
            for (size_type i = 0; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node)
                return i;
            */

            key_type key = (node->prop & LEAF) ? (static_cast<data_node_ptr>(node)->key[node->size - 1]) : (static_cast<inner_node_ptr>(node)->key_ptr()[static_cast<inner_node_ptr>(node)->last()]);
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

    /*
    inline size_type at(const key_type &x) const {
        key_type *key = key_ptr();
        for (size_type i = 0; i < this->slot_size; ++i)
            if (key[i] >= x) return i;
        return this->slot_size;
    }
    */

    // return the first item position.
    inline size_type first() const {
        if (this->prop & ML_NODE){
            bitmap bm = bitmap_ptr();
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
            bitmap bm = bitmap_ptr();
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
        bitmap bm = this->bitmap_ptr();
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
        bitmap bm = this->bitmap_ptr();
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
        return std::min(model.predict(key), this->slot_size - 1);
        //return model.predict(key);
    }

public:
    Model model;

    size_type slot_size;

    /*
    key_type* key_ptr;

    node_ptr* child_ptr;

    bitmap bitmap_ptr;
    */

};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_data_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef aex_data_node<_Key, _Val, traits> self;
    
    typedef _Key key_type;

    typedef _Val value_type;

    key_type key[traits::DATA_NODE_SLOT_SIZE];
    value_type data[traits::DATA_NODE_SLOT_SIZE];
    self *prev, *next;
};


/* TODO: if the data node set large? */


}