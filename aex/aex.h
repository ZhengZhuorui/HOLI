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
#include <algorithm>

#ifndef AEX_DEBUG
#define AEX_DEBUG
#endif

#ifndef AEX_EXPERIMENT
#define AEX_EXPERIMENT
#endif

#include "aex/aex_def.h"
#include "aex/aex_balance.h"
#include "aex/aex_traits.h"
#include "aex/aex_utils.h"
#include "aex/aex_model.h"
#include "aex/aex_node.h"
#include "aex/aex_allocator.h"
#include "aex/aex_iterator.h"

template<typename _Tp>
struct aex_node_balance_stats;

template<typename _Tp>
struct aex_tree_balance_stats;

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

    typedef typename traits::slot_type slot_type;

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

    // allocator:
    typedef aex_node_allocator<key_type, value_type, traits> NodeAllocator;

    // balance
    typedef aex_node_balance_stats<typename traits::AllowBalance> node_balance_stats;
    typedef aex_tree_balance_stats<typename traits::AllowBalance> tree_balance_stats;

    struct aex_stats{
        size_type data_node, inner_node; //debug
        size_type level_node[traits::MAX_DEPTH];
        size_type size;
        unsigned int height;
        key_type max_key, min_key;
        aex_stats():data_node(0), inner_node(0), size(0), height(0){
            memset(level_node, 0, sizeof(level_node));
            max_key = std::numeric_limits<key_type>::min();
            min_key = std::numeric_limits<key_type>::max();
        }
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

    tree_balance_stats balance_stats;

    NodeAllocator node_allocator;

    double inner_node_few_ratio[traits::MAX_DEPTH], inner_node_full_ratio[traits::MAX_DEPTH];

    typename std::false_type fp;

    constexpr static double lambda = 1 - 1.0 / traits::LAMBDA_;

public:

    aex_tree();

    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);

    aex_tree(const self& _index);

    aex_tree(self&& _index);

    ~aex_tree();

    aex_tree& operator = (aex_tree &_index){
        this->init();
        this->construct(_index.root, this->root);
        this->m_stats = _index.m_stats;
        this->balance_stats = _index.balance_stats;
        this->node_allocator = _index.node_allocator;
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
        this->node_allocator = _index.node_allocator;
        _index.node_allocator = NodeAllocator();
        return *this;
    }

    void clear(){
        AEX_HINT("[begin]");
        this->deconstruct(this->root);
        this->m_stats = aex_stats();
        this->root = this->head_leaf = this->tail_leaf = nullptr;
        AEX_HINT("[end]");
    }


    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    
    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    iterator find(const key_type &x) {
        this->balance_stats.update_timestamp();
        iterator it = find_iterator(x);
        if (it.key() != x) 
            return end();
        return it;
    }

    void range_scan(const key_type &L, const key_type &R, std::vector<std::pair<key_type, value_type>>& answer){
        this->balance_stats.update_timestamp();
        iterator iter = this->find_iterator(L);
        while(iter.key() < R){
            answer.push_back(std::make_pair(iter->key(), iter->value()));
            iter++;
        }
    }

    size_type count(const key_type &x){
        this->balance_stats.update_timestamp();
        iterator it = find(x);
        if (it.key() != x) 
            return 0;
        return 1;
    }

    bool exists(const key_type &x) {
        this->balance_stats.update_timestamp();
        iterator it = find(x, this->allow_rw_balance);
        if (it.key() != x) return false;
        return true;
    }

    iterator lower_bound(const key_type &x){
        this->balance_stats.update_timestamp();
        return find_iterator(x);
    }

    iterator upper_bound(const key_type &x){
        this->balance_stats.update_timestamp();
        iterator iter = find_iterator(x);
        while (iter.key() <= x) 
            ++iter;
        return iter;
    }

    /* erase one key*/
    size_t erase(const key_type &x){
        this->balance_stats.update_timestamp();
        if (root == nullptr) return 0;
        size_type cnt = erase_one(x);
        return cnt;
    }

    bool erase_one(const key_type &x){
        iterator find_iter = find_iterator(x);
        if (find_iter == end()) 
            return false;
        if (find_iter.key() != x)
            return false; 
        data_node_ptr node = find_iter._M_node;
        if (check_split(node, false)){
            std::vector<key_type> new_key;
            std::vector<node_ptr> new_child;
            split(node, new_key, new_child);
            AEX_ASSERT(new_key.size() > 1);
            if (new_key.size() > 1)
                insert_ascend(node->parent, new_key, new_child);
            find_iter = find_iterator(x);
        }
        erase_iterator(find_iter);
        return true;
    }

    inline void erase(const_iterator &iter){
        if (root == nullptr) return end();
        key_type x = iter._M_node->key[iter.offset];
        data_node_ptr node = iter._M_node;
        iterator niter = iter;
        if (check_split(node)){
            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            split(node, key_buf, child_buf);
            insert_ascend(node->parent, key_buf, child_buf);
            niter = find_iterator(x);
        }
        erase_iterator(niter);
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

    inline size_type size() const{
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
        AEX_IMPORTANT("data size=" << m_stats.size << ", tree height=" << m_stats.height << ", data node size=" << m_stats.data_node \
                    << ", inner node size=" << m_stats.inner_node << ", max key=" << m_stats.max_key << ", min_key=" << m_stats.min_key);
        node_allocator.print_stats();
        balance_stats.print_stats();
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

    inline void deconstruct(node_ptr node){
        erase_tree_recursive(node);
    }

    // ========== 0. utils ==========

    // ========== 1. find ==========

    // if no item greater than or equal to x, return end()
    inline iterator find_iterator(const key_type &x){
        data_node_ptr node = find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos == node->size)
            return end();

        if (IS_ML_NODE(node))
            if (std::abs(node->predict(x) - pos) >= traits::DATA_NODE_ERROR_BOUND * 2) {
                fix_data_node(node);
                node = find_leaf(x);
                pos = node->find_lower_pos(x);
            }
        return iterator(node, pos);
    }

    // if no item greater than x, return NULL
    inline iterator find_upper(const data_node_ptr node, const key_type &x){
        slot_type pos = node->find_upper_pos(x);
        if (pos == node->size)
            return end();
        return iterator(node, pos);
    }

    inline node_ptr find(const inner_node_ptr node, const key_type &x){
        return node->child_ptr[node->find(x)];
    }

    inline data_node_ptr find_leaf(const key_type &key){
        this->balance_stats.update_timestamp();
        node_ptr node = root;
        while (!IS_LEAF_NODE(node)){
            node->balance_stats.update_read_frequency(this->balance_stats.get_timestamp());
            node = find(static_cast<inner_node_ptr>(node), key);
        }
        return static_cast<data_node_ptr>(node);
    }

    // ========== 2. balance tree ==========

    // update node/tree freuency counter.
    // One subtree represents a segment, frequency = node->balance_stats.write_times / tree->balance_stats.write_times
    bool check_insert_merge(inner_node_ptr* node_buffer, slot_type size);
    void merge_nodes(inner_node_ptr* node_buffer, slot_type buffer_size);

    bool check_insert_merge(data_node_ptr* node_buffer, slot_type size);
    void merge_nodes(data_node_ptr* node_buffer, slot_type buffer_size);

    inline bool check_insert_merge(node_ptr* node_buffer, slot_type size){
        if (IS_LEAF_NODE(node_buffer[0]))
            return check_insert_merge(static_cast<data_node_ptr*>(node_buffer), size);
        else 
            return check_insert_merge(static_cast<inner_node_ptr*>(node_buffer), size);
    }

    inline void merge_nodes(node_ptr* node_buffer, slot_type size){
        if (IS_LEAF_NODE(node_buffer[0]))
            merge_nodes(static_cast<data_node_ptr*>(node_buffer), size);
        else 
            merge_nodes(static_cast<inner_node_ptr*>(node_buffer), size);
    }

    bool check_split(data_node_ptr node, bool is_forced=false);

    void update_node_list_frequency(node_ptr node, node_ptr* node_list, slot_type n);

    // ========== 3. insert ==========

    // insert an item(<key, data>) to data node
    // please use node->insert(key, data);
    // size_type insert_data(data_node_ptr node, const key_type &key, const value_type &data);

    // try to insert an node to inner node. If no position to insert, return false
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::false_type allow_rw_balance);
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::true_type allow_rw_balance);

    // A part of bulk load.
    void build_tree(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf);

    // insert some items to node from bottom to up
    void insert_ascend(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);

    void insert_split(inner_node_ptr node, const key_type* const key, const node_ptr* const child, const slot_type n,
                    std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    
    void insert_split(data_node_ptr node, const key_type key, const value_type data, 
                    std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    // ========== 4. erase ==========

    // erase subtree of the node.
    void erase_tree_recursive(node_ptr node);

    // erase an node from bottom to up
    void erase_ascend(inner_node_ptr node);

    // erase one iterator
    void erase_iterator(iterator &iter);

    // erase one child node from parent. return false if parent or child not exists
    // no free the node
    bool erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node);

    // erase one item(iterator) from data_node
    void erase_data(iterator &iter);

    bool erase_split(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);
    bool erase_split(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node);

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
    void split(data_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with child pointers array to inner node array.
    void split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, bool can_retrain=true);
    // split a ordered key array with data array to node array.
    void split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with data array to inner node array. Use linear probe(use greedy).
    void split_with_linear_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    slot_type linear_probe(const key_type* const key, const size_type n, data_node_model &m);

    // change the parent key of the child node.
    // need no key of item between old key and new key.
    bool update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const key_type &key);

    // Unused. Please use insert directly.
    // check if a node can insert the key.
    bool check_insert(const inner_node_ptr node, const key_type &key);

    // check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
    static bool check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, inner_node_model &m);

    // Rescale a node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
    // if node expand or narrow successed, the old node will free and return true. Otherwise return false.
    bool rescale(inner_node_ptr node, const slot_type new_slot_size);
    bool rescale(data_node_ptr node, const slot_type new_slot_size);
    bool rescale(node_ptr node, const slot_type new_slot_size);

    // link node_buf parent, prev and next pointer, the prev point of first node is node->prev, next too.
    void link_node_list_and_replace_last_node(node_ptr node, std::vector<node_ptr> &new_child);

    void fix_data_node(data_node_ptr node);

    inline bool isfull(const data_node_ptr node) const {
        return node->size >= node->slot_size;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfew(const data_node_ptr node, const slot_type offset) const {
        return (node->size + offset) < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return node->size >= node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_full_ratio[node->level] : traits::DATA_NODE_FULL_RATIO));
    }

    inline bool isfull(const inner_node_ptr node, slot_type offset) const {
        return node->size + offset >= node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_full_ratio[node->level] : traits::DATA_NODE_FULL_RATIO));
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return node->size < node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_few_ratio[node->level] : traits::DATA_NODE_FEW_RATIO));
    }

    inline bool isfew(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfew(static_cast<data_node_ptr>(node)) : isfew(static_cast<inner_node_ptr>(node));
    }

    inline bool isfull(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfull(static_cast<data_node_ptr>(node)) : isfull(static_cast<inner_node_ptr>(node));
    }

    // copy keys and pointers of a node to key buffer and pointers buffer
    static void copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* __restrict__ key_buf, node_ptr* __restrict__ child_buf);

    // copy keys of a node to key buffer
    static void copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* const __restrict__ key_buf);

    // copy pointers of a node to pointers buffer
    static void copy_to_buffer(const inner_node_ptr __restrict__ node, node_ptr* __restrict__ child_buf);

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
