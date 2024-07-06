#include <cmath>
#pragma once
#include <cstddef>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <queue>
#include <algorithm>

#include "aex/aex_utils.h"
#include "aex/aex_utils_avx.h"
#include "aex/aex_def.h"
#include "aex/aex_traits.h"
#include "aex/aex_components.h"
#include "aex/aex_hash_table.h"
#include "aex/aex_model.h"
#include "aex/aex_model_avx.h"
#include "aex/aex_node.h"
#include "aex/con/aex_node_con.h"
#include "aex/aex_allocator.h"
#include "aex/con/aex_allocator_con.h"
#include "aex/aex_iterator.h"

//template<typename _Tp>
//struct aex_node_balance_stats;
//
//template<typename _Tp>
//struct aex_tree_balance_stats;

namespace aex{

template<typename _Key, 
        typename _Val,
        typename traits=aex_default_traits<_Key, _Val>>
class aex_tree{
public:

    //static_assert(!traits::AllowMultiKey, "index doesn't support multi key");

    static_assert(std::is_arithmetic<_Key>::value, "key types must be numeric.");

    // type traits
    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    // components:
    typedef aex_default_components<traits> components;

    typedef aex_tree<_Key, _Val, traits> self;

    typedef typename components::base_node base_node;

    typedef typename components::base_dynamic_node base_dynamic_node;

    typedef typename components::inner_node inner_node;

    typedef typename components::data_node data_node;

    // iterator:
    typedef aex_iterator<_Key, _Val, traits> iterator;

    typedef aex_const_iterator<_Key, _Val, traits> const_iterator;

    typedef aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;

    typedef aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    typedef base_node* node_ptr;

    typedef base_dynamic_node* dynamic_node_ptr;

    // inner_node:    
    typedef inner_node* inner_node_ptr;

    typedef typename components::InnerNodeModel InnerNodeModel;

    // data_node:
    typedef data_node* data_node_ptr;
    
    typedef typename components::DataNodeModel DataNodeModel;

    // bitmap:
    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::bitmap bitmap;
    typedef typename traits::bitmap_base bitmap_base;

    // allocator:
    typedef typename components::Allocator Allocator;

    // balance
    typedef typename components::node_balance_stats node_balance_stats;
    typedef typename components::tree_balance_stats tree_balance_stats;

    struct aex_stats{
        size_type level_node[traits::MAX_DEPTH];
        size_type size;
        unsigned int height;
        key_type max_key, min_key;
        aex_stats():size(0), height(0){
            memset(level_node, 0, sizeof(size_type) * traits::MAX_DEPTH);
            max_key = std::numeric_limits<key_type>::lowest();
            min_key = std::numeric_limits<key_type>::max();
        }
        inline size_type inner_node(){
            size_t inner_node_size = 0;
            for (int i = 1; i < traits::MAX_DEPTH; ++i)
                inner_node_size += level_node[i];
            return inner_node_size;
            //return (height > 1) ? std::reduce(level_node + 1, level_node + height) : 0;
        }
        inline size_type data_node(){
            return level_node[0];
        }
    };

    #ifdef AEX_DEBUG
    struct operation_stats{
        operation_stats():inner_node_split_cnt(0), inner_node_merge_cnt(0), inner_node_rescale_cnt(0),
                        inner_node_split_pipeline_cnt(0), inner_node_split_bulk_load_cnt(0), inner_node_split_by_buffer_cnt(0), inner_node_split_dense_node_cnt(0),
                        data_node_split_cnt(0), data_node_merge_cnt(0), data_node_rescale_cnt(0),
                        inner_node_train_cnt(0), inner_node_train_tot_size(0),
                        inner_node_balance_split_cnt(0), inner_node_balance_check_split_cnt(0), inner_node_insert_balance_cnt(0),
                        inner_node_split_left_buffer_cnt(0), inner_node_split_right_buffer_cnt(0),
                        inner_node_lsm_merge_try_cnt(0), inner_node_lsm_merge_cnt(0),
                        inner_node_split_hotspot_cnt(0){}
        size_type inner_node_split_cnt, inner_node_merge_cnt, inner_node_rescale_cnt;
        size_type inner_node_split_pipeline_cnt, inner_node_split_bulk_load_cnt, inner_node_split_by_buffer_cnt, inner_node_split_dense_node_cnt; 
        size_type data_node_split_cnt, data_node_merge_cnt, data_node_rescale_cnt;
        size_type inner_node_train_cnt, inner_node_train_tot_size;
        size_type inner_node_balance_split_cnt, inner_node_balance_check_split_cnt, inner_node_insert_balance_cnt;
        size_type inner_node_split_left_buffer_cnt, inner_node_split_right_buffer_cnt;
        size_type inner_node_lsm_merge_try_cnt, inner_node_lsm_merge_cnt;
        size_type inner_node_split_hotspot_cnt;
        void print_stats(){
            AEX_PRINT("[Operation status] inner node: split times=" << inner_node_split_cnt << ", merge times=" << inner_node_merge_cnt <<
                    ", rescale times=" << inner_node_rescale_cnt);
            AEX_PRINT("inner node: split pipeline times=" << inner_node_split_pipeline_cnt << ", bulk_load cnt=" << inner_node_split_bulk_load_cnt << ", by_buffer cnt=" << inner_node_split_by_buffer_cnt << ", split dense node cnt=" << inner_node_split_dense_node_cnt
                    << " left_buffer cnt=" << inner_node_split_left_buffer_cnt << ", right_buffer cnt=" << inner_node_split_right_buffer_cnt);
            AEX_PRINT("inner node: train cnt=" << inner_node_train_cnt << ", train size=" << inner_node_train_tot_size);
            AEX_PRINT("inner_node_lsm_merge_try_cnt = " << inner_node_lsm_merge_try_cnt << ", inner_node_lsm_merge_cnt = " << inner_node_lsm_merge_cnt << ", ratio=" << 1.0 * inner_node_lsm_merge_cnt / inner_node_lsm_merge_try_cnt);
            AEX_PRINT("inner_node_split_hotspot_cnt = " << inner_node_split_hotspot_cnt);
            AEX_PRINT(" data node: split times=" << data_node_split_cnt << ", merge times=" << data_node_merge_cnt <<
                    ", rescale times=" << data_node_rescale_cnt);   
            AEX_PRINT("[balance status] inner node balance split cnt=" << inner_node_balance_split_cnt << ", inner node balance check split cnt=" << inner_node_balance_check_split_cnt << 
                    ", balance split ratio=" << 1.0 * inner_node_balance_split_cnt / (inner_node_balance_check_split_cnt + 1) << ", inner_node_insert_balance_cnt=" << inner_node_insert_balance_cnt);
        }
    }opt_stats;
    #endif

    //#ifdef AEX_DEBUG
    static int debug_level; 
    //#endif

#ifndef AEX_DEBUG
private:
#endif

    node_ptr root;

    data_node_ptr head_leaf;

    data_node_ptr tail_leaf;

    aex_stats m_stats;

    tree_balance_stats balance_stats;

    Allocator allocator;

    data_node_ptr empty_leaf;

    static double inner_node_few_ratio[traits::MAX_DEPTH], inner_node_full_ratio[traits::MAX_DEPTH];

    static int log_inner_node_few_ratio[traits::MAX_DEPTH], log_inner_node_full_ratio[traits::MAX_DEPTH];

    size_type max_inner_node_slot_size[traits::MAX_DEPTH];

    inner_node_ptr tree_stack[traits::MAX_DEPTH + 1];

    typename std::false_type fp;

    //constexpr static double lambda = 1 - 1.0 / traits::LAMBDA_;

public:

    aex_tree();

    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);

    aex_tree(const self& _index);

    aex_tree(self&& _index);

    ~aex_tree();

    aex_tree& operator = (aex_tree &_index){
        this->init();
        this->root = this->construct(_index.root);
        this->link_tree_ptr();
        this->m_stats = _index.m_stats;
        this->balance_stats = _index.balance_stats;
        //this->allocator = _index.allocator;
        return *this;
    }

    aex_tree& operator = (aex_tree &&_index){
        this->init();
        this->root = _index.root;
        _index.root = nullptr;
        this->head_leaf = _index.head_leaf;
        _index.head_leaf = nullptr;
        this->tail_leaf = _index.tail_leaf;
        _index.tail_leaf = nullptr;

        this->m_stats = _index.m_stats;
        _index.m_stats = aex_stats();
        this->balance_stats = _index.balance_stats;
        _index.m_stats = balance_stats();
        this->allocator = _index.allocator;
        //_index.allocator = allocator();
        return *this;
    }

    void clear(){
        this->deconstruct(this->root);
        this->m_stats = aex_stats();
        this->root = this->head_leaf = this->tail_leaf = nullptr;
    }


    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    
    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    inline const_iterator find(const key_type &x) const{
        this->balance_stats.update_timestamp();
        iterator it = find_iterator(x);
        if (it == end() || it.key() != x) 
            return end();
        return it;
    }

    inline iterator find(const key_type &x){
        this->balance_stats.update_timestamp();
        iterator it = find_iterator(x);
        if (it == end() || it.key() != x) 
            return end();
        return it;
    }

    inline void find(const key_type &x, value_type &y)const{
        iterator iter = this->find(x);
        if (iter == end() || iter.key() != x) 
            return;
        y = iter.data();
    }

    void range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer)const{
        this->balance_stats.update_timestamp();
        iterator iter = this->find_iterator(lower_key);
        while(iter.key() <= upper_key){
            answer.push_back(std::make_pair(iter->key(), iter->value()));
            iter++;
        }
    }

    size_type count(const key_type &x) const {
        this->balance_stats.update_timestamp();
        iterator it = find(x);
        if (it.key() != x) 
            return 0;
        return 1;
    }

    bool exists(const key_type &x) {
        this->balance_stats.update_timestamp();
        iterator it = find(x);
        if (it.key() != x) return false;
        return true;
    }

    const_iterator lower_bound(const key_type &x){
        this->balance_stats.update_timestamp();
        return find_iterator(x);
    }

    const_iterator upper_bound(const key_type &x){
        this->balance_stats.update_timestamp();
        iterator iter = find_iterator(x);
        while (iter.key() <= x) 
            ++iter;
        return iter;
    }

    /* erase one key*/
    size_t erase(const key_type &x){
        if (root == nullptr) return 0;
        size_type cnt = 0;
        while (true){
            if (erase_one(x)) ++cnt;
            else break;
        }
        return cnt;
    }

    bool erase_one(const key_type &x);

    inline void erase(const_iterator &iter){
        if (root == nullptr || iter == end()) 
            return end();
        erase_iterator(iter);
    }

    inline iterator begin() {
        return iterator(head_leaf, 0);
    }

    inline const_iterator begin() const {
        return const_iterator(head_leaf, 0);
    }

    inline iterator end() {
        //return iterator(tail_leaf, tail_leaf->size);
        return iterator(empty_leaf, 0);
    }

    inline const_iterator end() const {
        return const_iterator(empty_leaf, 0);
    }

    inline reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    inline const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    inline reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    inline const_reverse_iterator rend() const {
        return reverse_iterator(begin());
    }

    inline size_type size() const {
        return static_cast<size_t>(m_stats.size);
    }

    inline bool empty() const {
        return m_stats.size == 0;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums);

    inline const aex_stats& get_stats() const{
        return m_stats;
    }

    void print_stats(){
        AEX_IMPORTANT("data size=" << m_stats.size << ", tree height=" << m_stats.height << ", data node size=" << m_stats.data_node() \
                    << ", inner node size=" << m_stats.inner_node());
        #ifdef AEX_DEBUG
        allocator.print_stats();
        opt_stats.print_stats();
        balance_stats.print_stats();
        #endif
    }

    static void print_nodes(node_ptr node){
        AEX_PRINT("node=" << node << "IS_LEAF_NODE?: " << IS_LEAF_NODE(node) << ", IS_ML_NODE?: " << IS_ML_NODE(node));
        if (IS_LEAF_NODE(node)){
            data_node_ptr _node = static_cast<data_node_ptr>(node);
            for (slot_type i = 0; i < _node->size; ++i)
                std::cout << "(" << _node->key[i] << ", " << _node->data[i] << "), ";
            std::cout << std::endl;
        }
        else{
            inner_node_ptr _node = static_cast<inner_node_ptr>(node);
            if (IS_ML_NODE(node)){
                for (slot_type i = 0; i < _node->slot_size; ++i)
                if (bitmap_impl::at(_node->bitmap_ptr, i))
                    std::cout << "(" << i << ", " << _node->key_ptr[i] << ", " << _node->child_ptr[i] << "), ";
                std::cout << std::endl;
            }
            else{
                for (slot_type i = 0; i < _node->size; ++i)
                    std::cout << "(" << _node->key_ptr[i] << ", " << _node->child_ptr[i] << "), ";
                std::cout << std::endl;
            }
        }
    }

    inline size_type memory_used() const{
        if (this->root == nullptr)
            return 0;
        size_t memory_used = 0;
        std::queue<node_ptr> que;
        que.push(this->root);
        while (!que.empty()){
            node_ptr now = que.front();
            que.pop();
            if (!IS_LEAF_NODE(now)){
                inner_node_ptr in_now = static_cast<inner_node_ptr>(now);
                //memory_used += Allocator::INNER_NODE_MEMORY_USED(now->slot_size);
                memory_used += sizeof(inner_node) + 
                                (in_now->key_ptr != nullptr) * Allocator::KEY_MEMORY_USED(in_now->slot_size) + 
                                (in_now->child_ptr != nullptr) * Allocator::PTR_MEMORY_USED(in_now->slot_size) + 
                                (in_now->bitmap_ptr != nullptr) * Allocator::BITMAP_MEMORY_USED(in_now->slot_size);
                if (IS_ML_NODE(in_now)){
                    for (slot_type i = 0; i < in_now->slot_size; ++i)
                    if (bitmap_impl::at(in_now->bitmap_ptr, i))
                        que.push(in_now->child_ptr[i]);
                }
                else{
                    for (slot_type i = 0; i < in_now->size; ++i)
                        que.push(in_now->child_ptr[i]);
                }
            }
            else{
                memory_used += Allocator::STATIC_DATA_NODE_MEMORY_USED();
            }
        }
        return memory_used;
    }

#ifndef AEX_DEBUG
protected:


private:    
#endif
     
    node_ptr construct(node_ptr node);

    void link_tree_ptr();

    inline void deconstruct(node_ptr node){
        erase_tree_recursive(node);
    }

    // ========== 0. utils ==========

    // ========== 1. find ==========

    // if no item greater than or equal to x, return end()
    inline const_iterator find_iterator(const key_type &x) const{
        data_node_ptr node = find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos == node->size)
            return end(); 
        return const_iterator(node, pos);
    }

    inline iterator find_iterator(const key_type &x){
        data_node_ptr node = find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos == node->size)
            return end(); 
        return iterator(node, pos);
    }

    // if no item greater than x, return NULL
    inline const_iterator find_upper(const data_node_ptr node, const key_type &x) const{
        slot_type pos = node->find_upper_pos(x);
        if (pos == node->size)
            return end();
        return const_iterator(node, pos);
    }

    inline iterator find_upper(const data_node_ptr node, const key_type &x) {
        slot_type pos = node->find_upper_pos(x);
        if (pos == node->size)
            return end();
        return iterator(node, pos);
    }

    inline data_node_ptr find_leaf(const key_type &key) const{
        node_ptr node = root;
        for (unsigned int level = this->m_stats.height - 1; level > 0; --level){
            node = static_cast<inner_node_ptr>(node)->find(key);
        }
        return static_cast<data_node_ptr>(node);
    }

    inline data_node_ptr find_leaf_with_stack(const key_type &key, inner_node_ptr* &stack){
        node_ptr node = root;
        AEX_ASSERT(this->m_stats.height == root->level + 1);
        int top = 0;
        for (unsigned int level = this->m_stats.height - 1; level > 0; --level){
            this->tree_stack[++top] = static_cast<inner_node_ptr>(node);
            node = static_cast<inner_node_ptr>(node)->find(key);
        }        
        stack = this->tree_stack + top;
        return static_cast<data_node_ptr>(node);
    }

    // ========== 2. balance tree ==========
    bool insert_merge(inner_node_ptr parent, const key_type &new_key, node_ptr new_node);

    bool check_split(data_node_ptr node, bool is_forced=false);

    int check_split(inner_node_ptr node);

    slot_type check_split_size(inner_node_ptr node);

    void update_node_list_frequency(node_balance_stats &stats, const slot_type size, node_ptr* node_list, slot_type n);

    // ========== 3. insert ==========

    // insert an item(<key, data>) to data node
    // please use node->insert(key, data);
    // size_type insert_data(data_node_ptr node, const key_type &key, const value_type &data);

    // try to insert an node to inner node. If no position to insert, return false
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::false_type allow_rw_balance);
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::true_type allow_rw_balance);

    // A part of bulk load.
    void build_tree(key_type* key, data_node_ptr* child, size_type n);

    // a part of function "insert_split_pipeline". If node can't insert to parent pipeline, then split and insert to parent together.
    // start means the number of function "insert_split_pipeline" split. key and child is the splited key and child. half_flag means 
    //void __insert_split_bulk_load(inner_node_ptr node, const slot_type start, const key_type key, inner_node_ptr child, bool half_flag, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child);
    void __insert_split_bulk_load(const key_type* key_buf, node_ptr* child_buf, const slot_type size, const slot_type tail, const int split_size, key_type last_key, inner_node_ptr last_node, node_balance_stats &stats, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child);

    void insert_split_bulk_load(inner_node_ptr* stack, key_type* key_buf, node_ptr* child_buf, const slot_type size, const slot_type tail, const key_type last_key, inner_node_ptr last_node, int split_size, node_balance_stats &stats){
        //AEX_HINT("[bulk_load]");
        AEX_ASSERT(*stack != nullptr);
        std::vector<key_type>& new_key = allocator.allocate_dynamic_key_buf(last_node->level & 1);
        std::vector<inner_node_ptr>& new_child = reinterpret_cast<std::vector<inner_node_ptr>&>(allocator.allocate_dynamic_nodeptr_buf(last_node->level & 1));
        //std::vector<key_type> new_key;
        //std::vector<inner_node_ptr> new_child;
        __insert_split_bulk_load(key_buf, child_buf, size, tail, split_size, last_key, last_node, stats, new_key, new_child);
        this->allocator.deallocate_key_buffer(key_buf);
        this->allocator.deallocate_nodeptr_buffer(child_buf);
        if (new_child.size() > 0)
            insert_recursive(stack - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    }

    void insert_split_left_buffer_by_buffer(inner_node_ptr* stack, key_type* key, node_ptr* child, size_type n){
        inner_node_ptr &node = *stack;
        std::vector<key_type>& new_key = allocator.allocate_dynamic_key_buf(node->level & 1);
        std::vector<inner_node_ptr>& new_child = reinterpret_cast<std::vector<inner_node_ptr>&>(allocator.allocate_dynamic_nodeptr_buf(node->level & 1));
        split(key, child, n, node->level, new_key, new_child);
        size_type m = new_child.size();
        for (size_type i = 0; i < m; ++i){
            new_child[i]->next = new_child[i + 1];
            new_child[i + 1]->prev = new_child[i];
        }
        
        new_child[0]->prev = node->prev;
        new_child[m - 1]->next = node;
        if (node->prev != nullptr)
            node->prev->next = new_child[0];
        node->prev = new_child[m - 1];

        allocator.deallocate_key_buffer(key);
        allocator.deallocate_nodeptr_buffer(child);
        update_node_list_frequency(node->balance_stats, node->size, reinterpret_cast<node_ptr*>(new_child.data()), m);
        insert_recursive(stack - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
        return;
    }

    void insert_split_right_buffer_by_buffer(inner_node_ptr* stack, key_type* key, node_ptr* child, size_type n, const key_type split_key){
        AEX_PRINT("[insert_split_right_buffer_by_buffer]");
        inner_node_ptr &node = *stack;
        std::vector<key_type>& new_key = allocator.allocate_dynamic_key_buf(node->level & 1);
        std::vector<inner_node_ptr>& new_child = reinterpret_cast<std::vector<inner_node_ptr>&>(allocator.allocate_dynamic_nodeptr_buf(node->level & 1));
        split(key, child, n, node->level, new_key, new_child);
        size_type m = new_child.size();
        node_ptr prev_node = node->prev, next_node = node->next;
        
        update_node_list_frequency(node->balance_stats, node->size, reinterpret_cast<node_ptr*>(new_child.data()), m);
        inner_node_ptr tmp = new_child[m - 1];
        std::swap(*node, *new_child[m - 1]);
        std::move_backward(new_child.data(), new_child.data() + m - 1, new_child.data() + m);
        new_child[0] = tmp;
        new_key.insert(new_key.begin(), split_key);

        for (size_type i = 0; i < m; ++i){
            new_child[i]->next = new_child[i + 1];
            new_child[i + 1]->prev = new_child[i];
        }
        if (prev_node != nullptr)
            prev_node->next = new_child[0];
        if (next_node != nullptr)
            next_node->prev = new_child[m - 1];
        new_child[0]->prev = prev_node;
        new_child[m - 1]->next = node;
        node->prev = new_child[m - 1];
        node->next = next_node;

        allocator.deallocate_key_buffer(key);
        allocator.deallocate_nodeptr_buffer(child);
        insert_recursive(stack - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
        return;
    }

    void insert_split_left_buffer(inner_node_ptr* stack, const key_type* key, const node_ptr* child, const slot_type n);

    void insert_split_right_buffer(inner_node_ptr* stack, const key_type* key, const node_ptr* child, const slot_type n);
    
    // insert child to node and split it, then insert them to node->parent pipeline
    void insert_split_pipeline(inner_node_ptr* stack, const key_type* key, const node_ptr* child, const slot_type n);

    // insert key and node with buffer.
    void __insert_split_by_buffer(inner_node_ptr node, const key_type* key, node_ptr* child, const slot_type n, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child);

    void insert_split_by_buffer(inner_node_ptr* stack, const key_type* key, node_ptr* child, const slot_type n){
        AEX_ASSERT(*stack != nullptr);
        //AEX_PRINT("by_buffer");
        //std::vector<key_type> new_key;
        //std::vector<inner_node_ptr> new_child;
        inner_node_ptr &node = *stack;
        std::vector<key_type>& new_key = allocator.allocate_dynamic_key_buf(node->level & 1);
        std::vector<inner_node_ptr>& new_child = reinterpret_cast<std::vector<inner_node_ptr>&>(allocator.allocate_dynamic_nodeptr_buf(node->level & 1));
        __insert_split_by_buffer(node, key, child, n, new_key, new_child);
        if (new_child.size() > 0)
            insert_recursive(stack - 1, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    }

    // insert a new key and new child to a dense node (not ml node), then split it.
    std::pair<key_type, node_ptr> __insert_split_dense_inner_node(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n);

    void insert_split_dense_inner_node(inner_node_ptr* stack, const key_type* new_key, node_ptr* new_child, const slot_type n){
        AEX_ASSERT(*stack != nullptr);
        //AEX_PRINT("split_dense_inner_node");
        key_type split_key;
        node_ptr split_node;
        std::tie(split_key, split_node) = __insert_split_dense_inner_node(*stack, new_key, new_child, n);
        insert_recursive(stack - 1, &split_key, &split_node, 1);
    }

    //void insert_split_hotspot(inner_node_ptr* stack, const key_type* key, node_ptr* child, const slot_type n){
    //    inner_node_ptr &node = *stack;
    //    slot_type tot_size = node->size + n;
    //    key_type* key_buf = allocator.allocate_key_buffer(tot_size);
    //    node_ptr* child_buf = allocator.allocate_nodeptr_buffer(tot_size);
    //    std::vector<key_type>& new_key = allocator.allocate_dynamic_key_buf(node->level & 1);
    //    std::vector<inner_node_ptr>& new_child = reinterpret_cast<std::vector<inner_node_ptr>&>(allocator.allocate_dynamic_nodeptr_buf(node->level & 1));
    //    copy_to_buffer(node, key_buf, child_buf);
    //    slot_type pos = std::lower_bound(key_buf, key_buf + node->size - 1, key[0]) - key_buf;
    //    slot_type L = pos, R = pos;
    //    if (pos <= traits::MIN_INNER_NODE_SLOT_SIZE / 2) 
    //        L = R = traits::MIN_INNER_NODE_SLOT_SIZE / 2;
    //    if (pos >= node->size - traits::MIN_INNER_NODE_SLOT_SIZE / 2)
    //        L = R = node->size - traits::MIN_INNER_NODE_SLOT_SIZE / 2;
    //    while (L > traits::MIN_INNER_NODE_SLOT_SIZE && (node->predict(key[0]) - node->predict(key_buf[L])) < traits::ERROR_BOUND)
    //        --L;
    //    while (R < node->size - traits::MIN_INNER_NODE_SLOT_SIZE / 2 && (node->predict(key_buf[R]) - node->predict(key[n - 1])) < traits::ERROR_BOUND)
    //        ++R;
    //    R += n;
    //    if (R - L < traits::ERROR_BOUND){
    //        AEX_WARNING("split hotspot warning!");
    //        L = std::min(traits::MIN_INNER_NODE_SLOT_SIZE / 2, R - traits::ERROR_BOUND);
    //        R = std::max(tot_size - traits::MIN_INNER_NODE_SLOT_SIZE / 2, L + traits::ERROR_BOUND);
    //    }
    //    //AEX_ASSERT(R - L < traits::ERROR_BOUND);
    //    std::move_backward(key_buf + pos, key_buf + node->size - 1, key_buf + node->size - 1 + n);
    //    std::move_backward(child_buf + pos, child_buf + node->size, child_buf + node->size + n);
    //    split(key_buf, child_buf, L, node->level, new_key, new_child);
    //    new_key.push_back(key_buf[L]);
    //    split(key_buf + L, child_buf + L, R - L, node->level, new_key, new_child);
    //    new_key.push_back(key_buf[L]);
    //    allocator.deallocate_key_buffer(key_buf);
    //    allocator.deallocate_nodeptr_buffer(key_buf);
    //    insert_recursive(stack - 1, &split_key, &split_node, 1);
    //}
    
    // insert some nodes to an inner node from bottom to up.
    void insert_recursive(inner_node_ptr* stack, const key_type* key_buf, node_ptr* child_buf, const slot_type n);

    // a helper function insert some child to a inner node.
    void insert_split_helper(inner_node_ptr* stack, const key_type* key, node_ptr* new_child, const slot_type n, const NODE_INSERT_CODE error_code=NODE_INSERT_CODE::NONE);
    
    // insert some data to a dynamic data node.
    void insert_split(data_node_ptr node, const key_type key, const value_type data);
    
    // ========== 4. erase ==========

    // erase subtree of the node.
    void erase_tree_recursive(node_ptr node);

    // erase an node from bottom to up
    void erase_recursive(inner_node_ptr* stack);

    void _erase(inner_node_ptr* stack, data_node_ptr node);

    // erase one iterator
    void erase_iterator(const_iterator &iter);

    // erase one child node from parent. return false if parent or child not exists
    // free the node
    void erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node);
    //void erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node, slot_type left_node_pos);

    // erase one item(iterator) from data_node
    void erase_data(iterator &iter);
    //bool erase_merge(inner_node_ptr __restrict__ parent, static_data_node_ptr __restrict__ left_node, static_data_node_ptr __restrict__ right_node);

    inline void erase_link(node_ptr node){
        if (node->prev != nullptr) 
            node->prev->next = node->next;
        if (node->next != nullptr) 
            node->next->prev = node->prev;
    }

    // ========== 4. Structure Modify Operation(SMO) ==========
    
    // split a inner node to inner nodes
    // if split to one inner node, it is equal trains the origin node
    void split(inner_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a data node to at least two data node
    void split(data_node_ptr node, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    // split a ordered key array with child pointers array to inner node array.
    void split(const key_type* const key, node_ptr* child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child);
    void split(const key_type* const key, const size_type n, const unsigned int level);

    //void split_left_buffer(inner_node_ptr node, const key_type* const key, node_ptr* child, unsigned int n){
    //    
    //}
    //void split_right_buffer(inner_node_ptr node, const key_type* const key, node_ptr* child, unsigned int n);

    // split a ordered key array with data array to node array.
    void split_to_static_data_node(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    void split_to_static_data_node_with_gap(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    // use exponential probe to split a ordered key array with data array to node array.
    void split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    // use exponential probe to split a ordered key array with data array to node array.
    void split_with_linear_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    // a part of split_with_linear_probe.
    slot_type linear_probe(const key_type* const key, const size_type n, DataNodeModel &m);

    void merge(inner_node_ptr __restrict__ parent, inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node, std::false_type erase);
    bool merge(inner_node_ptr __restrict__ parent, inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node, std::true_type insert);
    bool merge(inner_node_ptr __restrict__ parent, data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    // split a ordered key array with data array to inner node array. Use linear probe(use greedy).
    // return: <size, slot_size, ML_flag>
    std::tuple<slot_type, slot_type, bool> split_with_exponential_probe(const key_type* const key, const size_type n, const unsigned int level);
    std::tuple<slot_type, slot_type, bool> split_with_exponential_probe_reverse(const key_type* const key, const size_type n, const unsigned int level);
    //std::tuple<slot_type, slot_type, bool> split_with_linear_probe(const key_type* const key, const size_type n, const unsigned int level);

    inline void loop_merge_left(inner_node_ptr __restrict__ &parent, inner_node_ptr __restrict__ &node){
        if constexpr (!traits::AllowMergeNode)
            return;
        std::true_type tp;
        while (node->prev != nullptr && static_cast<inner_node_ptr>(parent->child_ptr[0]) != node){
            if (!CAN_RIGHT_MERGED_NODE(node->prev) && !CAN_LEFT_MERGED_NODE(node))
                return;
            if (static_cast<inner_node_ptr>(node->prev)->slot_size != node->slot_size)
                return;
            //AEX_HINT("loop_merge_left");
            if (merge(parent, static_cast<inner_node_ptr>(node->prev), node, tp) == false)
                return;
        }
    }

    inline void loop_merge_right(inner_node_ptr __restrict__ &parent, inner_node_ptr __restrict__ &node){
        if constexpr (!traits::AllowMergeNode)
            return;
        std::true_type tp;
        while (node->next != nullptr && static_cast<inner_node_ptr>(parent->child_ptr[parent->last()]) != node){
            inner_node_ptr next_node = static_cast<inner_node_ptr>(node->next);
            if (!CAN_RIGHT_MERGED_NODE(node) && !CAN_LEFT_MERGED_NODE(next_node))
                return;
            if (next_node->slot_size != node->slot_size)
                return;
            //AEX_HINT("loop_merge_right");
            if (merge(parent, node, next_node, tp) == false)
                return;
            node = next_node;
        }
    }

    key_type split_dense_inner_node(inner_node_ptr new_node, inner_node_ptr old_node);
    void split(data_node_ptr new_node, data_node_ptr old_node);
    void split_reverse(data_node_ptr new_node, data_node_ptr old_node);

    // check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
    template<typename Model>
    static bool check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, Model &m);

    template<typename HashModel>
    bool check_collision_hash_table(const key_type* const key, const slot_type size, const slot_type slot_size, HashModel &m);

    // Rescale a node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
    // if node expand or narrow successed, the old node will free and return true. Otherwise return false.
    bool rescale(inner_node_ptr node, const slot_type new_slot_size);
    bool rescale_implement(inner_node_ptr node, const slot_type new_slot_size);
    bool rescale(data_node_ptr node, const slot_type new_slot_size);
    bool rescale(node_ptr node, const slot_type new_slot_size);

    bool expand(inner_node_ptr node){
        AEX_HINT("expand");
        bool ret;
        if (!IS_ML_NODE(node)){
            if (node->size >= traits::MIN_ML_INNER_NODE_SIZE){
                slot_type new_slot_size = min_slot_size(node->size, self::inner_node_few_ratio[node->level], traits::MIN_INNER_NODE_SLOT_SIZE);
                if (new_slot_size * self::inner_node_few_ratio[node->level] > node->size) new_slot_size >>= 1;
                ret = rescale(node, new_slot_size);
            }
            else
                ret = rescale(node, node->real_slot_size() << 1);
        }
        else{
            ret = rescale(node, node->real_slot_size() << 1);
        }
        if constexpr (traits::AllowInsertBalance)
            if (ret == true)
                SET_FLAG(node, CAN_MERGED);
        return ret;
    }

    bool narrow(inner_node_ptr node){
        bool ret;
        if (!IS_ML_NODE(node)){
            ret = rescale(node, node->real_slot_size() >> 1);
        }
        else{
            if (node->size < traits::MIN_ML_INNER_NODE_SIZE){
                slot_type new_slot_size = min_slot_size(node->size, traits::MIN_INNER_NODE_SLOT_SIZE);
                ret = rescale(node, new_slot_size);
            }
            else{
                ret = rescale(node, node->real_slot_size() >> 1);
            }
        }
        if constexpr (traits::AllowInsertBalance)
            if (ret)
                SET_FLAG(node, CAN_MERGED);
        return ret;
    }

    // link node_buf parent, prev and next pointer, the prev point of first node is node->prev, next too.
    void link_node_list_and_replace_last_node(node_ptr node, node_ptr* new_child, slot_type m);

    void add_root(const key_type* key_buf, node_ptr* child_buf, slot_type n);

    inline void link_to_next_node(node_ptr new_node, node_ptr old_node){
        new_node->prev = old_node->prev;
        if (old_node->prev != nullptr)
            old_node->prev->next = new_node;
        old_node->prev = new_node;
        new_node->next = old_node;
    }

    bool retrain(inner_node_ptr node);

    inline bool isfull(const data_node_ptr node) const {
        if constexpr (traits::AllowDynamicDataNode)
            return node->size >= node->slot_size;
        else
            return node->size >= traits::MIN_DATA_NODE_SLOT_SIZE;
    }

    inline bool isfew(const data_node_ptr node) const {
        if constexpr (traits::AllowDynamicDataNode)
            return node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO;
        else
            return node->size < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfew(const data_node_ptr node, const slot_type offset) const {
        if constexpr (traits::AllowDynamicDataNode)
            return (node->size + offset) < node->slot_size * traits::DATA_NODE_FEW_RATIO;
        else
            return (node->size + offset) < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return node->size >= node->real_slot_size() >> ((IS_ML_NODE(node) ? self::log_inner_node_full_ratio[node->level] : traits::LOG_DATA_NODE_FULL_RATIO));
    }

    inline bool isfull(const inner_node_ptr node, const slot_type offset) const {
        return node->size + offset >= node->real_slot_size() >> ((IS_ML_NODE(node) ? self::log_inner_node_full_ratio[node->level] : traits::LOG_DATA_NODE_FULL_RATIO));
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return node->size < node->real_slot_size() >> (((IS_ML_NODE(node) ? self::log_inner_node_full_ratio[node->level] : traits::LOG_DATA_NODE_FEW_RATIO)) + 1);
        //return node->size < node->real_slot_size * ((IS_ML_NODE(node) ? self::inner_node_few_ratio[node->level] : traits::DATA_NODE_FEW_RATIO)) * traits::DENSITY_NARROW_RATIO;
    }

    inline bool isfew(const inner_node_ptr node, const slot_type offset) const {
        return node->size + offset < node->real_slot_size() >> (((IS_ML_NODE(node) ? self::log_inner_node_few_ratio[node->level] : traits::LOG_DATA_NODE_FEW_RATIO)) + 1);
    }

    inline bool isfew(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfew(static_cast<data_node_ptr>(node)) : isfew(static_cast<inner_node_ptr>(node));
    }

    inline bool isfull(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfull(static_cast<data_node_ptr>(node)) : isfull(static_cast<inner_node_ptr>(node));
    }

    // copy keys and pointers of a node to key buffer and pointers buffer
    void copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf);

    // copy keys of a node to key buffer
    //void copy_to_buffer(const inner_node_ptr node, key_type*  key_buf);

    // copy pointers of a node to pointers buffer
    //void copy_to_buffer(const inner_node_ptr node, node_ptr* child_buf);

    void init();

// debug
friend inner_node;

friend data_node;

friend Allocator;

#ifndef AEX_DEBUG
private:
    //ostream 
#endif

};

#ifdef AEX_DEBUG
template<typename _Key,
        typename _Val,
        typename traits>
int aex_tree<_Key, _Val, traits>::debug_level = 0;
#endif

template<typename _Key,
        typename _Val,
        typename traits>
double aex_tree<_Key, _Val, traits>::inner_node_full_ratio[traits::MAX_DEPTH];

template<typename _Key,
        typename _Val,
        typename traits>
double aex_tree<_Key, _Val, traits>::inner_node_few_ratio[traits::MAX_DEPTH];

template<typename _Key,
        typename _Val,
        typename traits>
int aex_tree<_Key, _Val, traits>::log_inner_node_full_ratio[traits::MAX_DEPTH];

template<typename _Key,
        typename _Val,
        typename traits>
int aex_tree<_Key, _Val, traits>::log_inner_node_few_ratio[traits::MAX_DEPTH];

};

#include "aex/aex_init.hpp"

#include "aex/aex_balance.hpp"

#include "aex/aex_insert.hpp"

#include "aex/aex_erase.hpp"

#include "aex/aex_SMO.hpp"