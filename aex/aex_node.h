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

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef aex_node_base<key_type, value_type, traits> self;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    //typedef typename base_tree::data_node data_node;
    typedef aex_static_data_node<_Key, _Val, traits> data_node;

    typedef aex_node_balance_stats<typename traits::AllowBalance> node_balance_stats;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;

    typedef self node;

    typedef self* node_ptr;

    node_ptr prev, next;

    inner_node_ptr parent;

    // size: the child node of the node(inner node); the data of the node(data node)
    slot_type size; 

    unsigned int prop;

    aex_node_base():prev(nullptr), next(nullptr), parent(nullptr), size(0), prop(0){}

    aex_node_base(aex_node_base &other_node):prev(other_node.prev), next(other_node.next), parent(other_node.parent), size(other_node.size), prop(other_node.prop){}
    aex_node_base(aex_node_base &&other_node):prev(other_node.prev), next(other_node.next), parent(other_node.parent), size(other_node.size), prop(other_node.prop){}

    aex_node_base& operator = (aex_node_base &other_node){
        this->prev = other_node.prev;this->next = other_node.next;this->parent = other_node.parent;this->size = other_node.size;
        this->prop = other_node.prop;
        return *this;
    }

    aex_node_base& operator = (aex_node_base &&other_node){
        this->prev = other_node.prev;this->next = other_node.next;this->parent = other_node.parent;this->size = other_node.size;
        this->prop = other_node.prop;
        return *this;
    }
};


template<typename _Key,
        typename _Val,
        typename traits>
struct aex_dynamic_node_base: public aex_node_base<_Key, _Val, traits>{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef aex_node_base<key_type, value_type, traits> base_node;

    typedef base_node* node_ptr;

    typedef aex_dynamic_node_base<key_type, value_type, traits> self;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    //typedef typename base_tree::data_node data_node;
    typedef aex_static_data_node<_Key, _Val, traits> data_node;

    typedef aex_node_balance_stats<typename traits::AllowBalance> node_balance_stats;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;

    // size: the child node of the node(inner node); the data of the node(data node)
    // slot_size: the slot of the node
    slot_type slot_size;

    // prop
    // level: node height
    unsigned int level;
    
    node_balance_stats balance_stats;

    aex_dynamic_node_base():base_node(), slot_size(0),  level(0), balance_stats(){}

    explicit aex_dynamic_node_base(slot_type _slot_size): base_node(), slot_size(_slot_size), level(0), balance_stats(){}

    aex_dynamic_node_base(aex_dynamic_node_base &other_node):base_node(other_node), slot_size(other_node.slot_size), 
                                            level(other_node.level), balance_stats(other_node.balance_stats){}

    aex_dynamic_node_base(aex_dynamic_node_base &&other_node):base_node(other_node), slot_size(other_node.slot_size), 
                                            level(other_node.level), balance_stats(other_node.balance_stats){}

    aex_dynamic_node_base& operator = (aex_dynamic_node_base &other_node){
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        this->slot_size = other_node.slot_size;this->prop = other_node.prop;this->level = other_node.level;this->balance_stats = other_node.balance_stats;
        return *this;
    }

    aex_dynamic_node_base& operator = (aex_dynamic_node_base &&other_node){
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
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
struct aex_inner_node : public aex_dynamic_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef aex_node_allocator<key_type, value_type, traits> NodeAllocator;

    typedef aex_node_base<key_type, value_type, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_dynamic_node_base<key_type, value_type, traits> base_dynamic_node;

    typedef base_dynamic_node* dynamic_node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap_base bitmap_base;

    typedef typename bitmap_impl::bitmap bitmap;

    //typedef linear_model<key_type> Model;
    //typedef aex_model<key_type, traits> Model;

    //typedef gap_array_linear_model<key_type, traits> Model;
    typedef piecewise_linear_model<key_type, traits> Model;

    typedef aex_static_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;
    
    typedef inner_node* inner_node_ptr;


    explicit aex_inner_node(slot_type _slot_size):base_dynamic_node(_slot_size){
        this->key_ptr = static_cast<key_type*>(malloc(NodeAllocator::KEY_MEMORY_USED(this->slot_size)));
        this->child_ptr = static_cast<node_ptr*>(malloc(NodeAllocator::PTR_MEMORY_USED(this->slot_size)));
        this->bitmap_ptr = static_cast<bitmap>(malloc(NodeAllocator::BITMAP_MEMORY_USED(this->slot_size)));
    }

    ~aex_inner_node(){
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->child_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
    }

    aex_inner_node(inner_node &other_node):base_dynamic_node(other_node), model(other_node.model){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node.bitmap_ptr, NodeAllocator::BITMAP_MEMORY_USED(other_node.slot_size));
    }

    aex_inner_node(inner_node &&other_node):base_dynamic_node(other_node), model(other_node.model){
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
        if (IS_ML_NODE(this)){
            for (slot_type i = 0; i < this->slot_size; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i))
                this->child_ptr[i]->parent = this;
        }
        else{
            for (slot_type i = 0; i < this->size; ++i)
                this->child_ptr[i]->parent = this;
        }
    }

    aex_inner_node& operator = (aex_inner_node &other_node){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        *static_cast<dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node.bitmap_ptr, NodeAllocator::BITMAP_MEMORY_USED(other_node.slot_size));
        return *this;
    }

    aex_inner_node& operator = (aex_inner_node &&other_node){
        *static_cast<dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->child_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
        this->key_ptr = other_node.key_ptr;
        this->child_ptr = other_node.child_ptr;
        this->bitmap_ptr = other_node.bitmap_ptr;
        other_node.key_ptr = nullptr;
        other_node.child_ptr = nullptr;
        other_node.bitmap_ptr = nullptr;
        if (IS_ML_NODE(this)){
            for (slot_type i = 0; i < this->slot_size; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i)){
                this->child_ptr[i]->parent = this;
            }
            this->child_ptr[this->slot_size - 1]->parent = this;
        }
        else{
            for (slot_type i = 0; i < this->size; ++i)
                this->child_ptr[i]->parent = this;
        }
        return *this;
    }

    // clear bitmap
    inline void clear_bitmap(){
        memset(this->bitmap_ptr, 0, NodeAllocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear(){
        clear_bitmap();
        std::fill(this->key_ptr, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        std::fill(this->child_ptr, this->child_ptr + this->slot_size, nullptr);
    }

    // Construct a node with key array, don't check model is fit. 
    inline void construct(const key_type* const key, const node_ptr* const child, const slot_type n){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        this->clear();
        this->size = n;
        std::copy(key, key + n - 1, this->key_ptr);
        std::copy(child, child + n, this->child_ptr);
        std::fill(this->key_ptr + n - 1, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        std::fill(this->child_ptr + n, this->child_ptr + this->slot_size, child[n - 1]);
        for (slot_type i = 0; i < n; ++i)
            child[i]->parent = this;
    }

    // construct a node with key array and model
    inline void construct(const key_type* const key, const node_ptr* const child, const slot_type n, const Model &m){
        AEX_ASSERT(n > 0);
        bitmap bm = this->bitmap_ptr;
        this->clear();
        this->model = m;
        this->size = n;
        // meta
        for (int i = 0; i < n; ++i)
            child[i]->parent = this;
        AEX_ASSERT(IS_ML_NODE(this) == true);
        slot_type start = 0, his_pos = 0;
        for (slot_type i = 0; i < n - 1; ++i){
            slot_type pos = this->predict(key[i]);
            start = std::max(start, pos);
            bitmap_impl::set_one(bm, start);
            //if (start - pos >= traits::ERROR_BOUND){
            //    for (slot_type j = 0; j < n - 1; ++j)
            //        AEX_PRINT(this->predict(key[i]) << ", " << this->model.predict(key[i]));
            //    AEX_PRINT("check=" << base_tree::check_collision(key, n - 1, this->real_slot_size(), this->model));
            //}
            AEX_ASSERT(start - pos < traits::ERROR_BOUND);
            AEX_ASSERT(start < this->slot_size - 1);
            std::fill(this->key_ptr + his_pos, this->key_ptr + start + 1, key[i]);
            std::fill(this->child_ptr + his_pos, this->child_ptr + start + 1, child[i]);
            ++start;
            his_pos = start;
        }
        std::fill(this->key_ptr + his_pos, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        std::fill(this->child_ptr + his_pos, this->child_ptr + this->slot_size, child[n - 1]);
    }

    inline void inplace_construct(slot_type n){
        if (IS_ML_NODE(this))
            construct(this->key_ptr + this->slot_size - n, this->child_ptr + this->slot_size - n, n, this->model);
        else{
            std::move(this->key_ptr + this->slot_size - n, this->key_ptr + this->slot_size, this->key_ptr);
            std::move(this->child_ptr + this->slot_size - n, this->child_ptr + this->slot_size, this->child_ptr);
        }
    }

    // insert a node
    inline bool insert(const key_type &key, node_ptr child){
        if (!IS_ML_NODE(this)) {
            AEX_ASSERT(this->size < this->slot_size);
            slot_type pos = this->find(key);
            AEX_ASSERT(pos < this->size);
            std::move_backward(this->key_ptr + pos, this->key_ptr + this->size, this->key_ptr + this->size + 1);
            std::move_backward(this->child_ptr + pos, this->child_ptr + this->size, this->child_ptr + this->size + 1);
            this->key_ptr[pos] = key;
            this->child_ptr[pos] = child;
            ++this->size;
            child->parent = this;
            return true;
        }
        else{
            slot_type pred_pos = this->predict(key);
            slot_type inserted_pos = pred_pos, upper_bound = std::min(this->slot_size - 1, pred_pos + traits::ERROR_BOUND);
            for (; inserted_pos < upper_bound; ++inserted_pos)
            if (key < this->key_ptr[inserted_pos]){
                break;
            }
            // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
            if (inserted_pos >= this->slot_size - 1 || inserted_pos - pred_pos >= traits::ERROR_BOUND){
                //static int insert_pos_large = 0;
                //insert_pos_large += (inserted_pos >= this->slot_size - 1);
                //AEX_PRINT("insert_pos_large=" << insert_pos_large);
                //static int insert_error_large = 0;
                //insert_error_large += inserted_pos - pred_pos >= traits::ERROR_BOUND;
                //AEX_PRINT("insert_error_large=" << insert_error_large);
                return false;
            }

            // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
            slot_type max_slot = std::min(inserted_pos + traits::ERROR_BOUND, this->slot_size - 1);
            for (slot_type i = inserted_pos; i < max_slot; ++i){
                //if (bitmap_impl::at(this->bitmap_ptr, i)){
                //    slot_type shift_pos = this->predict(this->key_ptr[i]);
                //    if (i + 1 - shift_pos >= traits::ERROR_BOUND){
                //        //static int shift_key_error_large = 0;
                //        //++shift_key_error_large;
                //        //AEX_PRINT("shift_key_error_large=" << shift_key_error_large);
                //        return false;
                //    }
                //}
                //else{
                if (!bitmap_impl::at(this->bitmap_ptr, i)){
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
            // if need shift move more than ERROR_BOUND item, return false
            //static int no_empty_pos = 0;
            //++no_empty_pos;
            //AEX_PRINT("no_empty_pos=" << no_empty_pos);
            return false;
        }
    }

    // erase a  node
    inline void erase(node_ptr node){
        AEX_ASSERT(node != this->child_ptr[this->slot_size - 1]);
        slot_type pos = this->at(node);
        if (IS_ML_NODE(this)){
            AEX_ASSERT(pos < this->slot_size - 1);
            AEX_ASSERT(bitmap_impl::at(this->bitmap_ptr, pos)!=0);
            bitmap_impl::set_zero(this->bitmap_ptr, pos);
            slot_type prev_pos = this->prev_item(pos);
            std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + pos + 1, this->key_ptr[pos + 1]);
            std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + pos + 1, this->child_ptr[pos + 1]);
        }
        else{
            if (pos >= this->size - 1)
                AEX_PRINT("size=" << this->size << ", slot size=" << this->slot_size);
            AEX_ASSERT(pos < this->size - 1);
            std::move(this->key_ptr + pos + 1, this->key_ptr + this->size, this->key_ptr + pos);
            std::move(this->child_ptr + pos + 1, this->child_ptr + this->size, this->child_ptr + pos);
            this->key_ptr[this->size - 1] = std::numeric_limits<key_type>::max();
            this->child_ptr[this->size - 1] = this->child_ptr[this->size - 2];
        }
        --this->size;
    }

    // return the slot of child node
    inline slot_type at(const node_ptr node) const {
        AEX_ASSERT(node != nullptr);
        //if (node == nullptr) 
        //    return this->slot_size;
        bitmap bm = this->bitmap_ptr;
        node_ptr* child = this->child_ptr;
        if (IS_ML_NODE(this)){
            if (node == this->child_ptr[this->slot_size - 1])
                return this->slot_size - 1;

            key_type last_key;
            if (IS_LEAF_NODE(node)) {
                if (node->size > 0){
                    last_key = static_cast<data_node_ptr>(node)->key[node->size - 1];
                }
                else{
                    for (slot_type i = 0; i < this->slot_size; ++i)
                    if (bitmap_impl::at(bm, i) && child_ptr[i] == node)
                        return i;
                    //for (slot_type i = 0; i < this->slot_size; ++i)
                    //if (child_ptr[i] == node)
                    //    return i;
                    AEX_ASSERT(0 == 1);
                    return this->slot_size;
                }
            }
            else if (IS_ML_NODE(node)){
                if (node->size > 1){
                    slot_type pos = static_cast<inner_node_ptr>(node)->prev_item(static_cast<inner_node_ptr>(node)->slot_size - 1);
                    AEX_ASSERT(pos >= 0);
                    last_key = static_cast<inner_node_ptr>(node)->key_ptr[pos];
                }
                else{
                    for (slot_type i = 0; i < this->slot_size; ++i)
                    if (bitmap_impl::at(bm, i) && child_ptr[i] == node)
                        return i;
                    //for (slot_type i = 0; i < this->slot_size; ++i)
                    //if (child_ptr[i] == node)
                    //    return i;
                    AEX_ASSERT(0 == 1);
                    return this->slot_size;
                }
            }
            else
                last_key = static_cast<inner_node_ptr>(node)->key_ptr[node->size - 2];
            slot_type pred_pos = this->predict(last_key);
            for (slot_type i = pred_pos; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node) 
                return i;
            AEX_WARNING("IS LEAF NODE?" << IS_LEAF_NODE(node) << ", IS_ML_NODE?" << IS_ML_NODE(node));
            AEX_WARNING("node=" << node << ", node->size=" << node->size << ", last_key=" << last_key << ", " << this->child_ptr[this->slot_size - 1]);
            AEX_ASSERT(0 == 1);
            return this->slot_size;
        }
        else{
            slot_type pos = std::find(child, child + this->size - 1, node) - child;
            //if (pos == this->size){
            //    AEX_PRINT("child is leaf?" << IS_LEAF_NODE(node));
            //    if (IS_LEAF_NODE(node))
            //        AEX_PRINT("last key=" << static_cast<data_node_ptr>(node)->key[node->size - 1]);
            //    AEX_PRINT("size=" << this->size);
            //    for (slot_type i = 0; i < this->size; ++i){
            //        AEX_PRINT("key=" << this->key_ptr[i] << ", node=" << this->child_ptr[i]);
            //    }
            //}
            AEX_ASSERT(pos < this->size);
            return pos;
        }
    }

    // return the prev item position. If none, return slot_size
    inline slot_type prev_item(slot_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (pos == 0) return -1;
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
        //return IS_ML_NODE(this) ? (this->slot_size - traits::ERROR_BOUND) : this->slot_size;
        return (this->slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE) ? (this->slot_size - traits::ERROR_BOUND) : this->slot_size;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline slot_type predict(const key_type& key) const {
        AEX_ASSERT(model.predict(key) < 1.0 + 1e-6);
        //return std::max((slot_type)0, std::min(static_cast<slot_type>(model.predict(key) * this->real_slot_size()), this->slot_size - 1));
        return std::max(0, static_cast<slot_type>(model.predict(key) * this->real_slot_size()));
    }

    // find key pos in which slot. If not, return last item(node->slot_size)
    inline slot_type find(const key_type& x) const{
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            //slot_type upper_bound = std::min(this->slot_size, pred_pos + traits::ERROR_BOUND + 1);
            for (slot_type i = pred_pos; i < this->slot_size; ++i)
            if (x <= key_ptr[i]){
                return i;
            }
            return this->slot_size - 1;
        }
        else{
            slot_type pos = std::lower_bound(this->key_ptr, this->key_ptr + this->size - 1, x) - this->key_ptr;
            return pos;
        }
    }

    inline slot_type find_child(const key_type& x) const{
        return child_ptr[this->find(x)];
    }

public:

    Model model;

    key_type* key_ptr;

    node_ptr* child_ptr;
    
    bitmap bitmap_ptr;
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_data_node : public aex_dynamic_node_base<_Key, _Val, traits>{
public:

    typedef aex_node_allocator<_Key, _Val, traits> NodeAllocator;

    typedef aex_node_base<_Key, _Val, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_dynamic_node_base<_Key, _Val, traits> base_dynamic_node;
    
    typedef base_dynamic_node* base_dynamic_node_ptr;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef linear_model<_Key, traits> Model;

    Model model;
    
    key_type *key;

    value_type *data;

    //typedef linear_model<key_type, traits> Model;

    aex_data_node(){}

    explicit aex_data_node(slot_type _slot_size):base_dynamic_node(_slot_size){
        this->key = static_cast<key_type*>(malloc(NodeAllocator::KEY_MEMORY_USED(_slot_size)));
        this->data = static_cast<value_type*>(malloc(NodeAllocator::KEY_MEMORY_USED(_slot_size)));
        std::fill(this->key, this->key + _slot_size, std::numeric_limits<key_type>::max());
    }

    ~aex_data_node(){
        if (key != nullptr)
            free(this->key);
        if (data != nullptr)
            free(this->data);
    }

    aex_data_node(aex_data_node &other_node):base_dynamic_node(other_node), model(other_node.model){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key, other_node.key + other_node.size, this->key);
        std::copy(other_node.data, other_node.data + other_node.size, this->data);
    }

    aex_data_node(aex_data_node &&other_node):base_dynamic_node(other_node), model(other_node.model){
        if (this->key != nullptr)
            free(this->key);
        if (this->data != nullptr)
            free(this->data);
        this->key = other_node.key;
        this->data = other_node.data;
        other_node.key = nullptr;
        other_node.data = nullptr;
    }

    aex_data_node& operator = (aex_data_node &other_node){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        *static_cast<base_dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        std::copy(other_node.key, other_node.key + other_node.size, this->key);
        std::copy(other_node.data, other_node.data + other_node.size, this->data);
        return *this;
    }

    aex_data_node& operator = (aex_data_node &&other_node){
        *static_cast<node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
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
        std::copy(_key, _key + nums - 1, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
    }

    void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    void construct(const key_type *_key, const value_type *_data, slot_type nums, Model &m){
        AEX_ASSERT(IS_ML_NODE(this) == true);
        std::move(_key, _key + nums, this->key);
        std::move(_data, _data + nums, this->data);
        this->size = nums;
        this->model = m;
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_lower_pos(x);
        AEX_ASSERT(x < key[pos]);
        insert(x, data, pos);
        return pos;
    }

    // insert a item in position
    inline void insert(const key_type &x, const value_type &data, const slot_type pos){
        AEX_ASSERT(this->size == this->slot_size);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
    }

    inline void erase(const slot_type pos){
        AEX_ASSERT(pos < this->size);
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->key[this->size - 1] = std::numeric_limits<key_type>::max();
        this->size--;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x){
        slot_type pos;
        if (traits::AllowDynamicDataNode::value && IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            for (pos = 0; pos < this->size; ++pos)
            if (x <= key[pos])
                break;
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

template<typename _Key,
        typename _Val,
        typename traits>
class aex_static_data_node : public aex_node_base<_Key, _Val, traits>{
public:

    typedef aex_node_allocator<_Key, _Val, traits> NodeAllocator;

    typedef aex_node_base<_Key, _Val, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_static_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;
    
    key_type key[traits::MIN_DATA_NODE_SLOT_SIZE];

    value_type data[traits::MIN_DATA_NODE_SLOT_SIZE];

    typedef linear_model<key_type, traits> Model;

    aex_static_data_node():base_node(){
    }

    //explicit aex_static_data_node(slot_type _slot_size):base_node(_slot_size){
    //}

    ~aex_static_data_node(){
    }

    aex_static_data_node(aex_static_data_node &other_node):base_node(other_node){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
    }

    aex_static_data_node(aex_static_data_node &&other_node):base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
    }

    aex_static_data_node& operator = (aex_static_data_node &other_node){
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        return *this;
    }

    aex_static_data_node& operator = (aex_static_data_node &&other_node){
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        return *this;
    }

    inline void construct(const key_type *_key, const value_type *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        AEX_ASSERT(nums >= traits::MIN_DATA_NODE_SLOT_SIZE / 2);
        std::copy(_key, _key + nums, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
        std::fill(this->key + nums, this->key + traits::MIN_DATA_NODE_SLOT_SIZE, std::numeric_limits<key_type>::max());
    }

    inline void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        AEX_ASSERT(nums >= traits::MIN_DATA_NODE_SLOT_SIZE / 2);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    inline void construct(const key_type *_key, const value_type *_data, slot_type nums, Model &m){
        AEX_ASSERT(false == true);
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_upper_pos(x);
        insert(x, data, pos);
        return pos;
    }

    // insert a item in position
    inline void insert(const key_type &x, const value_type &data, const slot_type pos){
        AEX_ASSERT(this->size < traits::MIN_DATA_NODE_SLOT_SIZE);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
    }

    inline void erase(const slot_type pos){
        AEX_ASSERT(pos < this->size);
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->key[this->size - 1] = std::numeric_limits<key_type>::max();
        this->size--;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x){
        //for (int i = 0; i < this->size; ++i)
        //if (x <= key[i])
        //    return i;
        //return this->size;
        return std::lower_bound(this->key, this->key + this->size, x) - this->key;
    }

    inline slot_type find_upper_pos(const key_type &x){
        slot_type pos = 0;
        for (int i = 0; i < this->size; ++i)
        if (x < key[i])
            return i;
        return pos;
    }

};

}