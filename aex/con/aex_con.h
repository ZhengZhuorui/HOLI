#pragma once

#include "aex/aex.h"
#include "aex/multi_thread/aex_utils_con.h"
#include "aex/multi_thread/aex_node_con.h"

namespace aex{
template<typename _Key, 
        typename _Val, 
#ifdef AEX_TLI
        typename SearchClass,
#endif
        typename traits>
class aex_tree_con : public aex_tree<_Key, _Val, 
                                    #ifdef AEX_TLI
                                    typename SearchClass,
                                    #endif
                                    traits>
{
public:
    static_assert(is_same<traits::AllowMultiKey, std::false_type>::value, "index doesn't support multi key");

    static_assert(is_same<traits::AllowRWBalance, std::false_type>::value, "index doesn't support balance tree in multi thread");
    #ifdef AEX_TLI
    typedef aex_inner_node_con<key_type, value_type, SearchClass, traits> inner_node_con;

    typedef aex_data_node_con<key_type, value_type, SearchClass, traits> data_node_con;

    typedef aex_tree<_Key, _Val, SearchClass, traits> base_tree;

    typedef aex_tree_con<_Key, _Val, SearchClass, traits> self;
    #else 
    typedef aex_inner_node_con<key_type, value_type, traits> inner_node_con;

    typedef aex_data_node_con<key_type, value_type, traits> data_node_con;

    typedef aex_tree<_Key, _Val, traits> base_tree;

    typedef aex_tree_con<_Key, _Val, traits> self;
    #endif

    typedef inner_node_con* inner_node_ptr;

    typedef data_node_con* data_node_ptr;

    typedef aex_components<traits> components;

    typedef components::node_balance_stats node_balance_con_stats;
    typedef components::tree_balance_con_stats tree_balance_con_stats;
    
    aex_tree_con():base_tree(){}

    template<typename _InputIterator>
    aex_tree_con(_InputIterator __first, _InputIterator __last):base_tree(__first, __last){}

    aex_tree_con(const self& _index):base_tree(_index){}

    aex_tree_con(self&& _index):base_tree(_index){}

    ~aex_tree_con(){}

    aex_tree_con& operator = (aex_tree_con &_index){
        std::lock_guard<std::shared_mutex> lock(_index.tree_mutex);
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        *static_cast<base_tree*>(this) = static_cast<base_tree>(_index);
        this->change_to_concurrency(this->root);
        return *this;
    }

    aex_tree_con& operator = (aex_tree_con &&_index){
        std::lock_guard<std::shared_mutex> lock(_index.tree_mutex);
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        *static_cast<base_tree*>(this) = std::move(static_cast<base_tree>(_index));
        return *this;
    }

    aex_tree_con& operator = (aex_tree &_index){
        std::lock_guard<std::shared_mutex> lock(_index.tree_mutex);

    }

    aex_tree_con& operator = (aex_tree &&_index){
        
    }

    

    void clear(){
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        this->base_tree::deconstruct(this->root);
    }

    bool insert(const key_type &key, const value_type &value);

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
        this->base_tree::bulk_load(data, nums);
    }

    bool find(const key_type &x, const value_type &y) {
        std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
        data_node_ptr node = find_leaf_con(x);
        if (node == nullptr || node == end())
            return false;
        pos_type pos = node->find_lower_pos(x);
        if (node->key[pos] != x)
            return false;
        y = node->data[y];
        return true;        
    }

    void range_scan(const key_type &L, const key_type &R, std::vector<std::pair<key_type, value_type>>& answer);

    size_type count(const key_type &x){
        std::false_type fp;
        value_type _;
        if (lookup(x, _) == true) 
            return 0;
        return 1;
    }

    bool exists(const key_type &x) {
        std::false_type fp;
        value_type _;
        if (lookup(x, _) == false) 
            return false;
        return true;
    }

    bool erase(const key_type &x);

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        this->base_tree::bulk_load(data, nums);
        this->change_to_concurrency();
    }

    inline size_type size(){
        std::lock_shared<std::shared_mutex> lock(this->stats_mutex);
        size_type ret;
        ret = this->base_tree::size();
        return ret;
    }

    inline bool empty() const {
        std::lock_shared<std::shared_mutex> lock(this->stats_mutex);
        bool ret;
        ret = this->base_tree::empty();
        return ret;
    }

    void print_stats(){
        std::lock_shared<std::shared_mutex> lock(this->stats_mutex);
        this->base_tree::print_stats();
    }

    inline const aex_stats& get_stats() const{
        std::lock_shared<std::shared_mutex> lock(this->stats_mutex);
        aex_stats stats = this->base_tree::get_stats();
    }

#ifndef AEX_EXPERIMENT
protected:

private:    
#endif

    void insert_split_bulk_load_con(inner_node_ptr node, const slot_type start, const key_type* key, node_ptr* child, const slot_type n){
        std::vector<key_type> new_key;
        std::vector<inner_node_ptr> new_child;
        this->base_tree::__insert_split_bulk_load(node, start, key, child, n, new_key, new_child);
        if (new_child.size() > 0)
            insert_nodes_con(node->parent, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    }

    void insert_split_by_buffer_con(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
        std::vector<key_type> new_key;
        std::vector<inner_node_ptr> new_child;
        this->base_tree::__insert_split_by_buffer(node, new_key, new_child, n, new_key, new_child);
        if (new_child.size() > 0)
            insert_nodes_con(node->parent, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
        for (int i = 0; i < n; ++i)
            new_child->node_mutex->unlock();
    }

    void insert_split_dense_inner_node(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
        std::pair<key_type, node_ptr> ret = this->base_tree::__insert_split_dense_inner_node(node, new_key, new_child, n);
        insert_nodes_con(node->parent, &ret.first, &ret.second, 1);
    }

    void insert_split_pipeline_con(inner_node_ptr node, const key_type* key, const node_ptr* child, const slot_type n);

    

private:
    std::shared_mutex tree_mutex;
    std::mutex mutex_mutex;
    tree_balance_con_stats balance_stats;
};

#include "aex/con/aex_SMO_con.hpp"
#include "aex/con/aex_insert_con.hpp"
#include "aex/con/aex_find_con.hpp"
#include "aex/con/aex_erase_con.hpp"


}