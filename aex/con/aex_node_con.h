#pragma once
#include "aex/aex_node.h"
namespace aex{

/*
    memory layout:
    meta(const size): size, prop, level, Model, slot size
    key array(variable size)
    pointer array(variable size)
    bitmap array(variable size)
    - mutex array (if multithread)
*/

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node_con : public aex_inner_node<_Key, _Val, traits>{

    static_assert(traits::AllowConcurrency == true, "aex_inner_node_con must allow concurrency");
    typedef _Key key_type;
    typedef _Val value_type;

    typedef aex_tree_con<_Key, _Val, traits> base_tree;
    
    typedef aex_default_components<traits> components;

    typedef aex_inner_node<_Key, _Val, traits> base_inner_node;
    typedef aex_inner_node_con<_Key, _Val, traits> inner_node;

    typedef typename components::bitmap_impl bitmap_impl;
    //typedef typename components::inner_node inner_node;
    typedef typename components::base_node base_node;
    typedef typename components::data_node data_node;

    typedef typename components::InnerNodeModel Model;
    typedef typename components::NodeAllocator NodeAllocator;
    typedef typename components::Lock Lock;
    typedef typename components::RWLock RWLock;

    typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;

    typedef base_node* node_ptr;
    typedef base_inner_node* base_inner_node_ptr;
    typedef inner_node* inner_node_ptr;

    //using base_inner_node::key_ptr;
    //using base_inner_node::child_ptr;
    //using base_inner_node::bitmap_ptr;
    //using base_node::node_mutex;

    aex_inner_node_con(slot_type _slot_size) : base_inner_node(_slot_size){
        node_mutex_array_ptr = (RWLock*)(malloc(NodeAllocator::MUTEX_MEMORY_USED(_slot_size)));
    }

    ~aex_inner_node_con(){
        if (node_mutex_array_ptr != nullptr)
            delete node_mutex_array_ptr;
    }

    aex_inner_node_con(aex_inner_node_con &other_node):base_inner_node(other_node){

    }
    aex_inner_node_con(aex_inner_node_con &&other_node):base_inner_node(other_node){

    }

    aex_inner_node_con(base_inner_node &other_node):base_inner_node(other_node){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node.bitmap_ptr, NodeAllocator::BITMAP_MEMORY_USED(other_node.slot_size));
        node_mutex_array_ptr = node_mutex_array_ptr = (RWLock*)(malloc(NodeAllocator::MUTEX_MEMORY_USED(this->slot_size)));;
    }

    aex_inner_node_con(base_inner_node &&other_node):base_inner_node(other_node){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
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
        node_mutex_array_ptr = (RWLock*)(malloc(NodeAllocator::MUTEX_ARRAY_MEMORY_USED(other_node.slot_size)));;
    }

    inner_node& operator = (base_inner_node &other_node){
        AEX_ASSERT(this->slot_size != other_node.slot_size);
        *static_cast<base_inner_node_ptr>(this) = static_cast<base_inner_node>(other_node);
        return this;
    }

    inner_node& operator = (base_inner_node &&other_node){
        AEX_ASSERT(this->slot_size != other_node.slot_size);
        *static_cast<base_inner_node_ptr>(this) = std::move(static_cast<base_inner_node>(other_node));
        if (this->node_mutex_array_ptr != nullptr)
            delete this->node_mutex_array_ptr;
        this->node_mutex_array_ptr = other_node.node_mutex_array_ptr;
        return this;
    }

    inner_node& operator = (inner_node &other_node){
        *static_cast<base_inner_node_ptr>(this) = static_cast<base_inner_node>(other_node);
        return *this;
    }

    inner_node& operator = (inner_node &&other_node){
        *static_cast<base_inner_node_ptr>(this) = std::move(static_cast<base_inner_node>(other_node));
        return *this;
    }

    inline void lock_array_shared(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].lock_shared();
    }

    inline void unlock_array_shared(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].unlock_shared();
    }

    //inline void lock_array_writer(slot_type lower_slot, slot_type upper_slot){
    //    slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
    //    for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
    //        this->node_mutex_array_ptr[i].lock();
    //}

    //inline void unlock_array_writer(slot_type lower_slot, slot_type upper_slot){
    //    slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
    //    for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
    //        this->node_mutex_array_ptr[i].unlock();
    //}

    inline bool try_lock_array_shared(const slot_type lower_slot, const slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        AEX_ASSERT(upper_mutex - lower_mutex <= 1);
        if (lower_mutex == upper_mutex)
            return this->node_mutex_array_ptr[lower_mutex].try_lock_shared();
        else{
            for (slot_type i = lower_mutex; i <= upper_mutex; ++i){
                if (this->node_mutex_array_ptr[i].try_lock_shared() == false){
                    for (slot_type j = lower_mutex; j < i; ++j)
                        this->node_mutex_array_ptr[j].unlock_shared();
                    return false;
                }
            }
        }
        return true;
    }

    inline void lock_array(const slot_type lower_slot, const slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].lock();
    }

    inline void unlock_array(const slot_type lower_slot, const slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].unlock();
    }

    using base_inner_node::find;

    inline bool find(const key_type& x, slot_type &ret) {
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            slot_type upper_pos = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size - 1);
            bool _ = this->try_lock_array_shared(pred_pos, upper_pos);
            if (_)
                return false;
            #ifdef AEX_TLI
            return SearchClass::lower_bound(this->key_ptr, this->key_ptr + this->slot_size, x, this->key_ptr + pred_pos) - this->key_ptr;
            #else
            ret = this->slot_size - 1;
            for (slot_type i = pred_pos; i < this->slot_size; ++i)
            if (x <= this->key_ptr[i]){
                ret = i;
                break;
            }
            this->unlock_array_shared(pred_pos, upper_pos);
            #endif
        }
        else{
            ret = this->base_inner_node::find(x);
        }
        return true;
    }

    bool insert(const key_type &key, const node_ptr child){
        bool ret;
        if (!IS_ML_NODE(this)) {
            ret = this->base_inner_node::insert(key, child);
        }
        else{
            slot_type pred_pos = this->predict(key);
            slot_type upper_pos = std::min(this->slot_size, pred_pos + 2 * traits::ERROR_BOUND);
            slot_type lower_pos = this->prev_item(pred_pos);
            this->lock_array(lower_pos, upper_pos);
            slot_type inserted_pos = pred_pos, upper_bound = std::min(this->slot_size - 1, pred_pos + traits::ERROR_BOUND);
            for (; inserted_pos < upper_bound; ++inserted_pos)
            if (key < this->key_ptr[inserted_pos]){
                break;
            }
            // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
            if (inserted_pos >= this->slot_size - 1 || inserted_pos - pred_pos >= traits::ERROR_BOUND){
                return false;
            }

            // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
            slot_type max_slot = std::min(inserted_pos + traits::ERROR_BOUND, this->slot_size - 1);
            for (slot_type i = inserted_pos; i < max_slot; ++i){
                if (bitmap_impl::at(this->bitmap_ptr, i)){
                    slot_type shift_pos = this->predict(this->key_ptr[i]);
                    if (i + 1 - shift_pos >= traits::ERROR_BOUND)
                        return false;
                }
                else{
                    slot_type prev_pos = this->prev_item(pred_pos);
                    this->unlock_array(pred_pos, i + 1);
                    this->lock_array(prev_pos, i + 1);
                    std::move_backward(this->key_ptr + inserted_pos, this->key_ptr + i, this->key_ptr + i + 1);
                    std::move_backward(this->child_ptr + inserted_pos, this->child_ptr + i, this->child_ptr + i + 1);
                    bitmap_impl::set_one(this->bitmap_ptr, i);
                    std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + inserted_pos + 1, key);
                    std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + inserted_pos + 1, child);
                    ++this->size; 
                    this->unlock_array(prev_pos, i + 1);
                    return true;
                }
            }
            ret = false;
            this->unlock_array(lower_pos, upper_pos);
        }
        return ret;
    }

    using base_inner_node::erase;

public:
    RWLock* node_mutex_array_ptr;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_data_node_con : public aex_static_data_node<_Key, _Val, traits>{
public:
    static_assert(traits::AllowConcurrency == true, "aex_data_node_con must allow concurrency");

    typedef aex_static_data_node<_Key, _Val, traits> base_data_node;

    typedef aex_data_node_con<_Key, _Val, traits> self;

    typedef aex_data_node_con<_Key, _Val, traits> data_node;

    typedef base_data_node* base_data_node_ptr;

    typedef data_node* data_node_ptr;

    aex_data_node_con(){}

    ~aex_data_node_con(){}

    aex_data_node_con(aex_data_node_con &other_node):base_data_node(other_node){}

    aex_data_node_con(aex_data_node_con &&other_node):base_data_node(other_node){}

    self& operator= (self &x){
        *static_cast<base_data_node_ptr>(this) = static_cast<base_data_node>(x);
        return *this;
    }

    self& operator= (self &&x){
        *static_cast<base_data_node_ptr>(this) = std::move(static_cast<base_data_node>(x));
        return *this;
    }

};

}
