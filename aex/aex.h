#pragma once
#include <cmath>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <utility>

//#define AEX_DEBUG

#define AEX_DEBUG_MSG

#ifdef AEX_DEBUG

#define AEX_PRINT(x)  do { std::cout << "[DEBUG] File:" << __FILE__ << ", Line:" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << std::endl; } while(0)

#define AEX_ASSERT(x) do { assert(x); } while(0)

#else

#define AEX_PRINT(x) do { } while(0)

#define AEX_ASSERT(x) do { } while(0)

#endif

#define AEX_DEBUG_MSG

#ifdef AEX_DEBUG_MSG

#define AEX_DEBUG_PRINT(x)  do { std::cout << "[DEBUG] File:" << __FILE__ << ", Line:" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << std::endl; } while(0)

#else 

#define AEX_DEBUG_PRINT() do {} while(0)

#endif


#include "aex/aex_traits.h"
#include "aex/aex_utils.h"
#include "aex/aex_model.h"
#include "aex/aex_node.h"
#include "aex/aex_allocator.h"
#include "aex/aex_iterator.h"

namespace aex{

template<typename _Key, typename _Val,
        typename traits>
class aex_tree{
public:

    typedef typename traits::key_type key_type;

    typedef typename traits::value_type value_type;

    typedef typename traits::used_as_set used_as_set;

    typedef typename traits::AllowMultiKey AllowMultiKey;

    typedef typename traits::size_type size_type;

    typedef aex_tree self;

    typedef aex_iterator<_Key, _Val, traits> iterator;

    typedef aex_const_iterator<_Key, _Val, traits> const_iterator;

    typedef aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;

    typedef aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    typedef aex_node_base<_Key, _Val, traits> node;

    typedef node* node_ptr;
    
    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model Model;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef aex_allocator<_Key, _Val, traits> allocator;

    typedef aex_node_allocator<_Key, _Val, traits> node_allocator;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename bitmap_impl::bitmap bitmap;

    struct aex_stats{
        size_type data_node, inner_node, size, height;
        size_type write_times, read_times;
        aex_stats(){data_node = 0; inner_node = 0; size = 0; height = 0; write_times = 0;}
    };

    #ifdef AEX_DEBUG
    int debug_level; 
    #endif

private:

    node_ptr root;

    data_node_ptr head_leaf;

    data_node_ptr tail_leaf;

    aex_stats m_stats;

    size_type max_inner_node_slot_size[8], ops_times;

    long long read_times, write_times;

public:

    aex_tree();

    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);

    aex_tree(const self& _index);

    aex_tree(self&& _index);

    ~aex_tree();

    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }

    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    iterator find(const key_type &x) {
        std::true_type tp;
        iterator it = find_lower(x, tp);
        if (it.key() != x) 
            return end();
        return it;
    }

    const_iterator find(const key_type &x) const{
        std::false_type fp;
        iterator it = find_lower(x, fp);
        if (it.key() != x) 
            return end();
        return it;
    }

    size_type count(const key_type &x){
        if (find(x) != end()) return 1;
        return 0;
    }

    bool exists(const key_type &x) {
        std::true_type tp;
        iterator it = find_lower(x, tp);
        if (it.key() != x) return false;
        return true;
    }

    iterator lower_bound(const key_type &x){
        std::true_type tp;
        return find_lower(x, tp);
    }

    iterator upper_bound(const key_type &x){
        std::true_type tp;
        return find_upper(x, tp);
    }

    /* erase one key*/
    size_type erase(const key_type &x){
        if (root == nullptr) return 0;
        size_type cnt = 0;
        while (erase_one(x)){
            ++cnt;
            if (!traits::AllowMultiKey) break;
        }
        return cnt;
    }

    bool erase_one(const key_type &x){
        std::true_type tp;
        iterator find_iter = find(x, tp);
        if (find_iter == end()) return false;
        bool res = erase(find_iter);
        return res;
    }

    void erase(const iterator iter){
        if (root == nullptr) return end();
        --m_stats.size;
        return;
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
        return m_stats.size;
    }

    inline bool empty() const {
        return m_stats.size == 0;
    }

    inline const struct aex_stats& get_stats() const
    {
        return m_stats;
    }

protected:

private:    
    void construct(node_ptr node, node_ptr &new_node);

    void deconstruct(node_ptr node);

    // ========== 1. find ==========

    node_ptr find_head_leaf(node_ptr node) const;

    node_ptr find_tail_leaf(node_ptr node) const;

    // if no item greater than or equal x, return node->slot_size (ml node) or node->size(otherwise)
    size_type find_lower_pos(inner_node_ptr node, const key_type &x, const std::true_type tp);
    size_type find_lower_pos(const inner_node_ptr node, const key_type &x, const std::false_type fp) const;

    // if no item greater than or equal x, return node->size
    size_type find_lower_pos(const data_node_ptr node, const key_type &x) const;

    // if no item greater than x, return node->slot_size (ml node) or node->size(otherwise)
    size_type find_upper_pos(inner_node_ptr node, const key_type &x, const std::true_type tp);
    size_type find_upper_pos(const inner_node_ptr node, const key_type &x, const std::false_type fp) const;

    // if no item greater than x, return node->size
    size_type find_upper_pos(const data_node_ptr node, const key_type &x);

    // if no item greater than or equal x, return NULL
    node_ptr find_lower(inner_node_ptr node, const key_type &x, const std::true_type tp);
    node_ptr find_lower(const inner_node_ptr node, const key_type &x, const std::false_type fp) const;

    iterator find_lower(const data_node_ptr node, const key_type &x);

    // find the lowest item greater than or equal x, if no, return end()
    iterator find_lower(const key_type &key, const std::true_type tp);

    const_iterator find_lower(const key_type &key, const std::false_type fp) const;

    // find the lowest item greater than x, if no, return end()
    node_ptr find_upper(inner_node_ptr node, const key_type &x, const std::true_type tp);
    node_ptr find_upper(const inner_node_ptr node, const key_type &x, const std::false_type tp) const;

    iterator find_upper(const data_node_ptr node, const key_type &x);

    iterator find_upper(const key_type &key, const std::true_type tp);

    const_iterator find_upper(const key_type &key, const std::false_type fp) const;

    // layout: [a, old_node, b] -> [a, old_node, new_node, b]
    void split(data_node_ptr old_node, data_node_ptr new_node);

    // ========== 2.balance tree ==========
    
    bool check_merge(inner_node_ptr node);

    bool check_split(data_node_ptr node);
    
    double check_split(inner_node_ptr node, size_type slot_size);

    void balance_merge(inner_node_ptr node);

    void balance_split(data_node_ptr node);

    double estimate_cost() const ;

    // ========== 3. insert ==========
    // Split an node if the node insert item and the size is larger than upper bound
    // if the node is replaced, the node will free
    bool insert_split(inner_node_ptr node, inner_node_ptr parent, const key_type* const key, const node_ptr* const child, 
               const unsigned int n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    // insert an item to data node
    size_type insert_data(data_node_ptr node, const key_type &key, const value_type &data);

    // insert an node to inner node
    size_type insert_node(inner_node_ptr node, const key_type &key, const node_ptr child);

    void bulk_load_node(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf);

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums);

    // insert an item from bottom to up
    bool insert_one(inner_node_ptr node, const key_type &key, const node_ptr child);

    bool insert_many(inner_node_ptr node, std::vector<key_type> &key, std::vector<node_ptr> &child);

    void insert_subtree(inner_node_ptr node, std::vector<key_type> &key, std::vector<node_ptr> &child);

    // ========== 4. erase ==========
    // erase subtree of the node.
    void erase_subtree(node_ptr node);

    void erase_tree_recursive(node_ptr node);

    // erase an item from bottom to up
    bool erase_node(node_ptr node);

    // erase one iterator
    void erase_iterator(iterator &iter);

    // erase one child node from parent. return false if parent or child not exists
    // no free the node
    bool erase_son_node(node_ptr parent, inner_node_ptr node);

    // erase one item(iterator) from parent
    void node_son_data(data_node_ptr node, iterator &iter);

    // ========== 4. operation ==========
    bool split_with_old_node(const key_type* const key, const node_ptr* const child, const unsigned int n, const unsigned int level, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr node);
    
    void split(const key_type* const key, const node_ptr* const child, const unsigned int n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    // change the parent key of the child node
    bool update_childnode_key(inner_node_ptr parent, const node_ptr node, const key_type &key);

    // change the parent node ptr with same key
    bool update_childnode_ptr(inner_node_ptr parent, const node_ptr old_node, const node_ptr new_node);

    // check if a node can insert the key
    bool check_insert(const inner_node_ptr node, const key_type &key);
    
    // check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
    bool check_rewired(const key_type* const key, const size_type size, const size_type slot_size, Model &m);

    // rewired the key-points position.
    bool rewired(inner_node_ptr node);

    // if node is expanded, the old node will free and  return true. Otherwise return false
    bool expand(inner_node_ptr &node, const inner_node_ptr &parent);

    // if node is narrowed, the old node will free and return true. Otherwise return false;
    bool narrow(inner_node_ptr &node);

    // copy keys and data/pointers from a node to another node, regardless of the node type
    void copy_node(inner_node_ptr node, inner_node_ptr new_node);

    // merge right leaf to left leaf
    void merge_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node);

    // merge left leaf to right leaf
    void merge_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node);

    // merge left inner node to right inner node. require the left inner node and right inner node must be not ML node.
    void merge_to_left_node(inner_node_ptr left_node, inner_node_ptr right_node);

    // merge right inner node to left inner node. require the left inner node and right inner node must be not ML node.
    void merge_to_right_node(inner_node_ptr left_node, inner_node_ptr right_node);

    // shift one item from right leaf to left leaf
    void shift_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node);

    // shift one item from left leaf to right leaf
    void shift_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node);

    // shift one item from right inner node to left brother, the left node must be least node, because left node will narrow if left node is ML_NODE, it is same as shift_to_right_node
    void shift_to_left_node(inner_node_ptr left_node, inner_node_ptr right_node);

    // shift one item to right brother, the right node must be least node
    void shift_to_right_node(inner_node_ptr left_node, inner_node_ptr right_node);

    inline void data_memmove(value_type* dest, const value_type* const src, const size_type n, std::true_type f){}

    inline void data_memmove(value_type* dest, const value_type* const src, const size_type n, std::false_type f){
        memmove(dest, src, n);
    }

    inline void data_memmove(value_type* dest, const value_type* const src, const size_type n){
        used_as_set s;
        data_memmove(dest, src, n, s);
    }

    inline size_type max_inner_slot_size_func(size_type level) const {
        return (level < 7)?this->max_inner_node_slot_size[level]:this->max_inner_node_slot_size[6];
    }

    inline bool isfull(const data_node_ptr node) const {
        return node->size >= node->slot_size;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return (node->prop & ML_NODE) ? (node->size >= node->slot_size * traits::INNER_NODE_FULL_RATIO) : (node->size >= node->slot_size * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return (node->prop & ML_NODE) ? (node->size < node->slot_size * traits::INNER_NODE_FEW_RATIO) : (node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO);
    }

    // copy keys and pointers of a node to key buffer and pointers buffer
    void copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf);

    // copy keys of a node to key buffer
    void copy_to_buffer(const inner_node_ptr node, key_type* const key_buf);

    // copy pointers of a node to pointers buffer
    void copy_to_buffer(const inner_node_ptr node, node_ptr* child_buf);

    void init();

// debug
#ifdef AEX_DEBUG_MSG
public:

    // debug the subtree of a node
    std::pair<key_type, bool> _debug(node_ptr node);

    // debug 
    bool debug_error();

private:
    //ostream 
#endif


};

};

#include "aex/aex_init.hpp"

#include "aex/aex_balance.hpp"

#include "aex/aex_find.hpp"

#include "aex/aex_insert.hpp"

#include "aex/aex_erase.hpp"

#include "aex/aex_operation.hpp"

