#pragma once
#include <cmath>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <queue>

#ifndef AEX_DEBUG
#define AEX_DEBUG
#endif

#ifndef AEX_EXPERIMENT
#define AEX_EXPERIMENT
#endif

#include "aex/aex_traits.h"
#include "aex/aex_utils.h"
#include "aex/aex_model.h"
#include "aex/aex_node.h"
#include "aex/aex_allocator.h"
#include "aex/aex_iterator.h"


namespace aex{

template<typename _Key, 
        typename _Val,
        typename traits>
class aex_tree{
public:

    static_assert(std::is_arithmetic<_Key>::value, "key types must be numeric.");

    typedef aex_tree<_Key, _Val, traits> self;

    // type traits
    typedef typename traits::key_type key_type;

    typedef typename traits::value_type value_type;

    typedef typename traits::used_as_set used_as_set;

    typedef typename traits::size_type size_type;

    typedef typename traits::pos_type pos_type;

    typedef typename traits::version_type version_type;

    // iterator:
    typedef aex_iterator<_Key, _Val, traits> iterator;

    typedef aex_const_iterator<_Key, _Val, traits> const_iterator;

    typedef aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;

    typedef aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    typedef aex_node_base<_Key, _Val, traits> base_node;

    typedef base_node* node_ptr;

    // inner_node:    
    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model inner_node_model;

    // data_node:
    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef typename data_node::Model data_node_model;

    // bitmap:
    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::bitmap bitmap;

    struct aex_stats{
        size_type data_node, inner_node; //debug
        size_type size;
        size_type timestamp, recent_update_timestamp;
        double write_times, read_times, lambda_timestamp;
        unsigned int height;
        key_type max_key, min_key;
        aex_stats(){data_node = inner_node = size = height = write_times = read_times = timestamp = lambda_timestamp = 0;}
    };

    //#ifdef AEX_DEBUG
    static int debug_level; 
    //#endif

#ifndef AEX_EXPERIMENT
private:
#endif

    node_ptr root;

    data_node_ptr head_leaf;

    data_node_ptr tail_leaf;

    aex_stats m_stats;

    //Alloc& alloc_;

    //aex_allocator<_Key, _Val, Alloc, traits> allocator;

    aex_node_allocator<_Key, _Val, traits> node_allocator;

    //size_type max_inner_node_slot_size[8];
    double inner_node_few_ratio[traits::MAX_DEPTH], inner_node_full_ratio[traits::MAX_DEPTH];

    typename traits::AllowBalance allow_balance;

    typename std::false_type fp;

    constexpr static double lambda = 1 - 1.0 / traits::LAMBDA_;

public:

    aex_tree();

    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);

    aex_tree(const self& _index);

    aex_tree(self&& _index);

    ~aex_tree();

    void clear(){
        this->deconstruct(this->root);
        this->m_stats = aex_stats();
    }

    // con

    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    
    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    iterator find(const key_type &x) {
        data_node_ptr node = find_leaf(x, this->allow_balance);
        iterator it = find_lower(node, x);
        if (it.key() != x) 
            return end();
        return it;
    }

    const_iterator find(const key_type &x) const{
        std::false_type fp;
        data_node_ptr node = find_leaf(x, fp);
        iterator it = find_lower(node, x);
        if (it.key() != x) 
            return end();
        return it;
    }

    void range_query(const key_type &L, const key_type &R, std::vector<std::pair<key_type, value_type>>& answer){
        iterator iter = this->find(L);
        while(iter.key() < R){
            answer.push_back(std::make_pair(iter->key(), iter->value()));
        }
    }

    size_t count(const key_type &x){
        //if (find(x) != end()) return 1;
        if (traits::AllowMultiKey){
            iterator start = lower_bound(x), end = upper_bound(x);
            if (start._M_node == end._M_node) 
                return end.offset - start.offset;
            data_node_ptr node = start._M_node;
            size_type cnt = start->size - start.offset;
            node = node->next;
            while (node != end._M_node){
                cnt += node->size;
            }
            cnt += end.offset;
            return cnt;
        }
        else{
            std::false_type fp;
            iterator it = find(x, fp);
            if (it.key() != x) 
                return 0;
            return 1;
        }
    }

    bool exists(const key_type &x) {
        iterator it = find(x, this->allow_balance);
        if (it.key() != x) return false;
        return true;
    }

    iterator lower_bound(const key_type &x){
        data_node_ptr node = find_leaf(x, this->allow_balance);
        return find_lower(node, x);
    }

    const_iterator lower_bound(const key_type &x) const {
        std::false_type fp;
        data_node_ptr node = find_leaf(x, this->allow_balance);
        return find_lower(x, fp);
    }

    iterator upper_bound(const key_type &x){
        std::false_type fp;
        data_node_ptr node = find_leaf(x, fp);
        while (node != nullptr && node->key[node->size] <= x)
            node = node->next;
        if (node == nullptr) return end();
        iterator iter = find_upper(node, x);
        return iter;
    }

    const_iterator upper_bound(const key_type &x) const {
        std::false_type fp;
        data_node_ptr node = find_leaf(x, fp);
        while (node != nullptr && node->key[node->size] <= x)
            node = node->next;
        if (node == nullptr) return end();
        iterator iter = find_upper(node, x);
        return iter;
    }

    /* erase one key*/
    size_t erase(const key_type &x){
        if (root == nullptr) return 0;
        size_type cnt = 0;
        while (erase_one(x)){
            ++cnt;
            if (!traits::AllowMultiKey::value) break;
        }
        return static_cast<size_t>(cnt);
    }

    bool erase_one(const key_type &x){
        //std::true_type tp;
        node_ptr stack[traits::MAX_DEPTH];
        int top;
        data_node_ptr node = find_leaf_with_trace(x, stack, top, this->allow_balance);
        iterator find_iter = find_lower(node, x);
        if (find_iter == end()) 
            return false;
        if (find_iter.key() != x)
            return false;        
        if (this->allow_balance){
            pos_type best_slot_size = check_balance_split_best_slot_size(node);
            if (best_slot_size < node->slot_size){
                balance_split(stack, top, node, best_slot_size);
                node = find_leaf_with_trace(x, stack, top, this->fp);
            }
        }

        erase_iterator(find_iter, stack, top);
        return true;
    }

    inline void erase(const_iterator &iter){
        if (root == nullptr) return end();
        node_ptr stack[traits::MAX_DEPTH];
        int top;
        key_type x = iter._M_node->key[iter.offset];
        data_node_ptr node = find_leaf_with_trace(x, stack, top, this->allow_balance);
        if (traits::AllowMultiKey){
            node = iter._M_node;
            stack[top - 1] = iter._M_node;
        }
        if (this->allow_balance){
            size_type best_slot_size = check_balance_split_best_slot_size(node);
            if (best_slot_size < node->slot_size){
                balance_split(stack, top, node, best_slot_size);
            }
            node = find_leaf_with_trace(x, stack, top);
        }
        erase_iterator(iter, stack, top);
    }

    inline iterator begin() {
        return iterator(head_leaf, 0);
    }

    inline const_iterator begin() const {
        return const_iterator(head_leaf, 0);
    }

    inline iterator end(){
        return iterator(tail_leaf, tail_leaf->size);
    }

    inline const_iterator end() const {
        return const_iterator(tail_leaf, tail_leaf->size);
    }

    inline reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    inline const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    inline reverse_iterator rend(){
        return reverse_iterator(begin());
    }

    inline const_reverse_iterator rend() const{
        return reverse_iterator(begin());
    }

    inline size_t size() const{
        return static_cast<size_t>(m_stats.size);
    }

    inline bool empty() const {
        return m_stats.size == 0;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums);

    inline const aex_stats& get_stats() const
    {
        return m_stats;
    }

    void print_stats(){
        AEX_IMPORTANT("data size=" << m_stats.size << ", tree height=" << m_stats.height << ", data node size=" << m_stats.data_node \
                    << ", inner node size=" << m_stats.inner_node << ", max key=" << m_stats.max_key << ", min_key=" << m_stats.min_key);
        
    }

    void print_detail(){
        
    }

    inline size_type memory_used()const{
        // TODO
        return node_allocator._memory_used;
    }

#ifndef AEX_EXPERIMENT
protected:


private:    
#endif
     
    void construct(node_ptr node, node_ptr &new_node);

    void deconstruct(node_ptr node);

    // ========== 0. utils ==========

    // ========== 1. find ==========

    // if no item greater than or equal to x, return NULL
    inline iterator find_lower(const data_node_ptr node, const key_type &x){
        pos_type pos = node->find_lower_pos(x);
        if (pos == node->size)
            return end();
        return iterator(node, pos);
    }

    // if no item greater than x, return NULL
    inline iterator find_upper(const data_node_ptr node, const key_type &x){
        pos_type pos = node->find_upper_pos(x);
        if (pos == node->size)
            return end();
        return iterator(node, pos);
    }

    inline node_ptr find(const inner_node_ptr node, const key_type &x){
        return node->child_ptr[node->find(x)];
    }

    // find the lowest item greater than or equal x, if no, return end()
    data_node_ptr find_leaf(const key_type &key, std::true_type AllowBalance);

    data_node_ptr find_leaf(const key_type &key, std::false_type AllowBalance);

    data_node_ptr find_leaf_with_trace(const key_type &key, node_ptr* stack, int &top, std::true_type AllowBalance);

    data_node_ptr find_leaf_with_trace(const key_type &key, node_ptr* stack, int &top, std::false_type AllowBalance);

    // layout: [a, old_node, b] -> [a, old_node, new_node, b]
    void split(data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node);

    // ========== 2. balance tree ==========

    // update node/tree freuency counter.
    // One subtree represents a segment, frequency = node->base_stats.write_times / tree->base_stats.write_times
    void update_node_frequency(node_ptr node) const;
    void update_tree_frequency();
    inline double node_write_pro(const node_ptr node) const{
        return node->base_stats.write_times / this->m_stats.lambda_timestamp;
    }

    inline double node_train_pro(const node_ptr node) const{
        return node->base_stats.train_times / this->m_stats.lambda_timestamp;
    }

    //double split_cost(data_node_ptr node) const;

    //double merge_cost(inner_node_ptr node) const;

    data_node_ptr merge_to_node(inner_node_ptr node);
    bool check_balance_merge_subtree(inner_node_ptr node);
    bool check_balance_merge_nodes(node_ptr* node_buffer, size_type size);

    data_node_ptr balance_merge_to_left_node(inner_node_ptr __restrict__ parent, data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    inner_node_ptr balance_merge_to_left_node(inner_node_ptr __restrict__ parent, inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);

    //inner_node_ptr balance_merge_nodes(inner_node_ptr* node_buffer, size_type buffer_size);
    //data_node_ptr balance_merge_nodes(data_node_ptr* node_buffer, size_type buffer_size);
    node_ptr balance_merge_nodes(node_ptr* node_buffer, size_type buffer_size);

    bool check_balance_split(size_type node_slot_size, double node_write_pro, double train_pro, size_type slot_size);

    bool check_balance_split(data_node_ptr node, size_type slot_size=traits::MIN_DATA_NODE_SLOT_SIZE);
    
    pos_type check_balance_split_best_slot_size(data_node_ptr node);

    node_ptr balance_merge_subtree(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ parent);

    void balance_split(const node_ptr* stack, const int top, data_node_ptr node, pos_type slot_size);

    double estimate_cost() const;

    // ========== 3. insert ==========
    // Split an node if the node insert item and the size is larger than upper bound
    // if the node is replaced, the node will free
    bool insert_split(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ parent, const key_type* const key, const node_ptr* const child, 
               const pos_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    // insert an item(<key, data>) to data node
    // please use node->insert(key, data);
    // size_type insert_data(data_node_ptr node, const key_type &key, const value_type &data);

    // try to insert an node to inner node. If no position to insert, return false
    bool insert_node(inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::false_type allow_balance);
    bool insert_node(inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::true_type allow_balance);

    // A part of bulk load.
    void build_tree(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf);

    // insert an item to node(stack[top - 1]) from bottom to up
    void insert_one(const node_ptr* stack, const int top, const key_type &key, const node_ptr child);

    // insert some items to node(stack[top - 1]) from bottom to up

    void insert_many(const node_ptr* stack, const int top, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);

    // ========== 4. erase ==========

    // erase subtree of the node.
    void erase_tree_recursive(node_ptr node);

    // erase an node from bottom to up
    void erase_node(inner_node_ptr node, node_ptr* stack, int top);

    // erase one iterator
    void erase_iterator(iterator &iter, node_ptr* stack, int top);

    // erase one child node from parent. return false if parent or child not exists
    // no free the node
    bool erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node);

    // erase one item(iterator) from data_node
    void erase_data(iterator &iter);

    // ========== 4. Structure Modify Operation(SMO) ==========

    // split a ordered key array with child pointers array to inner node array. Support the old node firstly.
    bool split_with_old_node(const key_type* const __restrict__ key, const node_ptr* const __restrict__ child, const size_type n, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr __restrict__ node);
    
    // split a ordered key array with child pointers array to inner node array.
    void split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with data array to inner node array.
    void split(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with data array to inner node array. Use linear probe(use greedy).
    void split_with_linear_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    pos_type linear_probe(const key_type* const key, const size_type n, data_node_model &m);

    // change the parent key of the child node.
    // need no key of item between old key and new key.
    bool update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const key_type &key);

    // change the parent node ptr with same key. The old_node->key[0] must equal to the new_node->key[0].
    bool update_childnode_ptr(inner_node_ptr __restrict__ parent, const node_ptr old_node, const node_ptr new_node);

    // Unused. Please use insert directly.
    // check if a node can insert the key.
    bool check_insert(const inner_node_ptr node, const key_type &key);

    // check if a node can insert the key. If true, insert it.
    //bool check_insert_and_do(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ new_node);
    
    inline size_type init_rewired_cnt(inner_node_ptr node){
        return static_cast<size_type>(traits::INIT_REWIRED_CNT * log(node->real_slot_size()));
    }

    // check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
    static bool check_rewired(const key_type* const key, const pos_type size, const pos_type slot_size, inner_node_model &m);

    // rewired the <key, node_ptr> array of a node. Return true if <K, P> array can be rewired. Otherwise return false.
    bool rewired(inner_node_ptr node);

    // Rescale a node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
    // if node expand or narrow successed, the old node will free and return true. Otherwise return false.
    bool rescale(inner_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio);
    bool rescale(data_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio);
    bool rescale(node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio);

    // copy keys and data/pointers from a node to another node, regardless of the node type
    void copy_node(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ new_node);

    // merge right leaf to left leaf.
    void merge_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    // merge left leaf to right leaf.
    void merge_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    // merge right inner node to left inner node. require the left inner node and right inner node must be not ML node.
    void merge_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);

    // merge left inner node to right inner node. require the left inner node and right inner node must be not ML node.
    void merge_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);

    // shift one item from right leaf to left leaf
    void shift_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    // shift one item from left leaf to right leaf
    void shift_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

    // shift one item from right inner node to left brother, the left node must be least node, because left node will narrow if left node is node_property::ML_NODE, it is same as shift_to_right_node
    void shift_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);

    // shift one item from left inner node to right brother, the left node must be least node, because right node will narrow if right node is node_property::ML_NODE,
    // right node must not be ML_NODE
    void shift_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);

    // replace new_node to old_node (contain m_stats, level, prev, next of node)
    void replace_node(const inner_node_ptr __restrict__ old_node, inner_node_ptr __restrict__ new_node);
    void replace_node(const data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node);

    inline void data_memmove(value_type* __restrict__ dest, const value_type* const __restrict__ src, const size_type n){
        used_as_set s;
        data_memmove(dest, src, n, s);
    }

    //inline size_type max_inner_slot_size_func(size_type level) const {
    //    return (level < 7)?this->max_inner_node_slot_size[level]:this->max_inner_node_slot_size[6];
    //}

    inline bool isfull(const data_node_ptr node) const {
        return node->size >= node->slot_size;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return (node->prop & node_property::ML_NODE) ? (node->size >= node->slot_size * this->inner_node_full_ratio[node->level]) : (node->size >= node->slot_size * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return (node->prop & node_property::ML_NODE) ? (node->size < node->slot_size * this->inner_node_few_ratio[node->level]) : (node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO);
    }

    inline bool isfew(const node_ptr node) const{
        return (node->prop & LEAF) ? isfew(static_cast<data_node_ptr>(node)) : isfew(static_cast<inner_node_ptr>(node));
    }

    inline bool isfull(const node_ptr node) const{
        return (node->prop & LEAF) ? isfull(static_cast<data_node_ptr>(node)) : isfull(static_cast<inner_node_ptr>(node));
    }

    // copy keys and pointers of a node to key buffer and pointers buffer
    void copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* __restrict__ key_buf, node_ptr* __restrict__ child_buf);

    // copy keys of a node to key buffer
    void copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* const __restrict__ key_buf);

    // copy pointers of a node to pointers buffer
    void copy_to_buffer(const inner_node_ptr __restrict__ node, node_ptr* __restrict__ child_buf);

    void init();

// debug
#ifdef AEX_DEBUG
public:


friend inner_node;

friend data_node;
#endif

#ifndef AEX_EXPERIMENT
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

};

#include "aex/aex_init.hpp"

#include "aex/aex_balance.hpp"

#include "aex/aex_find.hpp"

#include "aex/aex_insert.hpp"

#include "aex/aex_erase.hpp"

#include "aex/aex_SMO.hpp"

#include "aex/aex_con.hpp"
