#pragma once
#include "aex/aex_balance.h"

namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_base{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef aex_node_base<key_type, value_type, traits> self;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef aex_node_balance_stats<typename traits::AllowBalance> node_balance_stats;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;

    typedef self node;

    typedef self* node_ptr;

    node_ptr prev, next;

    inner_node_ptr parent;

    // size: the child node of the node(inner node); the data of the node(data node)
    // slot_size: the slot of the node
    slot_type size, slot_size;

    // prop
    // level: node height
    unsigned int prop, level;
    
    node_balance_stats balance_stats;

    aex_node_base():prev(nullptr), next(nullptr), parent(nullptr), size(0), slot_size(0), prop(0), level(0), balance_stats(){}

    aex_node_base(aex_node_base &other_node):prev(other_node.prev), next(other_node.next), parent(other_node.parent), size(other_node.size), slot_size(other_node.slot_size), 
                                            prop(other_node.prop), level(other_node.level), balance_stats(other_node.balance_stats){}

    aex_node_base(aex_node_base &&other_node):prev(other_node.prev), next(other_node.next), parent(other_node.parent), size(other_node.size), slot_size(other_node.slot_size), 
                                            prop(other_node.prop), level(other_node.level), balance_stats(other_node.balance_stats){}

    aex_node_base& operator = (aex_node_base &other_node){
        this->prev = other_node.prev;this->next = other_node.next;this->parent = other_node.parent;this->size = other_node.size;
        this->slot_size = other_node.slot_size;this->prop = other_node.prop;this->level = other_node.level;this->balance_stats = other_node.balance_stats;
        return *this;
    }

    aex_node_base& operator = (aex_node_base &&other_node){
        this->prev = other_node.prev;this->next = other_node.next;this->parent = other_node.parent;this->size = other_node.size;
        this->slot_size = other_node.slot_size;this->prop = other_node.prop;this->level = other_node.level;this->balance_stats = other_node.balance_stats;
        return *this;
    }

};


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

    typedef typename traits::slot_type slot_type;

    typedef aex_tree<_Key, _Val, traits> Tree;

    typedef aex_node_allocator<_Key, _Val, traits> NodeAllocator;

    typedef aex_node_base<key_type, value_type, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    //typedef linear_model<key_type> Model;
    //typedef aex_model<key_type, traits> Model;

    //typedef gap_array_linear_model<key_type, traits> Model;
    typedef piecewise_linear_model<key_type, traits> Model;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;
    
    typedef inner_node* inner_node_ptr;

    aex_inner_node(){}

    ~aex_inner_node(){
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->key_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
    }

    aex_inner_node(inner_node &other_node):base_node(other_node), model(other_node.model){
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node->key_ptr, NodeAllocator::BITMAP_MEMORY_USED(other_node->slot_size));
    }

    aex_inner_node(inner_node &&other_node):base_node(other_node), model(other_node.model){
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->key_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
        this->key_ptr = other_node->key_ptr;
        this->child_ptr = other_node->child_ptr;
        this->bitmap_ptr = other_node->bitmap_ptr;
        other_node->key_ptr = nullptr;
        other_node->child_ptr = nullptr;
        other_node->bitmap_ptr = nullptr;
    }

    aex_inner_node& operator = (aex_inner_node &&other_node){
        static_cast<base_node>(*this) = static_cast<base_node>(other_node);
        model = other_node.model;
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->key_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
        this->key_ptr = other_node.key_ptr;
        this->child_ptr = other_node.child_ptr;
        this->bitmap_ptr = other_node.bitmap_ptr;
        other_node.key_ptr = nullptr;
        other_node.child_ptr = nullptr;
        other_node.bitmap_ptr = nullptr;
        return *this;
    }

    // clear bitmap
    inline void clear_bitmap(){
        memset(this->bitmap_ptr, 0, NodeAllocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear_key_array(){
        memset(this->key_ptr, 0, NodeAllocator::KEY_MEMORY_USED(this->slot_size));
    }

    inline void inplace_construct(){
        bitmap bm = this->bitmap_ptr;
        {
            for (slot_type i = 0; i < this->size; ++i)
                child_ptr[i]->parent = this;
        }

        if (IS_ML_NODE(this)){
            std::move_backward(this->key_ptr, this->key_ptr + this->size, this->key_ptr + this->slot_size);
            std::move_backward(this->child_ptr, this->child_ptr + this->size, this->child_ptr + this->slot_size);
            slot_type start = 0, his_pos = 0;
            for (slot_type i = this->slot_size - this->size; i < this->slot_size; ++i){
                slot_type pos = this->predict(key_ptr[i]);
                start = std::max(start, pos);
                bitmap_impl::set_one(bm, start);
                AEX_ASSERT(start - pos < traits::ERROR_BOUND);
                AEX_ASSERT(start - pos < traits::ERROR_BOUND);
                std::fill(this->key_ptr + his_pos, this->key_ptr + start + 1, key_ptr[i]);
                std::fill(this->child_ptr + his_pos, this->child_ptr + start + 1, child_ptr[i]);
                his_pos = start + 1;
                ++start;
            }
            std::fill(this->key_ptr + his_pos, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
            std::fill(this->child_ptr + his_pos, this->child_ptr + this->slot_size, this->child_ptr[start - 1]);
        }
        else{
        }
    }

    // Construct a node with key array, don't check model is fit. 
    inline void construct(const key_type* const key, const node_ptr* const child, const slot_type n){
        if (IS_ML_NODE(this)){
            model.train(key, n, this->real_slot_size());
            this->construct(key, child, n, model);
        }
        else{
            std::copy(key, key + n - 1, this->key_ptr);
            std::copy(child, child + n, this->child_ptr);
        }
    }

    // construct a node with key array and model
    
    inline void construct(const key_type* const key, const node_ptr* const child, const slot_type n, const Model &m){
        AEX_ASSERT(n > 0);
        bitmap bm = this->bitmap_ptr;
        key_type* node_key = this->key_ptr;
        node_ptr* node_child = this->child_ptr;
        this->clear_bitmap();
        this->clear_key_array();
        this->model = m;
        // meta
        {
            for (slot_type i = 0; i < n; ++i)
                child[i]->parent = this;
        }
        if (IS_ML_NODE(this)){
            slot_type start = 0, his_pos = 0;
            for (slot_type i = 0; i < n; ++i){
                slot_type pos = this->predict(key[i]);
                start = std::max(start, pos);
                bitmap_impl::set_one(bm, start);
                AEX_ASSERT(start - pos < traits::ERROR_BOUND);
                std::fill(this->key_ptr + his_pos, this->key_ptr + start + 1, key[i]);
                std::fill(this->child_ptr + his_pos, this->child_ptr + start + 1, child[i]);
                his_pos = start + 1;
                ++start;
            }
            std::fill(this->key_ptr + his_pos, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
            std::fill(this->child_ptr + his_pos, this->child_ptr + this->slot_size, child[n - 1]);
        }
        else{
            std::copy(key, key + n, node_key);
            std::copy(child, child + n, node_child);
        }
    }

    // insert a node
    bool insert(const key_type &key, const node_ptr child){
        if (!IS_ML_NODE(this)) {
            slot_type pos = this->find(key);
            std::move_backward(this->key_ptr + pos, this->key_ptr + this->size, this->key_ptr + this->size + 1);
            std::move_backward(this->child_ptr + pos, this->child_ptr + this->size, this->child_ptr + this->size + 1);
            ++this->size;
            child->parent = this;
            return true;
        }
        else{
            slot_type pred_pos = this->predict(key);
            slot_type inserted_pos = pred_pos;
            for (; inserted_pos < this->slot_size && inserted_pos - pred_pos < traits::ERROR_BOUND; ++inserted_pos)
            if (key < this->key_ptr[inserted_pos]){
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
            slot_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size);
            for (slot_type i = inserted_pos; i < max_slot; ++i){
                if (bitmap_impl::at(this->bitmap_ptr, i)){
                    slot_type shift_pos = this->predict(this->key_ptr[i]);
                    if (i + 1 - shift_pos >= traits::ERROR_BOUND){
                        return false;
                    }
                }
                else{
                    std::move_backward(this->key_ptr + inserted_pos, this->key_ptr + i, this->key_ptr + i + 1);
                    std::move_backward(this->child_ptr + inserted_pos, this->child_ptr + i, this->child_ptr + i + 1);
                    bitmap_impl::set_one(this->bitmap_ptr, i);
                    slot_type prev_pos = this->prev_item(inserted_pos);
                    std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + inserted_pos + 1, key);
                    std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + inserted_pos + 1, child);
                    ++this->size; 
                    child->parent = this;
                    return true;
                }
            }
            //AEX_PRINT("inserted_pos=" << inserted_pos << ", pred_pos=" << pred_pos << ", max_slot=" << max_slot);
            //for (size_type i = pred_pos; i < max_slot; ++i){
            //    AEX_PRINT("key_ptr[" << i << "]=" << key_ptr[i] << ", b" << (bitmap_impl::at(this->bitmap_ptr, i) > 0));
            //}
            // if need shift move more than ERROR_BOUND item, return false
            return false;
        }
    }

    // erase a node
    bool erase(node_ptr node){
        slot_type pos = this->at(node);
        if (pos == this->slot_size)
            return false;
        --this->size;
        if (IS_ML_NODE(node)){
            bitmap_impl::set_zero(this->bitmap_ptr, pos);
            slot_type prev_pos = this->prev_item(pos);
            if (pos < this->slot_size - 1){
                std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + pos + 1, this->key_ptr[pos + 1]);
                std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + pos + 1, this->child_ptr[pos + 1]);
            }
        }
        else{
            std::move(this->key_ptr + pos + 1, this->key_ptr + this->size, this->key_ptr + pos);
            std::move(this->child_ptr + pos + 1, this->child_ptr + this->size, this->child_ptr + pos);
        }
        return true;
    }

    // return the slot of child node
    inline slot_type at(const node_ptr node) const {
        bitmap bm = this->bitmap_ptr;
        node_ptr* child = this->child_ptr;
        if (node == nullptr) return this->slot_size;
        key_type node_key = IS_LEAF_NODE(node) ? (static_cast<data_node_ptr>(node)->key[0]) : (static_cast<inner_node_ptr>(node)->key_ptr[0]); 
        if (IS_ML_NODE(this)){
            slot_type pos = this->predict(node_key);
            for (slot_type i = pos; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node) 
                return i;
        }
        else{
            if (this->size > traits::BINEARY_SEARCH_SIZE){
                slot_type pos = std::lower_bound(this->key_ptr, this->key_ptr + this->size, node_key) - this->key_ptr;
                return (pos == this->size) ? this->slot_size : pos;
            }
            else{
                return std::find(child, child + this->size, node) - child;
            }
        }
        return this->slot_size;
    }

    // return the first item position.
    inline slot_type first() const {
        return 0;
    }

    // return the last item position.
    // inline size_type last() const{ return this->slot_size - 1;}
    inline slot_type last() const {
        return (this->prop & ML_NODE) ? this->slot_size : this->size - 1;
    }
    
    // return the prev item position. If none, return slot_size
    inline slot_type prev_item(slot_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (pos == 0) return this->slot_size;
        /* TODO: use __buitlin_clzll */
        if (IS_ML_NODE(this)){
            for (slot_type i = pos - 1; i >= 0; --i)
            if (bitmap_impl::at(bm, i))
                return i;
            return -1;
        }
        else 
            return pos - 1;
    }

    // return the next item position. If none, return slot_size
    inline slot_type next_item(slot_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (IS_ML_NODE(this)){
            for (slot_type i = pos + 1; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i))
                return i;
            return this->slot_size;
        }
        else 
            return (pos >= this->size) ? this->slot_size : pos + 1;
    }

    // real_slot_size() mean slot size minus error bound
    inline slot_type real_slot_size() const{
        return IS_ML_NODE(this) ? (this->slot_size - traits::ERROR_BOUND) : this->slot_size;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline slot_type predict(const key_type& key) const {
        return std::max((slot_type)0, std::min(static_cast<slot_type>(model.predict(key) * this->real_slot_size()), this->slot_size - 1));
    }

    // (X) if no item greater than or equal x, return node->slot_size
    // find key pos in which slot. If not, return node->slot_size
    inline slot_type find(const key_type& x) const{
        slot_type pos = -1;
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            slot_type upper_bound = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size);
            for (slot_type i = pred_pos; i < upper_bound; ++i)
            if (x <= key_ptr[i]){
                //AEX_PRINT("key=" << x << ", node key=" << key_ptr[i]);
                return i;
            }
            return this->slot_size;
        }
        else{
            pos = std::lower_bound(key_ptr, key_ptr + this->size, x) - key_ptr;
            if (pos == this->size)
                return this->slot_size;
            //AEX_PRINT("key=" << x << ", node key=" << key_ptr[pos]);
            return pos;
        }
    }


public:

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

    typedef aex_node_base<_Key, _Val, traits> base_node;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef linear_model<_Key, traits> Model;

    Model model;
    
    key_type __restrict__ *key;

    value_type __restrict__ *data;

    //typedef linear_model<key_type, traits> Model;

    aex_data_node(){
    }

    ~aex_data_node(){
        //if ((this->prop & COMPLEX_MODEL) & this->model.complex_model == nullptr){
        //    delete this->model.complex_model;
        //}
        if (key != nullptr)
            free(key);
        if (data != nullptr)
            free(data);
    }

    aex_data_node(aex_data_node &other_node):base_node(other_node), model(other_node.model){
        std::copy(other_node.key, other_node.key + other_node.size, this->key);
        std::copy(other_node.data, other_node.data + other_node.size, this->data);
    }

    aex_data_node(aex_data_node &&other_node):base_node(other_node), model(other_node.model){
        if (this->key != nullptr)
            free(this->key);
        if (this->data != nullptr)
            free(this->data);
        this->key = other_node.key;
        this->data = other_node.data;
        other_node.key = nullptr;
        other_node.data = nullptr;
    }

    aex_data_node& operator = (aex_data_node &&other_node){
        static_cast<base_node>(*this) = static_cast<base_node>(other_node);
        model = other_node.model;
        if (this->key != nullptr)
            free(this->key);
        if (this->data != nullptr)
            free(this->data);
        this->key = other_node.key;
        this->data = other_node.data;
        other_node.key = nullptr;
        other_node.data = nullptr;
        return *this;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline slot_type predict(const key_type& key) const {
        return std::max((slot_type)0, std::min(static_cast<slot_type>(model.predict(key) * this->size), this->size - 1));
    }

    void construct(const key_type *_key, const value_type *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        std::move(_key, _key + nums, this->key);
        std::move(_data, _data + nums, this->data);
        this->size = nums;
    }

    void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    void construct(const key_type *_key, const value_type *_data, slot_type nums, Model &m){
        std::move(_key, _key + nums, this->key);
        std::move(_data, _data + nums, this->data);
        this->size = nums;
        this->model = m;
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_upper_pos(x);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
        return pos;
    }

    inline bool erase(const slot_type pos){
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->size--;
        return true;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x){
        slot_type pos = this->slot_size;
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            pos = std::lower_bound(this->key, this->key + this->size, x) - this->key;
        }
        return pos;
    }

    inline slot_type find_upper_pos(const key_type &x){
        slot_type pos = this->slot_size;
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            pos = aex::exponential_search_upper_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            pos = std::upper_bound(this->key, this->key + this->size, x) - this->key;
        }
        return pos;
    }

    inline double RMSE(){
        if (IS_ML_NODE(this)){
            return this->model.RMSE(this->key, this->size);
        }
        else return 0;
    }
};

}