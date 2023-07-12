#pragma once
#include "aex/aex_node.h"
namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_tree;
template<typename _Key, typename _Val, typename traits> class aex_node_allocator;

template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_data_node;

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_con_base{
public:
    aex_spinlock lock;
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
struct aex_inner_node_con : public aex_inner_node<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::pos_type pos_type;

    typedef aex_tree<_Key, _Val, traits> Tree;

    typedef aex_node_allocator_con<_Key, _Val, traits> node_allocator;

    typedef aex_node_base<key_type, value_type, traits> base_node;
    
    typedef base_node* node_ptr;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    typedef piecewise_linear_model<key_type, traits> Model;

    typedef aex_data_node_con<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_inner_node_con<_Key, _Val, traits> inner_node;
    
    typedef inner_node* inner_node_ptr;

    typedef aex_inner_node<_Key, _Val, traits> base_inner_node;

    typedef base_inner_node* base_inner_node;

    typedef aex_read_write_lock rw_lock;
    
    typedef rw_lock* rw_lock_ptr;

    aex_inner_node_con(){}

    ~aex_inner_node_con(){}

    aex_inner_node_con(inner_node &other_node):base_inner_node(other_node){}

    aex_inner_node_con(inner_node &&other_node):base_inner_node(other_node){}

    inline void clear_bitmap(){
        node_lock.lock_writer();
        this->base_inner_node::clear_bitmap();
        node_lock.unlock_writer();
    }

    inline void clear_key_array(){
        node_lock.lock_writer();
        this->base_inner_node::clear_key_array();
        node_lock.unlock_writer();
    }

    inline void inplace_construct(){
        node_lock.lock_writer();
        this->base_inner_node::inplace_construct();
        node_lock.unlock_writer();
    }

    inline void construct(const key_type* const key, const node_ptr* const child, const pos_type n, const Model &m){
        node_lock.lock_writer();
        this->base_inner_node::construct(key, child, n, m);
        node_lock.unlock_writer();
    }

    bool insert(const key_type &key, const node_ptr child){
        if (!(this->prop & node_property::ML_NODE)) {
            node_lock.lock_writer();
            this->base_inner_node::insert(key, child);
            node_lock.unlock_writer();
            return true;
        }
        else{
            node_lock.lock_reader();
            pos_type pred_pos = this->predict(key);
            pos_type inserted_pos = pred_pos;
            for (; inserted_pos < this->slot_size && inserted_pos - pred_pos < traits::ERROR_BOUND; ++inserted_pos)
            if (key <= this->key_ptr[inserted_pos] || !bitmap_impl::at(this->bitmap_ptr, inserted_pos)){
                break;
            }
            if (inserted_pos >= this->slot_size){
                node_lock.unlock_reader();
                return false;
            }
            // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
            if (inserted_pos - pred_pos >= traits::ERROR_BOUND){
                node_lock.unlock_reader();
                return false;
            }

            // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
            pos_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, this->slot_size);
            pos_type start_lock_pos = static_cast<pos_type>(insert_pos / traits::SLOT_PER_LOCK), end_lock_pos = static_cast<pos_type>((max_slot - 1) / traits::SLOT_PER_LOCK);
            for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                rw_lock_array.lock_writer();
            for (pos_type i = inserted_pos; i < max_slot; ++i){
                if (bitmap_impl::at(this->bitmap_ptr, i)){
                    pos_type shift_pos = this->predict(this->key_ptr[i]);
                    if (i + 1 - shift_pos >= traits::ERROR_BOUND){
                        node_lock.unlock_reader();
                        for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                            rw_lock_array.unlock_writer();
                        return false;
                    }
                }
                else{
                    std::move_backward(this->key_ptr + inserted_pos, this->key_ptr + i, this->key_ptr + i + 1);
                    std::move_backward(this->child_ptr + inserted_pos, this->child_ptr + i, this->child_ptr + i + 1);
                    bitmap_impl::set_one(this->bitmap_ptr, i);
                    pos_type prev_pos = this->prev_item(inserted_pos);
                    std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + inserted_pos + 1, key);
                    std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + inserted_pos + 1, child);
                    ++this->size; 
                    this->m_stats.data_size += child->data_size();
                    this->m_stats.data_node += child->data_node_size();
                    node_lock.unlock_reader();
                    for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                        rw_lock_array.unlock_writer();
                    return true;
                }
            }

            // if need shift move more than ERROR_BOUND item, return false

            for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                rw_lock_array.unlock_writer();
            return false;
        }
    }

    bool erase(node_ptr node){
        node_lock.lock_reader();
        pos_type pos = this->at(node);
        if (pos == this->slot_size){
            node_lock.unlock_reader();
            return false;
        }
        node_lock.lock_writer();
        this->m_stats.data_size -= node->data_size();
        this->m_stats.data_node -= node->data_node_size();
        --this->size;
        if (this->prop & node_property::ML_NODE){
            node_lock.unlock_writer();
            pos_type prev_pos = this->prev_item(pos);
            pos_type start_lock_pos = static_cast<pos_type>(prev_pos / traits::SLOT_PER_LOCK), end_lock_pos = static_cast<pos_type>(pos / traits::SLOT_PER_LOCK);
            for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                rw_lock_array.lock_writer();

            bitmap_impl::set_zero(this->bitmap_ptr, pos);
            if (pos < this->slot_size - 1){
                std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + pos + 1, this->key_ptr[pos + 1]);
                std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + pos + 1, this->child_ptr[pos + 1]);
            }

            for (pos_type i = start_lock_pos; i <= end_lock_pos; ++i)
                rw_lock_array.unlock_writer();
        }
        else{
            std::move(this->key_ptr + pos + 1, this->key_ptr + this->size, this->key_ptr + pos);
            std::move(this->child_ptr + pos + 1, this->child_ptr + this->size, this->child_ptr + pos);
            node_lock.unlock_writer();
        }
        return true;
    }

    inline void copy(inner_node_ptr node){
        node->node_lock.lock_reader();
        this->node_lock.lock_writer();
        this->base_inner_node::copy(static_cast<base_inner_node_ptr>(node));
        this->node_lock.unlock_writer();
        node->node_lock.unlock_reader();
    }

public:

    // meta:
    rw_lock node_lock;

    rw_lock_ptr rw_lock_array;
    
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_data_node_con : public aex_data_node<_Key, _Val, traits>{
public:
    typedef aex_data_node_con<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef aex_data_node<_Key, _Val, traits> base_data_node;

    typedef base_data_node* base_data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::pos_type pos_type;

    typedef linear_model<_Key, traits> Model;

    Model model;
    
    key_type __restrict__ *key;

    value_type __restrict__ *data;

    //typedef linear_model<key_type, traits> Model;

    aex_data_node_con(){

    }
    ~aex_data_node_con(){

    }

    void construct(const std::pair<key_type, value_type>* _data, pos_type nums){
        this->node_lock.lock_writer();
        this->base_data_node::construct(_data, nums);
        this->node_lock.unlock_writer();
    }

    void construct(const key_type *_key, const value_type *_data, pos_type nums){
        this->node_lock.lock_writer();
        this->base_data_node::construct(_key, _data, nums);
        this->node_lock.unlock_writer();
    }

    void construct(const key_type *_key, const value_type *_data, pos_type nums, Model &m){
        this->node_lock.lock_writer();
        this->base_data_node::construct(_key, _data, nums, m);
        this->node_lock.unlock_writer();
    }

    // insert a item
    inline pos_type insert(const key_type &x, const value_type &data){
        pos_type pos = this->find_upper_pos(x);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
        return pos;
    }

    // if no item greater than or equal x, return slot_size
    inline pos_type find_lower_pos(const key_type &x){
        pos_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            pos_type pred_pos = this->predict(x);
            pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            pos = std::lower_bound(this->key, this->key + this->size, x) - this->key;
        }
        //AEX_PRINT("key=" << x << ", node key=" << key[pos]);
        return pos;
    }

    inline pos_type find_upper_pos(const key_type &x){
        pos_type pos = this->slot_size;
        if (this->prop & node_property::ML_NODE){
            pos_type pred_pos = this->predict(x);
            pos = aex::exponential_search_upper_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            pos = std::upper_bound(this->key, this->key + this->size, x) - this->key;
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

    inline pos_type data_size(){return this->size;}
    
    inline pos_type data_node_size(){return 1;}
};

}