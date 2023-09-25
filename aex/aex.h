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

    typedef aex_dynamic_node_base<_Key, _Val, traits> base_dynamic_node;

    typedef base_dynamic_node* dynamic_node_ptr;

    // inner_node:    
    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model inner_node_model;

    // data_node:
    typedef aex_data_node<_Key, _Val, traits> dynamic_data_node;
    typedef dynamic_data_node* dynamic_data_node_ptr;

    typedef aex_static_data_node<_Key, _Val, traits> static_data_node;
    typedef static_data_node* static_data_node_ptr;

    typedef static_data_node data_node;

    //static_assert((traits::AllowDynamicDataNode::value && std::is_same<aex_static_data_node<_Key, _Val, traits>, data_node>::value) == false);

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
        size_type level_node[traits::MAX_DEPTH];
        size_type size;
        unsigned int height;
        key_type max_key, min_key;
        aex_stats():size(0), height(0){
            memset(level_node, 0, sizeof(level_node));
            max_key = std::numeric_limits<key_type>::min();
            min_key = std::numeric_limits<key_type>::max();
        }
        inline size_type inner_node(){
            return (height > 1) ? std::reduce(level_node + 1, level_node + height) : 0;
        }
        inline size_type data_node(){
            return level_node[0];
        }
    };

    #ifdef AEX_EXPERIMENT
    struct operation_stats{
        operation_stats():inner_node_split_cnt(0), inner_node_merge_cnt(0), inner_node_rescale_cnt(0), inner_node_balance_split_cnt(0),
                        data_node_split_cnt(0), data_node_merge_cnt(0), data_node_rescale_cnt(0){}
        size_type inner_node_split_cnt, inner_node_merge_cnt, inner_node_rescale_cnt, inner_node_balance_split_cnt;
        size_type data_node_split_cnt, data_node_merge_cnt, data_node_rescale_cnt;
        void print_stats(){
            AEX_PRINT("[Operation status] inner node: split times=" << inner_node_split_cnt << ", merge times=" << inner_node_merge_cnt <<
                    ", rescale times=" << inner_node_rescale_cnt << ", inner_node_balance_split_cnt=" << inner_node_balance_split_cnt);
            AEX_PRINT(" data node: split times=" << data_node_split_cnt << ", merge times=" << data_node_merge_cnt <<
                    ", rescale times=" << data_node_rescale_cnt);
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

    NodeAllocator node_allocator;

    data_node_ptr empty_leaf;

    double inner_node_few_ratio[traits::MAX_DEPTH], inner_node_full_ratio[traits::MAX_DEPTH];

    size_type max_inner_node_slot_size[traits::MAX_DEPTH];

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
        this->root = this->construct(_index.root);
        this->link_tree_ptr();
        this->m_stats = _index.m_stats;
        this->balance_stats = _index.balance_stats;
        //this->node_allocator = _index.node_allocator;
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
        this->deconstruct(this->root);
        this->m_stats = aex_stats();
        this->root = this->head_leaf = this->tail_leaf = nullptr;
    }


    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    
    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    inline iterator find(const key_type &x) {
        this->balance_stats.update_timestamp();
        iterator it = find_iterator(x);
        if (it == end() || it.key() != x) 
            return end();
        return it;
    }

    void range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer){
        this->balance_stats.update_timestamp();
        iterator iter = this->find_iterator(lower_key);
        while(iter.key() <= upper_key){
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
        iterator it = find(x);
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
        const_iterator find_iter = find_iterator(x);
        if (find_iter == end()) 
            return false;
        if (find_iter.key() != x)
            return false; 
        data_node_ptr node = find_iter._M_node;
        if (std::is_same<data_node, dynamic_data_node>::value && check_split((dynamic_data_node_ptr)(node), false)){
            std::vector<key_type> new_key;
            std::vector<node_ptr> new_child;
            split((dynamic_data_node_ptr)node, new_key, new_child);
            AEX_ASSERT(new_key.size() > 1);
            if (new_key.size() > 1)
                insert_ascend(node->parent, new_key, new_child);
            find_iter = find_iterator(x);
        }
        AEX_ASSERT(find_iter._M_node != empty_leaf);
        erase_iterator(find_iter);
        return true;
    }

    inline void erase(const_iterator &iter){
        AEX_ERROR("???");
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

    inline iterator end(){
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
        AEX_IMPORTANT("data size=" << m_stats.size << ", tree height=" << m_stats.height << ", data node size=" << m_stats.data_node() \
                    << ", inner node size=" << m_stats.inner_node());
        node_allocator.print_stats();
        balance_stats.print_stats();
        #ifdef AEX_EXPERIMENT
        opt_stats.print_stats();
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

    inline size_type memory_used()const{
        return node_allocator._memory_used;
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
    inline iterator find_iterator(const key_type &x){
        data_node_ptr node = find_leaf(x);
        //bool flag = true;
        //slot_type pos = node->find_lower_pos(x, flag);
        slot_type pos = node->find_lower_pos(x);
        if (pos == node->size)
            return end(); 
        //if (!flag){
        //    fix_data_node(node);
        //    node = find_leaf(x);
        //    pos = node->find_lower_pos(x);
        //}
        return iterator(node, pos);
    }

    // if no item greater than x, return NULL
    inline iterator find_upper(const data_node_ptr node, const key_type &x){
        slot_type pos = node->find_upper_pos(x);
        if (pos == node->size)
            return end();
        return iterator(node, pos);
    }

    inline data_node_ptr find_leaf(const key_type &key){
        node_ptr node = root;
        while (!IS_LEAF_NODE(node)){
            slot_type pos = static_cast<inner_node_ptr>(node)->find(key);
            //if (static_cast<inner_node_ptr>(node)->child_ptr[pos]->parent != node){
            //    AEX_PRINT("root=" << this->root << ", node=" << node << ", child=" << static_cast<inner_node_ptr>(node)->child_ptr[pos] << ", child->parent=" << static_cast<inner_node_ptr>(node)->child_ptr[pos]->parent);
            //}
            //AEX_PRINT("pos=" << pos << ", key=" << key << ", node key=" << static_cast<inner_node_ptr>(node)->key_ptr[pos] << ", child=" << static_cast<inner_node_ptr>(node)->child_ptr[pos]);
            AEX_ASSERT(static_cast<inner_node_ptr>(node)->child_ptr[pos]->parent == node);
            node = static_cast<inner_node_ptr>(node)->child_ptr[pos];
            //if (node->next != nullptr && node->next->prev != node){
            //    this->print_stats();
            //    AEX_PRINT("IS_LEAF_NODE?" << IS_LEAF_NODE(node) << ", size=" << node->size);
            //    AEX_PRINT("");
            //}
            //if (!(node->next == nullptr || node->next->prev == node)){
            //    AEX_PRINT("IS_LEAF?" << IS_LEAF_NODE(node) << ", node=" << node << ", next=" << node->next << ", empty_leaf=" << this->empty_leaf);
            //}
            AEX_ASSERT((node->prev == nullptr || node->prev->next == node));
            AEX_ASSERT((node->next == nullptr || node->next->prev == node));
        }
        //for (auto i = 0; i < static_cast<data_node_ptr>(node)->size; ++i){
        //    std::cout << static_cast<data_node_ptr>(node)->key[i] << " ";
        //}
        //std::cout << std::endl;
        //for (auto i = 0; i < static_cast<data_node_ptr>(node->next)->size; ++i){
        //    std::cout << static_cast<data_node_ptr>(node->next)->key[i] << " ";
        //}
        //std::cout << std::endl;
        //if (node->next->parent != nullptr)
        //for (auto i = 0; i < static_cast<inner_node_ptr>(node->next->parent)->slot_size; ++i){
        //    std::cout << static_cast<inner_node_ptr>(node->next->parent)->key_ptr[i] << " ";
        //}
        //std::cout << std::endl;
        //AEX_PRINT("parent=" << node->parent << ", next=" << node->next << ", next->parent=" << node->next->parent);
        
        return static_cast<data_node_ptr>(node);
    }

    // ========== 2. balance tree ==========

    // update node/tree freuency counter.
    // One subtree represents a segment, frequency = node->balance_stats.write_times / tree->balance_stats.write_times
    bool check_insert_merge(node_ptr* node_buffer, slot_type size);
    void merge_nodes(key_type* key_buffer, inner_node_ptr* node_buffer, slot_type buffer_size);
    void merge_nodes(key_type* key_buffer, dynamic_data_node_ptr* node_buffer, slot_type buffer_size);

    inline void merge_nodes(key_type* key_buffer, dynamic_node_ptr* node_buffer, slot_type size){
        if (IS_LEAF_NODE(node_buffer[0]))
            merge_nodes(static_cast<dynamic_data_node_ptr*>(node_buffer), size);
        else 
            merge_nodes(static_cast<inner_node_ptr*>(node_buffer), size);
    }

    bool check_split(dynamic_data_node_ptr node, bool is_forced=false);
    bool check_split(inner_node_ptr node);

    void update_node_list_frequency(dynamic_node_ptr node, node_ptr* node_list, slot_type n);

    // ========== 3. insert ==========

    // insert an item(<key, data>) to data node
    // please use node->insert(key, data);
    // size_type insert_data(data_node_ptr node, const key_type &key, const value_type &data);

    // try to insert an node to inner node. If no position to insert, return false
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::false_type allow_rw_balance);
    bool insert_node(inner_node_ptr node, const key_type &key, const node_ptr child, std::true_type allow_rw_balance);

    // A part of bulk load.
    void build_tree(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf);

    // a part of function "ml_node_insert_split_pipeline". If node can't insert to parent pipeline, then split and insert to parent together.
    // start means the number of function "ml_node_insert_split_pipeline" split. key and child is the splited key and child. half_flag means 
    void ml_node_insert_split_bulk_load(inner_node_ptr node, const slot_type start, const key_type key, node_ptr child, bool half_flag);
    
    // insert child to node and split it, then insert them to node->parent pipeline
    void ml_node_insert_split_pipeline(inner_node_ptr node, const key_type* const key, const node_ptr* const child, const slot_type n);

    // insert one items to node from bottom to up. If node split, return false, else return true.
    bool insert_one(inner_node_ptr node, key_type new_key, node_ptr new_node);

    // insert some items to node from bottom to up. If node split, return false, else return true.
    bool insert_ascend(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);

    // insert some child to a inner node.
    void insert_split(inner_node_ptr node, const key_type* const key, const node_ptr* const child, const slot_type n);
    
    // insert some data to a dynamic data node.
    void insert_split(dynamic_data_node_ptr node, const key_type key, const value_type data, 
                    std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    // ========== 4. erase ==========

    // erase subtree of the node.
    void erase_tree_recursive(node_ptr node);

    // erase an node from bottom to up
    void erase_ascend(inner_node_ptr node);

    // erase one iterator
    void erase_iterator(const_iterator &iter);

    // erase one child node from parent. return false if parent or child not exists
    // free the node
    void erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node);

    // erase one item(iterator) from data_node
    void erase_data(iterator &iter);

    bool erase_merge(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node);
    bool erase_merge(dynamic_data_node_ptr __restrict__ left_node, dynamic_data_node_ptr __restrict__ right_node);
    bool erase_merge(static_data_node_ptr __restrict__ left_node, static_data_node_ptr __restrict__ right_node);

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
    void split(dynamic_data_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with child pointers array to inner node array.
    void split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, bool can_retrain=true);
    void split(const key_type* const key, const size_type n, const unsigned int level);

    // split a ordered key array with data array to node array.
    void split_to_static_data_node(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with data array to node array.
    void split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);
    // split a ordered key array with data array to inner node array. Use linear probe(use greedy).
    void split_with_linear_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child);

    void split(data_node_ptr new_node, data_node_ptr old_node);

    slot_type linear_probe(const key_type* const key, const size_type n, data_node_model &m);

    bool update_childnode_ptr(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const node_ptr __restrict__ new_node){
        slot_type pos = parent->at(node);
        if (pos == parent->slot_size){
            return false;
        }
        slot_type prev_pos = parent->prev_item(pos);
        std::fill(parent->child_ptr + prev_pos + 1, parent->child_ptr + pos + 1, new_node);
        return true;
    }

    // check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
    static bool check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, inner_node_model &m);

    // Rescale a node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
    // if node expand or narrow successed, the old node will free and return true. Otherwise return false.
    bool rescale(inner_node_ptr node, const slot_type new_slot_size);
    bool rescale_implement(inner_node_ptr node, const slot_type new_slot_size);
    bool rescale(dynamic_data_node_ptr node, const slot_type new_slot_size);
    bool rescale(node_ptr node, const slot_type new_slot_size);

    // link node_buf parent, prev and next pointer, the prev point of first node is node->prev, next too.
    void link_node_list_and_replace_last_node(node_ptr node, std::vector<node_ptr> &new_child);

    void fix_data_node(dynamic_data_node_ptr node);

    void add_root(key_type* key_buf, node_ptr* child_buf, slot_type n);
    inline void add_root(key_type &new_key, node_ptr new_child){
        node_ptr child_buf[2];
        child_buf[0] = new_child;
        child_buf[1] = root;
        add_root(&new_key, &child_buf, 2);
    }

    inline bool isfull(const dynamic_data_node_ptr node) const {
        return node->size >= node->slot_size;
    }

    inline bool isfull(const static_data_node_ptr node) const {
        return node->size >= traits::MIN_DATA_NODE_SLOT_SIZE;
    }

    inline bool isfew(const dynamic_data_node_ptr node) const {
        return node->size < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfew(const static_data_node_ptr node) const {
        return node->size < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfew(const data_node_ptr node, const slot_type offset) const {
        return (node->size + offset) < node->slot_size * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return node->size >= node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_full_ratio[node->level] : traits::DATA_NODE_FULL_RATIO));
    }

    inline bool isfull(const inner_node_ptr node, const slot_type offset) const {
        return node->size + offset >= node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_full_ratio[node->level] : traits::DATA_NODE_FULL_RATIO));
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return node->size < node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_few_ratio[node->level] : traits::DATA_NODE_FEW_RATIO)) * traits::DENSITY_NARROW_RATIO;
        //return node->size < node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_few_ratio[node->level] : traits::DATA_NODE_FEW_RATIO)) * traits::DENSITY_NARROW_RATIO;
    }

    inline bool isfew(const inner_node_ptr node, const slot_type offset) const {
        return node->size + offset < node->real_slot_size() * ((IS_ML_NODE(node) ? this->inner_node_few_ratio[node->level] : traits::DATA_NODE_FEW_RATIO)) * traits::DENSITY_NARROW_RATIO;
    }

    inline bool isfew(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfew(static_cast<data_node_ptr>(node)) : isfew(static_cast<inner_node_ptr>(node));
    }

    inline bool isfull(const node_ptr node) const{
        return IS_LEAF_NODE(node) ? isfull(static_cast<data_node_ptr>(node)) : isfull(static_cast<inner_node_ptr>(node));
    }

    // copy keys and pointers of a node to key buffer and pointers buffer
    static void copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf);

    // copy keys of a node to key buffer
    static void copy_to_buffer(const inner_node_ptr node, key_type*  key_buf);

    // copy pointers of a node to pointers buffer
    static void copy_to_buffer(const inner_node_ptr node, node_ptr* child_buf);

    void init();

// debug
friend inner_node;

friend data_node;

friend NodeAllocator;

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

};

#include "aex/aex_init.hpp"

#include "aex/aex_balance.hpp"

#include "aex/aex_insert.hpp"

#include "aex/aex_erase.hpp"

#include "aex/aex_SMO.hpp"

#include "aex/aex_con.hpp"
