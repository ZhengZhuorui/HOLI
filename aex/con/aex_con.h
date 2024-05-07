#pragma once
#include "aex/con/aex_utils_con.h"


namespace aex{
template<typename _Key, 
        typename _Val, 
        typename traits=aex_default_traits<_Key, _Val>>
class aex_tree_con : public aex_tree<_Key, _Val, traits>{
public:
    //static_assert(!traits::AllowMultiKey, "index doesn't support multi key");

    static_assert(traits::AllowConcurrency, "AllowConcurrency must be true");

    typedef _Key key_type;
    
    typedef _Val value_type;

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef aex_tree_con<key_type, _Val, traits> self;

    // type traits
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    // components:
    typedef aex_default_components<traits> components;

    typedef typename components::base_node base_node;

    typedef typename components::base_dynamic_node base_dynamic_node;

    typedef typename components::inner_node inner_node;

    typedef typename components::data_node data_node;

    //typedef typename components::static_data_node static_data_node;

    typedef base_node* node_ptr;

    typedef base_dynamic_node* dynamic_node_ptr;

    // inner_node:    
    typedef inner_node* inner_node_ptr;

    typedef typename components::InnerNodeModel InnerNodeModel;

    typedef data_node* data_node_ptr;
    
    typedef typename components::DataNodeModel DataNodeModel;

    // bitmap:
    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::bitmap bitmap;

    // allocator:
    typedef typename components::Allocator Allocator;

    // balance
    typedef typename components::node_balance_stats node_balance_stats;
    typedef typename components::tree_balance_stats tree_balance_stats;

    aex_tree_con():base_tree(){}

    template<typename _InputIterator>
    aex_tree_con(_InputIterator __first, _InputIterator __last):base_tree(__first, __last){}

    aex_tree_con(const self& _index):base_tree(_index){}

    aex_tree_con(self&& _index):base_tree(_index){}

    ~aex_tree_con(){}

    aex_tree_con& operator = (aex_tree_con &_index){
        std::lock_guard<std::shared_mutex> lk1(_index.tree_mutex);
        std::lock_guard<std::shared_mutex> lk2(this->tree_mutex);
        *static_cast<base_tree*>(this) = static_cast<base_tree>(_index);
        this->change_to_concurrency(this->root);
        return *this;
    }

    aex_tree_con& operator = (aex_tree_con &&_index){
        std::lock_guard<std::shared_mutex> lk1(_index.tree_mutex);
        std::lock_guard<std::shared_mutex> lk2(this->tree_mutex);
        *static_cast<base_tree*>(this) = std::move(static_cast<base_tree>(_index));
        return *this;
    }

    aex_tree_con& operator = (base_tree &_index){
        std::lock_guard<std::shared_mutex> lock(_index.tree_mutex);
        static_cast<base_tree>(this) = _index;
    }

    aex_tree_con& operator = (base_tree &&_index){
        std::lock_guard<std::shared_mutex> lock(_index.tree_mutex);
        static_cast<base_tree>(this) = std::move(_index);
    }

    void clear(){
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        this->base_tree::deconstruct(this->root);
    }

    bool insert(const key_type &key, const value_type &value){
        auto ret = this->base_tree::insert(key, value);
        return ret.second;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
        std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        this->base_tree::bulk_load(data, nums);
    }

    bool find(const key_type &x, value_type &y) {
        std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
        data_node_ptr node = find_leaf_con(x);
        if (node == nullptr)
            return false;
        slot_type pos = node->find_lower_pos(x);
        if (node->key[pos] != x)
            return false;
        y = node->data[pos];
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

    bool erase(const key_type &x){
        //std::lock_guard<std::shared_mutex> lock(this->tree_mutex);
        //bool ret = this->base_tree::erase(x);
        bool ret = this->erase(x);
        return ret;
    }

    inline size_type size(){
        std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
        size_type ret;
        ret = this->base_tree::size();
        return ret;
    }

    inline bool empty() const {
        std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
        bool ret;
        ret = this->base_tree::empty();
        return ret;
    }

    void print_stats(){
        std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
        this->base_tree::print_stats();
    }

#ifndef AEX_DEBUG
protected:

private:    
#endif


    // ========== 1. find ==========
    // Find(x): 
    // S(T)
    // N <- root
    // U(T)
    // while N is not leaf do
    //   C <- find(N, x)
    //   U(N)
    //   S(C)
    //   N <- C

    // Find the data node with the key. The data node will locked shared.
    data_node_ptr find_leaf_con(const key_type &key);

    // Find stack to the data node with the key. The data node will locked.
    data_node_ptr find_leaf_lock_con(const key_type &key, inner_node_ptr *stack, int &top);

    node_ptr find_node_lock_con(const key_type &key, const int level, inner_node_ptr *stack, int &top);

    // ========== 2. insert ==========
    // 

    void insert_split_bulk_load_con(inner_node_ptr* stack, int top, const slot_type start, const key_type* key, node_ptr* child, const slot_type n){
        std::vector<key_type> new_key;
        std::vector<inner_node_ptr> new_child;
        inner_node_ptr node = stack[top - 1];
        this->base_tree::__insert_split_bulk_load(node, start, key, child, n, new_key, new_child);
        if (new_child.size() > 0)
            insert_recursive_con(stack, top - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    }

    void insert_split_by_buffer_con(inner_node_ptr* stack, int top, const key_type* key, node_ptr* child, const slot_type n){
        std::vector<key_type> new_key;
        std::vector<inner_node_ptr> new_child;
        inner_node_ptr node = stack[top - 1], parent = stack[top - 2];
        
        parent->lock();
        this->base_tree::__insert_split_by_buffer(node, new_key, new_child, n, new_key, new_child);
        if (new_child.size() > 0)
            insert_recursive_con(stack, top - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    }

    void insert_split_dense_inner_node(inner_node_ptr* stack, int top, const key_type* key, node_ptr* child, const slot_type n){
        inner_node_ptr node = stack[top - 1];
        std::pair<key_type, node_ptr> ret = this->base_tree::__insert_split_dense_inner_node(node, key, child, n);
        insert_recursive_con(stack, top - 1, &ret.first, &ret.second, 1);
    }

    void insert_split_pipeline_con(inner_node_ptr node, const key_type* key, const node_ptr* child, const slot_type n);

    
    // ========== 3. erase ==========

    // ========== 4. SMO ==========
    //void add_root(){
    //    this->tree_mutex.lock();
    //}

    // ========== 4. concurrency ==========
    inline void SL(node_ptr node){node->node_mutex.lock_shared();}

    inline void SU(node_ptr node){node->node_mutex.unlock_shared();}

    inline void XL(node_ptr node){node->node_mutex.lock();}

    inline void XU(node_ptr node){node->node_mutex.unlock();}

    inline bool TSL(node_ptr node){return node->node_mutex.try_lock_shared();}

    inline bool TXL(node_ptr node){return node->node_mutex.try_lock();}
    inline void TXL(const key_type &key, node_ptr node, inner_node_ptr* stack, int &top){
        //return node->node_mutex.try_lock();
        while(true){
            if (TXL(node)) return;
            else{
                node = this->find_node_lock_con(key, node->level, stack, top);
            }
        }
    }

    inline void SL(){}
    
private:
    std::shared_mutex tree_mutex;
};

}


#include "aex/con/aex_find_con.hpp"
//#include "aex/con/aex_SMO_con.hpp"
//#include "aex/con/aex_insert_con.hpp"
//#include "aex/con/aex_erase_con.hpp"