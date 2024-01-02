#pragma once
#include "aex/aex_node.h"
namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_tree;
template<typename _Key, typename _Val, typename traits> class aex_node_allocator;

template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_data_node;

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
#ifdef AEX_TLI
        typename SearchClass,
#endif
        typename traits>
struct aex_inner_node_con : public aex_inner_node<_Key, _Val, traits>{
    #ifdef AEX_TLI
    typedef aex_inner_node<_Key, _Val, SearchClass, traits> base_inner_node;
    typedef aex_inner_node_con<_Key, _Val, SearchClass, traits> inner_node_con;
    #else
    typedef aex_inner_node<_Key, _Val, traits> base_inner_node;
    typedef aex_inner_node_con<_Key, _Val, traits> inner_node_con;
    #endif

    typedef base_inner_node* base_inner_node_ptr;
    typedef inner_node_con* inner_node_ptr;
public:
    aex_inner_node_con(slot_type _slot_size):base_inner_node(_slot_size){
        node_mutex_array_ptr = new std::shared_mutex[_slot_size / traits::NODE_MUTEX_SLOT_SIZE + (_slot_size % traits::NODE_MUTEX_SLOT_SIZE == 0)];
    }

    ~aex_inner_node_con(){
        if (node_mutex_array_ptr != nullptr)
            delete node_mutex_array_ptr;
    }

    //aex_inner_node_con(aex_inner_node_con &other_node):base_inner_node(other_node){}
    //aex_inner_node_con(aex_inner_node_con &&other_node):base_inner_node(other_node){}
    aex_inner_node_con(aex_inner_node &other_node):base_inner_node(other_node){
        node_mutex_array_ptr = new std::shared_mutex[_slot_size / traits::NODE_MUTEX_SLOT_SIZE + (_slot_size % traits::NODE_MUTEX_SLOT_SIZE == 0)];
    }
    aex_inner_node_con(aex_inner_node &&other_node):base_inner_node(other_node){
        node_mutex_array_ptr = new std::shared_mutex[_slot_size / traits::NODE_MUTEX_SLOT_SIZE + (_slot_size % traits::NODE_MUTEX_SLOT_SIZE == 0)];
    }

    inner_node_con& operator = (inner_node_con &other_node){
        other_node.lock_shared();
        this->mutex.lock();
        *static_cast<base_inner_node_ptr>(this) = static_cast<base_inner_node>(other_node);
        this->mutex.unlock();
        other_node.unlock_shared();
        return this;
    }

    inner_node_con& operator = (inner_node_con &&other_node){
        other_node.lock();
        this->mutex.lock();
        *static_cast<base_inner_node_ptr>(this) = std::move(static_cast<base_inner_node>(other_node));
        if (this->node_mutex_array_ptr != nullptr)
            delete this->node_mutex_array_ptr;
        this->node_mutex_array_ptr = other_node.node_mutex_array_ptr;
        this->mutex.unlock();
        other_node.lock();
        return this;
    }

    inner_node_con& operator = (inner_node &other_node){
        this->mutex.lock();
        *static_cast<base_inner_node_ptr>(this) = static_cast<base_inner_node>(other_node);
        this->mutex.unlock();
    }

    inner_node_con& operator = (inner_node &&other_node){
        this->mutex.lock();
        *static_cast<base_inner_node_ptr>(this) = std::move(static_cast<base_inner_node>(other_node));
        this->mutex.unlock();
        return this;
    }

    inline void lock_array_reader(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].lock_shared();
    }

    inline void unlock_array_reader(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].unlock_shared();
    }

    inline void lock_array_writer(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].lock();
    }

    inline void unlock_array_writer(slot_type lower_slot, slot_type upper_slot){
        slot_type lower_mutex = lower_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level), upper_mutex = upper_slot / (traits::NODE_MUTEX_SLOT_SIZE << this->level);
        for (slot_type i = lower_mutex; i <= upper_mutex; ++i)
            this->node_mutex_array_ptr[i].unlock();
    }

    inline void construct(const key_type* const key, node_ptr* child, const slot_type n){
        this->node_mutex.lock();
        this->base_inner_node::construct(key, child, n);
        this->node_mutex.unlock();
    }


    inline void construct(const key_type* const key, node_ptr* child, const slot_type n, const Model &m){
        this->node_mutex.lock();
        this->base_inner_node::construct(key, child, n, m);
        this->node_mutex.unlock();
    }

    inline void gap_array_construct(const key_type* const key, node_ptr* child, const slot_type n){
        this->node_mutex.lock();
        this->base_inner_node::gap_array_construct(key, child, n);
        this->node_mutex.unlock();
    }

    inline void inplace_construct(slot_type n){
        this->node_mutex.lock();
        this->base_inner_node::inplace_construct(n);
        this->node_mutex.unlock();
    }

    inline pos_type find(const key_type& x) const{
        slot_type ret;
        this->node_mutex.lock_shared();
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            slot_type upper_pos = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size - 1);
            this->mutex.lock_array_reader(pred_pos, upper_pos);
            #ifdef AEX_TLI
            return SearchClass::lower_bound(this->key_ptr, this->key_ptr + this->slot_size, x, this->key_ptr + pred_pos) - this->key_ptr;
            #else
            ret = this->slot_size - 1;
            for (slot_type i = pred_pos; i < this->slot_size; ++i)
            if (x <= key_ptr[i]){
                ret = i;
                break;
            }
            this->mutex.unlock_array_reader(pred_pos, upper_pos);
            #endif
        }
        else{
            ret = this->base_inner_node::find(x);
        }
        this->node_mutex.unlock_shared();
        return ret;
    }

    bool insert(const key_type &key, const node_ptr child){
        bool ret;
        if (!IS_ML_NODE(this)) {
            node_mutex.lock();
            ret = this->base_inner_node::insert(key, child);
            node_mutex.unlock();
        }
        else{
            node_mutex.lock_shared();
            slot_type pred_pos = this->predict(key);
            slot_type upper_pos = std::min(this->slot_size, pred_pos + 2 * traits::ERROR_BOUND);
            this->mutex.lock_array_reader(pred_pos, upper_pos);
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
                    slot_type prev_pos = this->prev_item(inserted_pos);
                    mutex_mutex.lock();
                    this->unlock_array_reader(pred_pos, upper_pos);
                    this->lock_array_writer(prev_pos, i + 1);
                    mutex_mutex.unlock();
                    std::move_backward(this->key_ptr + inserted_pos, this->key_ptr + i, this->key_ptr + i + 1);
                    std::move_backward(this->child_ptr + inserted_pos, this->child_ptr + i, this->child_ptr + i + 1);
                    bitmap_impl::set_one(this->bitmap_ptr, i);
                    std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + inserted_pos + 1, key);
                    std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + inserted_pos + 1, child);
                    ++this->size; 
                    child->parent = this;
                    this->unlock_array_writer(prev_pos, i + 1);
                    node_mutex.unlock_shared();
                    return true;
                }
            }
            ret = false;
            this->mutex.unlock_array_reader(pred_pos, upper_pos);
            node_mutex.unlock_shared();
        }
        return ret;
    }

    bool erase(node_ptr node){
        bool ret;
        node_mutex.lock_shared();
        if (!IS_ML_NODE(node)){
            mutex_mutex.lock();
            node_mutex.unlock_shared();
            node_mutex.lock();
            mutex_mutex.unlock();
            ret = this->base_inner_node::erase(node);
            node_mutex.unlock();
        }
        else{
            pos_type pos = this->at(node);
        }
        return true;
    }

public:
    std::shared_mutex* node_mutex_array_ptr;
};

template<typename _Key,
        typename _Val,
#ifdef AEX_TLI
        typename SearchClass,
#endif
        typename traits>
class aex_static_data_node_con : public aex_static_data_node<_Key, _Val, traits>{
public:

#ifdef AEX_TLI
    typedef aex_static_data_node<_Key, _Val, SearchClass, traits> base_data_node;
    
    typedef aex_static_data_node_con<_Key, _Val, SearchClass, traits> data_node_con;
#else
    typedef aex_static_data_node<_Key, _Val, traits> base_data_node;

    typedef aex_static_data_node_con<_Key, _Val, traits> data_node_con;
#endif

    typedef base_data_node* base_data_node_ptr;

    typedef data_node_con* data_node_con_ptr;

    aex_static_data_node_con(){}

    ~aex_static_data_node_con(){}

    aex_static_data_node_con(aex_static_data_node_con &other_node):base_data_node(other_node){}

    aex_static_data_node_con(aex_static_data_node_con &&other_node):base_data_node(other_node){}


};

}
