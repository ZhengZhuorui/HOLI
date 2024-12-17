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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

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
    typedef typename components::version_type version_type;
    typedef typename components::base_node    base_node;
    typedef typename components::inner_node   inner_node;
    typedef typename components::data_node    data_node;
    typedef typename components::hash_node    hash_node;
    typedef typename components::dense_node   dense_node;
    typedef typename components::Allocator    Allocator;
    typedef typename components::HashTable    HashTable;
    typedef typename components::bitmap_impl  bitmap_impl;
    typedef typename components::Lock         Lock;
    typedef typename components::RWLock       RWLock;

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
    typedef typename components::size_type      size_type;
    typedef typename components::atomic_size_type      atomic_size_type;
    typedef InnerNodeModel Model;

    typedef typename traits::slot_type   slot_type;
    typedef typename traits::bitmap      bitmap;
    typedef typename traits::bitmap_base bitmap_base;

    struct aex_stats{
        aex_stats() = default;
        ~aex_stats() = default;
        aex_stats& operator = (aex_stats& stats){
            size = stats.size.load();
            return *this;
        }
        atomic_size_type size;
    };

    operation_stats& get_opt_stats() const {return const_cast<operation_stats&>(opt_stats);}
    void _get_info_stats(const node_ptr node, const unsigned int depth, info_stats& stats) const ;
    info_stats get_info_stats() const ;

    void print_stats() const {
        AEX_HINT("size=" << m_stats.size.load());
        auto if_stats = get_info_stats();
        if_stats.print_stats();
        opt_stats.print_stats();
        hash_table.print_stats();
    }

    //size_t memory_used() const;

#ifndef AEX_DEBUG
private:
#endif

    version_type    version;
    inner_node_ptr  root;
    data_node_ptr   head_leaf;
    aex_stats       m_stats;
    HashTable       hash_table;
    operation_stats opt_stats;
    mutable RWLock  mtx;

public:
    aex_tree();
    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);
    aex_tree(const self& _index);
    aex_tree(self&& _index);
    ~aex_tree();

    inline aex_tree& operator = (aex_tree &_index){
        _index.XL();
        XL();
        this->init();
        data_node_ptr _tail_leaf;
        this->root = i_n(this->construct(_index, _index.root, _tail_leaf));
        this->m_stats = _index.m_stats;
        XU();
        _index.XU();
        return *this;
    }

    inline aex_tree& operator = (aex_tree &&_index){
        _index.XL();
        XL();
        this->init();
        this->root = _index.root;
        _index.root = nullptr;
        this->head_leaf = _index.head_leaf;
        _index.head_leaf = nullptr;
        this->allocator = _index.allocator;
        this->hash_table = std::move(_index.hash_table);
        XU();
        _index.XU();
        return *this;
    }

    inline void clear(){
        XL();
        this->init_index();
        XU();
    }

    /**
     * @brief insert kv_pair into index
     * @warning the interface not support concurrency
     */
    inline std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second);
    }
    /**
     * @brief insert kv_pair into index
     * @details the interface support concurrency
     */
    inline bool insert_con(const std::pair<key_type, value_type> &x){
        return insert(x.first, x.second).second;
    }
    inline bool insert_con(const key_type &key, const value_type &value){
        return insert(key, value).second;
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
        if (pos < node->size && node->key[pos] == x){
            y = node->data[pos];
            ret = true;
        }
        SU(node);
        SU();
        return ret;
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
    inline ULL count(const key_type &x) {
        value_type _;
        if (find(x, _)) return 1;
        return 0;
    }

    /**
     * @brief check the key $x$ exists
     * @details the interface support concurrency
     */
    inline bool exists(const key_type &x) {
        value_type _;
        return find(x, _);
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    inline bool lower_bound(const key_type &x, const std::pair<key_type, value_type> &res){
        SL();
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= node->size && node->next == nullptr){
            SU(node);
            SU();
            return false;
        }
        else{
            data_node_ptr next_node = node->next;
            SL(next_node);
            SU(node);
            node = next_node;
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
    inline iterator lower_bound(const key_type &x){
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= (LL)node->size)
            return end();
        return iterator(node, pos);
    }

    inline const_iterator lower_bound(const key_type &x) const {
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= (LL)node->size)
            return end();
        return iterator(node, pos);
    }

    /**
     * @brief find the minimum key of iterator larger than x
     * @warning the interface not support concurrency
     */
    inline iterator upper_bound(const key_type &x){
        AEX_ASSERT(traits::AllowConcurrency == false);
        iterator iter = find_iterator(x);
        while (iter.key() <= x) 
            ++iter;
        return iter;
    }

    inline const_iterator upper_bound(const key_type &x)const {
        AEX_ASSERT(traits::AllowConcurrency == false);
        const_iterator iter = find_iterator(x);
        while (iter.key() <= x) 
            ++iter;
        return iter;
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    inline bool upper_bound(const key_type &x, const std::pair<key_type, value_type> &res){
        SL();
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_upper_pos(x);
        if (pos >= node->size && node->next == nullptr){
            SU(node);
            SU();
            return false;
        }
        else{
            data_node_ptr next_node = node->next;
            SL(next_node);
            SU(node);
            node = next_node;
            pos = node->find_upper_pos(x);
        }
        res = std::make_pair(node->key[pos], node->child[pos]);
        SU(node);
        SU();
        return true;
    }

    /**
     * @brief erase the data which key equal to x
     * @details the interface support concurrency
     */
    inline ULL erase(const key_type &x){
        if (root == nullptr) return 0;
        ULL cnt = 0;
        if constexpr (traits::AllowMultiKey)
            return _erase(x);
        else{
            while (true){
                if (erase_one(x)) ++cnt;
                else break;  
            }
        }
        //while (true){
        //    if (erase_one(x)) ++cnt;
        //    else break;
        //}
        return cnt;
    }

    inline bool erase_one(const key_type &x){
        bool res = this->_erase(x);
        if (res)
            --this->m_stats.size;
        return res;
    }

    /**
     * @brief erase the iterator
     * @warning the interface not support concurrency
     */
    inline void erase(iterator &iter){
        if (root == nullptr || iter == end()) 
            return end();
        this->erase(iter.key);
    }
    
    /**
     * @brief iterator like STL-set
     * @warning the interface not support concurrency
     */
    inline iterator begin() {
        data_node_ptr p = head_leaf;
        while (p != nullptr && p->size == 0)
            p = p->next;
        return iterator(p, 0);
    }
    inline const_iterator begin() const {
        data_node_ptr p = head_leaf;
        while (p != nullptr && p->size == 0)
            p = p->next;
        return const_iterator(p, 0);
    }
    inline iterator end() {
        return iterator(nullptr, 0);
    }
    inline const_iterator end() const {
        return const_iterator(nullptr, 0);
    }

    inline ULL size() const {
        //return static_cast<size_t>(m_stats.size);
        //return m_stats.size;
        return static_cast<size_t>(m_stats.size.load());
    }

    inline bool empty() const {
        return m_stats.size == 0;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const ULL nums);

    inline const aex_stats& get_stats() {
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
    void init_index();
    node_ptr construct(self& other, const node_ptr node, data_node_ptr &tail_leaf);
    inner_node_ptr construct(const key_type* key, const node_ptr* node, const ULL n);
    void construct_hash_node(hash_node_ptr  node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct_dense_node(dense_node_ptr node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct_simple(dense_node_ptr node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct(inner_node_ptr node, const key_type* key, node_ptr *childs, const ULL n);
    void deconstruct(node_ptr node);

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
        if (pos == (LL)node->size)
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

    node_ptr find_update(inner_node_ptr node, const key_type &key, slot_type &pos, slot_type &next_pos) const ;
    node_ptr find_update(hash_node_ptr  node, const key_type &key, slot_type &pos, slot_type &next_pos) const ;
    node_ptr find_update(dense_node_ptr node, const key_type &key, slot_type &pos, slot_type &next_pos) const ;

    data_node_ptr find_tail_leaf(node_ptr node) const ;
    inline key_type node_zero_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[0]) : (hash_table.find(node, 0).first);
    }
    inline key_type node_first_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[1]) : (hash_table.find(node, h_n(node)->next_item(1)).first);
    }
    inline key_type node_last_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[node->size - 1]) : (hash_table.find(node, h_n(node)->next_item(h_n(node)->slot_size - 1)).first);
    }
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
    bool check_insert_SMO(inner_node_ptr node);
    bool check_upgrade_and_lock(hash_node_ptr node, const slot_type split_pos, const slot_type top_pos, const slot_type top_next_pos, bool &restart) const ;
    std::pair<iterator, bool> insert_data_node(data_node_ptr node, data_node_ptr &new_node, const key_type &key, const value_type &value);
    void insert_unlock(hash_node_ptr top_node, const slot_type top_pos, const slot_type top_next_pos, node_ptr node, const slot_type pos, const slot_type next_pos) const;
    void split(data_node_ptr old_node, data_node_ptr new_node);

    // ========== 3. erase ==========
    // below function implemention at 'aex_erase.hpp'

    bool _erase(const key_type &key);
    void erase(hash_node_ptr node, const slot_type pos, const slot_type next_pos);
    void erase(dense_node_ptr node, const slot_type pos);
    /**
     * @brief check node need do structual modification operation
     * @return true don't restart
     * @return false need restart find child(node)
     */
    bool check_erase_SMO(node_ptr node);
    bool erase_lock(inner_node_ptr node, const slot_type pos, const slot_type next_pos) const ;
    void erase_unlock(inner_node_ptr node, const slot_type pos, const slot_type next_pos) const ;
    void merge(data_node_ptr left_node, data_node_ptr right_node);

    // ========== 4. Structure Modify Operation(SMO) ==========
    // below function implemention at 'aex_SMO.hpp'

    // split a ordered key array with data array to node array.
    void split_to_static_data_node(const key_type* const key, const value_type* const data, const ULL n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
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
    slot_type split(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos);
    void construct_SMO(hash_node_ptr node, const key_type* const keys, node_ptr* childs, const ULL n);
    void get_childs(hash_node_ptr node,  std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void get_childs(dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void get_childs(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void extend_head_nodes(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void extend_tail_nodes(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    bool check_model(const Model &m, const key_type* const keys, const ULL n, const slot_type slot_size) const ;
    slot_type train(const key_type* const keys, const ULL n, Model &m);
    void rebuild(inner_node_ptr node);
    bool check_extend_head(const key_type fk, const key_type lk, const slot_type node_size, const node_ptr child) const ;
    bool check_extend_tail(const key_type fk, const key_type lk, const slot_type node_size, const node_ptr child) const ;
    void extend(const inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);

    /**
     * @brief check a node need to expand
     */
    inline bool isfull(const inner_node_ptr node) const {
        if (node->type == NodeType::HashNode)
            return isfull(h_n(node));
        else
            return isfull(d_n(node));
    }
    inline bool isfull(const hash_node_ptr node) const {
        AEX_ASSERT(check_lock(node) || check_lock_shared(node));
        return 1.0 * node->size / node->slot_size >= traits::HASH_NODE_FULL_RATIO && node->slot_size < traits::MAX_HASH_NODE_SLOT_SIZE;
    }

    inline bool isfull(const dense_node_ptr node) const {
        return node->size >= node->slot_size;
    }

    /**
     * @brief check a node need to narrow
     */
    inline bool isfew(const inner_node_ptr node) const {
        if (node->type == NodeType::HashNode)
            return isfew(h_n(node));
        else
            return isfew(d_n(node));
    }
    inline bool isfew(const hash_node_ptr node) const {
        return 1.0 * node->size < node->slot_size * traits::HASH_NODE_FEW_RATIO / 2 || node->size < traits::MAX_DENSE_NODE_SLOT_SIZE / 2;
    }
    inline bool isfew(const dense_node_ptr node) const {
        if (node->slot_size == traits::MIN_DATA_NODE_SLOT_SIZE)
            return false;
        return node->size < node->slot_size * traits::DENSE_NODE_FEW_RATIO;
    }

    inline bool is_rebuild(const inner_node_ptr node) const {
        return false;
        bool res = false;
        key_type key;
        node_ptr child1, child2;
        slot_type tot_size = 0, node_size = node->size;
        if (node->size <= 1)
            return false;
        if (node->type == NodeType::HashNode){
            h_n(node)->array_lock_shared(0, 0);
            std::tie(key, child1) = hash_table.find(node, 0);
            SL(child1);
            h_n(node)->array_unlock_shared(0, 0);
            h_n(node)->array_lock_shared(node->slot_size - 1, node->slot_size - 1);
            std::tie(key, child2) = hash_table.find(node, h_n(node)->prev_item_find(node->slot_size - 1));
            SL(child2);
            h_n(node)->array_unlock_shared(node->slot_size - 1, node->slot_size - 1);
        }
        else{
            child1 = d_n(node)->child_ptr[0];
            child2 = d_n(node)->child_ptr[d_n(node)->size - 1];
            SL(child1);
            SL(child2);
        }
        if (child1->type != NodeType::LeafNode)
            tot_size += child1->size;
        if (child2->type != NodeType::LeafNode)
            tot_size += child2->size;
        if (tot_size >= node_size * traits::MIN_REBUILD_RATIO)
            res = true;
        SU(child1);
        SU(child2);
        return res;
    }

    // ========== 5. Utility ==========

    inline bool isfull(const data_node_ptr node) const {
        return node->size >= traits::MIN_DATA_NODE_SLOT_SIZE;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < traits::MIN_DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline void clear(hash_node_ptr node){
        for (slot_type i = node->prev_item_find(node->slot_size - 1); i >= 0; i = node->prev_item_find(i - 1)){
            hash_table.erase(node, i);
            if (i == 0)
                break;
        }
        h_n(node)->clear();
    }

    inline void clear(inner_node_ptr node){
        if (node->type == NodeType::DenseNode)
            d_n(node)->clear();
        else
            clear(h_n(node));
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
                d_n(node)->clear();
                free(node);
                break;
            }
            case NodeType::HashNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_hash_node_cnt++;
                #endif
                clear(h_n(node));
                free(node);
                break;
            }
        }
        node = nullptr;
    }

    // ========== 6. concurrency ==========
    // SL: shared_lock          SU: shared_unlock
    // XL: lock                 XU: unlock
    // UL: upgrade_lock(SL->XL) DL: downgrade_lock(XL->SL)
    inline void SL(node_ptr  node) const { node->node_lock.lock_shared(); }
    inline void SU(node_ptr  node) const { AEX_ASSERT(check_lock_shared(node)); AEX_ASSERT(check_unlock(node)); node->node_lock.unlock_shared(); }
    inline void SU(hash_node_ptr node, slot_type l_pos, slot_type r_pos) const { AEX_ASSERT(check_lock_shared(node)); AEX_ASSERT(check_unlock(node));node->array_unlock_shared(l_pos, r_pos); SU(node); }
    inline void SU(inner_node_ptr node, slot_type l_pos, slot_type r_pos) const {
        if (node->type == NodeType::HashNode) SU(h_n(node), l_pos, r_pos);
        else SU(node);
    }
    inline void XL(node_ptr  node) const { node->node_lock.lock(); }
    inline void XU(node_ptr  node) const { AEX_ASSERT(check_lock(node)); AEX_ASSERT(check_unlock_shared(node)); node->node_lock.unlock(); }
    inline void XU(hash_node_ptr node, slot_type l_pos, slot_type r_pos) const { AEX_ASSERT(check_lock_shared(node)); node->array_unlock(l_pos, r_pos); SU(node); }
    inline void XU(inner_node_ptr node, slot_type l_pos, slot_type r_pos) const {
        if (node->type == NodeType::HashNode) XU(h_n(node), l_pos, r_pos);
        else XU(node);
    }
    inline void UL(node_ptr  node) const { AEX_ASSERT(check_lock_shared(node)); node->node_lock.upgrade_lock(); }
    inline void DL(node_ptr  node) const { AEX_ASSERT(check_lock(node)); AEX_ASSERT(check_unlock_shared(node)); node->node_lock.downgrade_lock(); }
    inline bool TSL(node_ptr node) const { return node->node_lock.try_lock_shared(); }
    inline bool TXL(node_ptr node) const { return node->node_lock.try_lock();        }
    inline bool TUL(node_ptr node) const { AEX_ASSERT(check_lock_shared(node)); return node->node_lock.try_upgrade_lock(); }
    //inline bool TULF(node_ptr node) const { return node->node_lock.try_upgrade_lock_first();}
    inline void SL() const {this->mtx.lock_shared();}
    inline void SU() const {AEX_ASSERT(check_lock_shared()); this->mtx.unlock_shared();}
    inline void XL() const {this->mtx.lock();}
    inline void XU() const {AEX_ASSERT(check_unlock_shared()); AEX_ASSERT(check_lock()); this->mtx.unlock();}
    inline bool TUL()const {AEX_ASSERT(check_lock_shared()); return this->mtx.try_upgrade_lock();}
    inline void DL() const {AEX_ASSERT(check_lock()); this->mtx.downgrade_lock();}

    // ========== 7. test ==========
    bool check_lock(node_ptr node) const;
    bool check_lock_shared(node_ptr node) const;
    bool check_unlock(node_ptr node) const;
    bool check_unlock_shared(node_ptr node) const;
    bool check_lock() const ;
    bool check_unlock() const;
    bool check_lock_shared() const;
    bool check_unlock_shared() const;
    bool check_node(node_ptr       node) const ;
    bool check_node(data_node_ptr  node) const ;
    bool check_node(dense_node_ptr node) const ;
    bool check_node(hash_node_ptr  node) const ;
};
}

#include "aex/aex_init.hpp"
#include "aex/aex_find.hpp"
#include "aex/aex_insert.hpp"
#include "aex/aex_erase.hpp"
#include "aex/aex_SMO.hpp"
#include "aex/aex_helper.hpp"
#include "aex/aex_test.h"

#undef n_n
#undef i_n
#undef h_n
#undef d_n
#undef l_n
#undef ULL
#undef LL

#pragma GCC diagnostic pop
