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
#include "aex/aex_allocator.h"
#include "aex/aex_iterator.h"
#include "aex/concurrency/aex_node_con.h"
#include "aex/concurrency/aex_hash_table_con.h"

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

    // components:
    typedef aex_tree<_Key, _Val, traits>      self;
    typedef aex_default_components<traits>    components;
    typedef typename components::size_type    size_type;
    typedef typename components::version_type version_type;
    typedef typename components::base_node    base_node;
    typedef typename components::inner_node   inner_node;
    typedef typename components::data_node    data_node;
    typedef typename components::hash_node    hash_node;
    typedef typename components::dense_node   dense_node;
    typedef typename components::Allocator    Allocator;
    typedef typename components::HashTable    HashTable;
    typedef typename components::bitmap_impl  bitmap_impl;

    // iterator:
    typedef aex_iterator<_Key, _Val, traits>       iterator;
    typedef aex_const_iterator<_Key, _Val, traits> const_iterator;
    //typedef aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;
    //typedef aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    // node pointer 
    typedef typename components::node_ptr       node_ptr;
    typedef typename components::inner_node_ptr inner_node_ptr;
    typedef typename components::hash_node_ptr  hash_node_ptr;
    typedef typename components::dense_node_ptr dense_node_ptr;
    typedef typename components::data_node_ptr  data_node_ptr;
    typedef typename components::InnerNodeModel InnerNodeModel;
    typedef InnerNodeModel Model;

    typedef typename traits::slot_type   slot_type;
    typedef typename traits::bitmap      bitmap;
    typedef typename traits::bitmap_base bitmap_base;

    struct aex_stats{
        size_type size;
    };

    struct operation_stats{
        //operation_stats():hash_node_rebuild_cnt(0),  cast_to_hash_node_cnt(0),  hash_node_expand_cnt(0),  hash_node_narrow_cnt(0),
        //                  dense_node_rebuild_cnt(0), cast_to_dense_node_cnt(0), dense_node_expand_cnt(0), dense_node_narrow_cnt(0),
        //                  hash_node_split_cnt(0), hash_node_construct_cnt(0), dense_node_split_cnt(0), dense_node_construct_cnt(0),
        //                  data_node_split_cnt(0), data_node_merge_cnt(0){}
        operation_stats() = default;
        size_t inner_node_rebuild_cnt, inner_node_rebuild_size;
        size_t cast_to_hash_node_cnt, hash_node_expand_cnt, hash_node_expand_size, hash_node_narrow_cnt, hash_node_narrow_size;
        size_t cast_to_dense_node_cnt, dense_node_expand_cnt, dense_node_narrow_cnt;
        size_t inner_node_split_cnt, hash_node_construct_cnt, dense_node_split_cnt, dense_node_construct_cnt;
        size_t data_node_split_cnt, data_node_merge_cnt;
        size_t model_train_cnt, model_train_size;
        size_t allocate_data_node_cnt, allocate_dense_node_cnt, allocate_hash_node_cnt;
        size_t free_data_node_cnt, free_dense_node_cnt, free_hash_node_cnt;
        void print_stats(){
            AEX_IMPORTANT("inner_node_rebuild_cnt=" << inner_node_rebuild_cnt << ", inner_node_rebuild_size=" << inner_node_rebuild_size);
            AEX_IMPORTANT("cast_to_hash_node_cnt="  << cast_to_hash_node_cnt  << ", cast_to_dense_node_cnt="  << cast_to_dense_node_cnt);
            AEX_IMPORTANT("hash_node_expand_cnt="   << hash_node_expand_cnt   << ", hash_node_expand_size="   << hash_node_expand_size);
            AEX_IMPORTANT("hash_node_narrow_cnt="   << hash_node_expand_cnt   << ", hash_node_narrow_size="   << hash_node_narrow_size);
            AEX_IMPORTANT("dense_node_expand_cnt="  << dense_node_expand_cnt  << ", dense_node_expand_size="  << dense_node_expand_size);
            AEX_IMPORTANT("dense_node_narrow_cnt="  << dense_node_expand_cnt  << ", dense_node_narrow_size="  << dense_node_narrow_size);
            AEX_IMPORTANT("data_node_split_cnt="    << data_node_split_cnt    << ", data_node_merge_cnt="     << data_node_merge_cnt);
            AEX_IMPORTANT("model_train_cnt="        << model_train_cnt        << ", model_train_size="        << model_train_size);
            AEX_IMPORTANT("allocate_data_node_cnt=" << allocate_data_node_cnt << ", allocate_dense_node_cnt=" << allocate_dense_node_cnt << ", allocate_hash_node_cnt=" << allocate_hash_node_cnt);
            AEX_IMPORTANT("free_data_node_cnt="     << free_data_node_cnt     << ", free_dense_node_cnt="     << free_dense_node_cnt  << ", free_hash_node_cnt=" << free_hash_node_cnt);
        }
    }opt_stats;
    operation_stats& get_opt_stats(){return opt_stats;}

    struct info_stats{
        //infomation_stats():hash_node_cnt(0), dense_node_cnt(0), data_node_cnt(0);
        info_stats() = default;
        size_t hash_node_cnt, dense_node_cnt, try_learn_dense_node_cnt, data_node_cnt;
        size_t hash_node_childs, dense_node_childs;
        size_t tot_depth, size, max_depth;
        size_t level_node[traits::MAX_DEPTH];
        size_t memory_used;
        void print_stats(){
            AEX_HINT("memory used=" << memory_used);
            AEX_HINT("tot_cnt=" << hash_node_cnt + dense_node_cnt + data_node_cnt);
            AEX_HINT("hash_node_cnt=" << hash_node_cnt << ", dense_node_cnt=" << dense_node_cnt << ", try_learn_dense_node_cnt" << try_learn_dense_node_cnt << ", data_node_cnt=" << data_node_cnt);
            AEX_HINT("hash_node_childs=" << hash_node_childs << ", dense_node_childs=" << dense_node_childs);
            AEX_HINT("size=" << size << ", avg_depth=" << 1.0 * tot_depth / size << ", max_depth=" << max_depth);
            for (int i = 0; i < traits::MAX_DEPTH; ++i)
                AEX_HINT("level " << i << "=" << level_node[i]);
        }
    };
    void _get_info_stats(const node_ptr node, const int depth, info_stats& stats);
    info_stats get_info_stats();

#ifndef AEX_DEBUG
private:
#endif

    //volatile node_ptr root;
    //atomicPtr     root;
    version_type  version;
    node_ptr      root;
    data_node_ptr head_leaf;
    data_node_ptr tail_leaf;
    data_node_ptr empty_leaf;
    aex_stats     m_stats;
    HashTable     hash_table;

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
        //this->balance_stats = _index.balance_stats;
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
        this->allocator = _index.allocator;
        this->hash_table = std::move(_index.hash_table);
        return *this;
    }

    void clear(){
        XL();
        this->deconstruct(this->root);
        this->m_stats = aex_stats();
        this->root = this->head_leaf = this->tail_leaf = nullptr;
        XU();
    }

    /**
     * @brief insert kv_pair into index
     * @warning the interface not support concurrency
     */
    std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    /**
     * @brief insert kv_pair into index
     * @details the interface support concurrency
     */
    bool insert_con(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second).second;
    }

    std::pair<iterator, bool> insert(const key_type &key, const value_type &value);

    /**
     * @brief find the iterator of the key
     * @warning the interface not support concurrency
     */
    inline const_iterator find(const key_type &x) const {
        iterator it = find_iterator(x);
        if (it == end() || it.key() != x) 
            return end();
        return it;
    }

    inline iterator find(const key_type &x){
        iterator it = find_iterator(x);
        if (it == end() || it.key() != x) 
            return end();
        return it;
    }

    /**
     * @brief find the value of key $x$
     * @details the interface support concurrency
     */
    inline bool find(const key_type &x, value_type &y){
        bool ret = false;
        SL();
        data_node_ptr node = find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos < node->size && node->key[pos] == ret){
            y = node->data[pos];
            ret = true;
        }
        SU(node);
        SU();
        return true;
    }

    /**
     * @brief find the value of key $x$
     * @details the interface support concurrency
     */
    void range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer) const ;

    /**
     * @brief find the value of key $x$
     * @details the interface support concurrency
     */
    size_t count(const key_type &x) const {
        value_type _;
        if (find(x, _)) return 1;
        return 0;
    }

    /**
     * @brief check the key $x$ exists
     * @details the interface support concurrency
     */
    bool exists(const key_type &x) {
        value_type _;
        return find(x, _);
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    bool lower_bound(const key_type &x, const std::pair<key_type, value_type> &res){
        SL();
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= node->size && node->next == nullptr){
            SU(node);
            SU();
            return false;
        }
        else{
            SL(node->next);
            SU(node);
            node = node->next;
            pos = node->find_lower_pos(x);
        }
        res = std::make_pair(node->key[pos], node->child[pos]);
        SU(node);
        SU();
        return true;
    }

    /**
     * @brief find the minimum key of iterator larger than or equal to x
     * @warning the interface not support concurrency
     */
    const_iterator lower_bound(const key_type &x){
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= node->size)
            return end();
        return const_iterator(node, pos);
    }

    /**
     * @brief find the minimum key of iterator larger than x
     * @warning the interface not support concurrency
     */
    const_iterator upper_bound(const key_type &x){
        //this->balance_stats.update_timestamp();
        AEX_ASSERT(traits::AllowConcurrency == false);
        iterator iter = find_iterator(x);
        while (iter.key() <= x) 
            ++iter;
        return iter;
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    bool upper_bound(const key_type &x, const std::pair<key_type, value_type> &res){
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_upper_pos(x);
        if (pos >= node->size && node->next == nullptr){
            SU(node);
            return false;
        }
        else{
            SL(node->next);
            SU(node);
            node = node->next;
            pos = node->find_upper_pos(x);
        }
        res = std::make_pair(node->key[pos], node->child[pos]);
        SU(node);
        return true;
    }

    /**
     * @brief erase the data which key equal to x
     * @details the interface support concurrency
     */
    size_t erase(const key_type &x){
        if (root == nullptr) return 0;
        size_type cnt = 0;
        while (true){
            if (erase_one(x)) ++cnt;
            else break;
        }
        return cnt;
    }

    bool erase_one(const key_type &x){
        bool res = this->_erase(x);
        if (res)
            --this->m_stats.size;
        return res;
    }

    /**
     * @brief erase the iterator
     * @warning the interface not support concurrency
     */
    inline void erase(const_iterator &iter){
        if (root == nullptr || iter == end()) 
            return end();
        erase_iterator(iter);
    }
    
    /**
     * @brief iterator like STL-set
     * @warning the interface not support concurrency
     */
    inline iterator begin() {
        return iterator(head_leaf, 0);
    }
    inline const_iterator begin() const {
        return const_iterator(head_leaf, 0);
    }
    inline iterator end() {
        return iterator(empty_leaf, 0);
    }
    inline const_iterator end() const {
        return const_iterator(empty_leaf, 0);
    }

    //inline reverse_iterator rbegin() {
    //    return reverse_iterator(end());
    //}
    //inline const_reverse_iterator rbegin() const {
    //    return const_reverse_iterator(end());
    //}
    //inline reverse_iterator rend() {
    //    return reverse_iterator(begin());
    //}
    //inline const_reverse_iterator rend() const {
    //    return reverse_iterator(begin());
    //}

    inline size_t size() const {
        //return static_cast<size_t>(m_stats.size);
        return m_stats.size;
    }

    inline bool empty() const {
        return m_stats.size == 0;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_t nums);

    inline const aex_stats& get_stats() const{
        SL();
        auto res = m_stats;
        SU();
        return res;
    }

#ifndef AEX_DEBUG
protected:

private:    
#endif
     
    // ========== 0. construction / init ==========
    // below function implemention at 'aex_init.hpp'
    void init();
    node_ptr construct(self& other, const node_ptr node);
    node_ptr construct(const key_type* key, const node_ptr* node, const size_t n);
    void construct(hash_node_ptr  node, const key_type* key, const node_ptr *childs, const size_t n);
    void construct(dense_node_ptr node, const key_type* key, const node_ptr *childs, const size_t n, const Model &m);
    void construct(dense_node_ptr node, const key_type* key, const node_ptr *childs, const size_t n);
    void construct(inner_node_ptr node, const key_type* key, const node_ptr *childs, const size_t n);

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

    // below function implemention at 'aex_find.hpp'

    /**
     * @brief 
     * @details keep shared_lock of returned data node;
     */
    inline data_node_ptr find_leaf(const key_type &key) const ;
    inline data_node_ptr find_leaf_con(const key_type &key) const ;
    
    /**
     * @brief 
     * @details keep shared_lock from root to returned data node;
     */
    // inline data_node_ptr find_leaf_with_stack(const key_type &key, inner_node_ptr* stack, int &top) const;

    /**
     * @brief find the child node from parent lower bound search with key.
     * @warning these funciton not support concurrency
     */
    node_ptr find(const inner_node_ptr node, const key_type &key) const ;
    node_ptr find(const hash_node_ptr  node, const key_type &key) const ;
    node_ptr find(const dense_node_ptr node, const key_type &key) const ;

    /**
     * @brief find the child node from parent lower bound search with key.
     * @warning these funciton support optimistic concurrency
     */
    node_ptr find_con(const inner_node_ptr node, const key_type &key) const;
    node_ptr find_con(const hash_node_ptr  node, const key_type &key) const;
    node_ptr find_con(const dense_node_ptr node, const key_type &key) const;

    node_ptr find_update(inner_node_ptr node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const;
    node_ptr find_update(hash_node_ptr  node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const;
    node_ptr find_update(dense_node_ptr node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const;
    // ========== 2. insert ==========
    // below function implemention at 'aex_insert.hpp'
    /**
     * @brief insert a child node into parent node.
     */
    void construct_tmp_node(dense_node_ptr node, const key_type &old_key, const node_ptr old_node, const key_type &new_key, const node_ptr new_node);
    void __construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child);
    void __insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child);
    void insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child);
    void insert(dense_node_ptr node, const key_type &key, const node_ptr child);
    bool check_insert_SMO(node_ptr node);
    bool insert_lock(inner_node_ptr node, const slot_type pos, const slot_type next_pos);
    bool check_upgrade_and_lock(hash_node_ptr node, const key_type &key, const slot_type pos, const slot_type next_pos, bool &restart);
    bool insert_init(const key_type &key, const value_type &value, std::pair<iterator, bool> &res);
    std::pair<iterator, bool> insert_data_node(data_node_ptr node, data_node_ptr new_node, const key_type &key, const value_type &value);
    void insert_unlock(hash_node_ptr top_node, const slot_type top_pos, const slot_type top_next_pos, node_ptr node, const slot_type pos, const slot_type next_pos, node_ptr child);
    void split(data_node_ptr old_node, data_node_ptr new_node);

    // ========== 3. erase ==========
    // below function implemention at 'aex_erase.hpp'

    bool _erase(const key_type &key);
    void erase(hash_node_ptr node, const slot_type prev_pos, const slot_type pos, const slot_type next_pos);
    void erase(dense_node_ptr node, const slot_type pos);
    // erase one iterator
    void erase_iterator(const_iterator &iter);
    bool erase_init(const key_type &key, bool &erase_flag);
    /**
     * @brief check node need do structual modification operation
     * @return true don't restart
     * @return false need restart find child(node)
     */
    bool check_erase_SMO(node_ptr node);
    bool erase_lock(inner_node_ptr node, const slot_type pos, const slot_type next_pos);
    void erase_unlock(inner_node_ptr node, const slot_type pos, const slot_type next_pos, node_ptr child);
    bool merge(data_node_ptr left_node, data_node_ptr right_node);
    bool erase_tail_leaf_node(inner_node_ptr parent, data_node_ptr child, const slot_type pos, const slot_type next_pos);

    // ========== 4. Structure Modify Operation(SMO) ==========
    // below function implemention at 'aex_SMO.hpp'

    // split a ordered key array with data array to node array.
    void split_to_static_data_node(const key_type* const key, const value_type* const data, const size_t n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    void cast_to_hash_node(inner_node_ptr node,  const slot_type slot_size);
    void cast_to_dense_node(inner_node_ptr node, const slot_type slot_size);

    void update(hash_node_ptr parent,  const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node);
    void update(dense_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node);
    void update(inner_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node);
    /**
     * @brief expand a node slot size
     */
    void expand(inner_node_ptr node);
    void expand(hash_node_ptr  node);
    void expand(dense_node_ptr node);
    /**
     * @brief narrow a node slot size
     */
    void narrow(inner_node_ptr node);
    void narrow(hash_node_ptr  node);
    void narrow(dense_node_ptr node);
    /**
     * @brief place split key of node into corresponding slot of hash node.
     */
    void split(hash_node_ptr node, inner_node_ptr split_child, const slot_type start_pos, const slot_type end_pos);
    void construct_SMO(hash_node_ptr node, const key_type* const keys, const node_ptr* const childs, const size_t n);
    void _get_childs(hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void get_childs(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    slot_type train(const key_type* const keys, const size_t n, Model &m);
    void rebuild(inner_node_ptr node);

    /**
     * @brief check a node need to expand
     */
    bool isfull(const inner_node_ptr node){
        switch(node->type){
            case NodeType::HashNode  : { return isfull(h_n(node)); }
            case NodeType::DenseNode : { return isfull(d_n(node)); }
            default : { AEX_ASSERT(0 == 1); return false;}
        }
    }
    bool isfull(const hash_node_ptr node){
        return 1.0 * node->size / (node->slot_size - node->first_pos) >= traits::HASH_NODE_FULL_RATIO;
    }
    bool isfull(const dense_node_ptr node){
        return node->size == node->slot_size;
    }

    /**
     * @brief check a node need to narrow
     */
    bool isfew(const inner_node_ptr node){
        switch (node->type){
            case NodeType::HashNode  : { return isfew(h_n(node)); }
            case NodeType::DenseNode : { return isfew(d_n(node)); }
            default : { AEX_ASSERT(0 == 1); }
        }
    }
    bool isfew(const hash_node_ptr node){
        return 1.0 * node->size / node->slot_size < traits::HASH_NODE_FEW_RATIO || node->size < traits::MIN_HASH_NODE_SIZE;
    }
    bool isfew(const dense_node_ptr node){
        return node->size < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    bool is_rebuild(const inner_node_ptr node){
        bool res = false;
        key_type key;
        node_ptr child;
        if (node->type == NodeType::HashNode){
            h_n(node)->lock_shared(node->slot_size - 1, node->slot_size - 1);
            std::tie(key, child) = hash_table.find(node, node->prev_item_find(node->slot_size - 1));
        }
        else
            child = d_n(node)->child_ptr[node->size - 1];
        SL(child);
        if (child->size >= node->size * traits::MIN_REBUILD_RATIO)
            res = true;
        if (node->type == NodeType::HashNode)
            h_n(node)->unlock_shared(node->slot_size - 1, node->slot_size - 1);
        SU(child);
        return false;
    }

    bool check_density(key_type array_gap_key, size_t n, key_type node_gap_key, size_t m){
        if (m >= n / 2)
            return true;
        if (1.0 * node_gap_key / m < 1.0 * array_gap_key / n)
            return true;
        return false;
    }

    // ========== 5. Utility ==========
    inline bool isfull(const data_node_ptr node) const {
        return node->size >= traits::MIN_DATA_NODE_SLOT_SIZE;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline void clear(inner_node_ptr node){
        if (node->type == NodeType::HashNode){
            for (slot_type i = 0; i < node->slot_size; i = node->next_item_find(i + 1))
                hash_table.erase(node, i);
            h_n(node)->clear();
        }
        else
            d_n(node)->clear();
    }

    inline void free_node(node_ptr node){
        switch (node->type){
            case NodeType::LeafNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_data_node_cnt++;
                #endif
                delete node;
                break;
            }
            case NodeType::DenseNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_dense_node_cnt++;
                #endif
                delete node;
                break;
            }
            case NodeType::HashNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_hash_node_cnt++;
                #endif
                clear(node);
                delete node;
                break;
            }
        }
    }

    // ========== 6. concurrency ==========
    // SL: shared_lock          SU: shared_unlock
    // XL: lock                 XU: unlock
    // UL: upgrade_lock(SL->XL) DL: downgrade_lock(XL->SL)
    inline void SL(node_ptr  node) const { node->node_lock.lock_shared();     }
    inline void SU(node_ptr  node) const { node->node_lock.unlock_shared();   }
    inline void XL(node_ptr  node) const { node->node_lock.lock();            }
    inline void XU(node_ptr  node) const { node->node_lock.unlock();          }
    inline void UL(node_ptr  node) const { node->node_lock.upgrade_lock();    }
    inline void DL(node_ptr  node) const { node->node_lock.downgrade_lock();  }
    inline bool TSL(node_ptr node) const { return node->node_lock.try_lock_shared(); }
    inline bool TXL(node_ptr node) const { return node->node_lock.try_lock();        }
    inline bool TUL(node_ptr node) const { return node->node_lock.try_upgrade_lock();}
    inline void SL() {this->mtx.lock_shared();}
    inline void SU() {this->mtx.unlock_shared();}
    inline void XL() {this->mtx.lock();}
    inline void XU() {this->mtx.unlock();}
    inline bool check_lock(node_ptr node) const {
        if constexpr (traits::AllowConcurrency)
            return node->node_lock.is_lock();
        else
            return true;
    }
    inline bool check_lock_shared(node_ptr node) const {
        if constexpr (traits::AllowConcurrency)
            return node->node_lock.is_lock_shared();
        else
            return true;
    }
    inline bool check_unlock(node_ptr node) const {
        if constexpr (traits::AllowConcurrency)
            return !node->node_lock.is_lock();
        else
            return true;
    }
    inline bool check_unlock_shared(node_ptr node) const {
        if constexpr (traits::AllowConcurrency)
            return !node->node_lock.is_lock_shared();
        else
            return true;
    }
};
}

#include "aex/aex_init.hpp"
#include "aex/aex_find.hpp"
#include "aex/aex_insert.hpp"
#include "aex/aex_erase.hpp"
#include "aex/aex_SMO.hpp"
#include "aex/aex_helper.hpp"