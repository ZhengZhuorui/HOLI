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
#include <random>
#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/queue.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"

#include "aex_utils.h"
#include "aex_utils_avx.h"
#include "aex_def.h"
#include "aex_traits.h"
#include "concurrency/aex_lock.h"
#include "concurrency/aex_memory_reclaim.h"
#include "aex_components.h"
#include "aex_hash_table.h"
#include "aex_model.h"
#include "aex_model_avx.h"
#include "aex_node.h"
#include "aex_allocator.h"
#include "aex_iterator.h"
#include "concurrency/aex_node_con.h"
#include "concurrency/aex_hash_table_con.h"

// TODO List:
// concurrent upper bound
// concurrent erase

namespace aex{

template<typename _Key, 
        typename _Val,
        typename traits = aex_default_traits<_Key, _Val>>
class aex_tree{
public:

    //static_assert(!traits::AllowMultiKey, "index doesn't support multi key");

    static_assert(std::is_arithmetic<_Key>::value, "key types must be numeric.");

    // type traits
    typedef _Key key_type;
    typedef _Val value_type;

    // components:
    typedef aex_tree<_Key, _Val, traits>       self;
    typedef aex_default_components<traits>     components;
    typedef typename components::version_type  version_type;
    typedef typename components::base_node     base_node;
    typedef typename components::inner_node    inner_node;
    typedef typename components::data_node     data_node;
    typedef typename components::hash_node     hash_node;
    typedef typename components::dense_node    dense_node;
    typedef typename components::Allocator     Allocator;
    typedef typename components::HashTable     HashTable;
    typedef typename components::bitmap_impl   bitmap_impl;
    typedef typename components::Lock          Lock;
    typedef typename components::RWLock        RWLock;
    //typedef typename components::LockFreeStack LockFreeStack;
    typedef typename components::LockFreeQueue LockFreeQueue;
    typedef typename components::atomic_version_type atomic_version_type;

    // Memory Reclaim
    typedef typename components::MRUnit MRUnit;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;
    typedef typename components::EpochGuard         EpochGuard;
    //typedef typename components::CP ConcurrencyParams;
    typedef typename components::LockArrayParams    LockArrayParams;
    typedef typename components::GetChildsParams    GetChildsParams;
    typedef typename components::ConstructSMOParams ConstructSMOParams;
    typedef typename components::HashTableRescaleParams HashTableRescaleParams;

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
    typedef typename components::ID_type        ID_type;
    typedef typename components::atomic_ID_type atomic_ID_type;
    typedef InnerNodeModel Model;

    typedef typename traits::slot_type   slot_type;
    typedef typename traits::bitmap      bitmap;
    typedef typename traits::bitmap_base bitmap_base;

    // function:

    operation_stats& get_opt_stats() const {return const_cast<operation_stats&>(opt_stats);}
    void _get_info_stats(const node_ptr node, const unsigned int depth, info_stats& stats) const ;
    info_stats get_info_stats() const ;

    void print_stats() const {
        AEX_HINT("size=" << _size);
        auto if_stats = get_info_stats();
        if_stats.print_stats();
        opt_stats.print_stats();
        con_stats.print_stats();
        hash_table.print_stats();
    }

    //size_t memory_used() const;

#ifndef AEX_DEBUG
private:
#endif

    //atomic_version_type    version;
    inner_node_ptr  root;
    data_node_ptr   head_leaf;
    size_type       _size;
    mutable operation_stats opt_stats;
    mutable concurrency_stats con_stats;
    atomic_ID_type  node_id;
    LockFreeQueue   work_queue;

public:
    HashTable       hash_table;
    Allocator       allocator;
    EpochBasedMemoryReclamationStrategy *ebr;


public:
    aex_tree();
    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last);
    aex_tree(const self& _index);
    aex_tree(self&& _index);
    ~aex_tree();

    inline aex_tree& operator = (aex_tree &_index){
        EpochGuard guard(this);
        this->_clear();
        data_node_ptr _tail_leaf;
        this->root = i_n(this->construct(_index, _index.root, _tail_leaf));
        this->_size = _index._size;
        AEX_PRINT(this->_size << ", node_id=" << this->node_id.load() << ", this->root->size=" << this->root->size << ", this->hash_table->size=" << this->hash_table.size);
        return *this;
    }

    inline aex_tree& operator = (aex_tree &&_index){
        this->_clear();
        this->root = _index.root;
        _index.root = nullptr;
        this->head_leaf = _index.head_leaf;
        _index.head_leaf = nullptr;
        this->_size = _index.size;
        this->node_id = _index.node_id;
        this->allocator = _index.allocator;
        this->hash_table = std::move(_index.hash_table);
        return *this;
    }

    inline value_type& operator[](const key_type key){
        //static_assert(std::is_arithmetic<_Key>::value, "key types must be numeric.");
        static_assert(traits::AllowConcurrency == false, "The operator not support concurrency");
        insert(std::pair<key_type, value_type>(key, value_type()));
        data_node_ptr node = find_leaf(key);
        int pos = node->find(key);
        AEX_ASSERT(pos < node->size);
        return node->data[pos];
    }

    inline void clear(){
        this->_clear();
        this->init();
    }

    /**
     * @brief insert kv_pair into index
     * @warning the interface not support concurrency
     */
    //inline std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
    //    static_assert(traits::AllowConcurrency == false, "The operator not support concurrency");
    //    return insert(x.first, x.second);
    //}
    /**
     * @brief insert kv_pair into index
     * @details the interface support concurrency
     */
    inline bool insert(const std::pair<key_type, value_type> x){
        if constexpr(!traits::AllowConcurrency)
            return _insert(x.first, x.second);
        else{
            return _insert_con_helper(x.first, x.second);
        }
    }

    inline bool insert(const key_type key, const value_type &value){
        if constexpr(!traits::AllowConcurrency)
            return _insert(key, value);
        else{
            return _insert_con_helper(key, value);
        }
    }

    inline bool _insert_con_helper(const key_type key, const value_type &value){
        EpochGuard guard(this);
        return _insert_con(key, value);
    }

    //std::pair<iterator, bool> insert(const key_type key, const value_type &value);
    bool _insert(const key_type key, const value_type &value);
    bool _insert_con(const key_type key, const value_type &value);

    /**
     * @brief find the iterator of the key
     * @warning the interface not support concurrency
     */
    inline const_iterator find(const key_type x) const {
        static_assert(traits::AllowConcurrency == false, "The operator not support concurrency");
        data_node_ptr node = find_leaf(x);
        int pos = node->find(x);
        if (pos == node->size)
            return end();         
        return const_iterator(node, pos);
    }

    /**
     * @brief find the value of key $x$
     * @details the interface not support concurrency
     */
    inline iterator find(const key_type x){
        static_assert(traits::AllowConcurrency == false, "The operator not support concurrency");
        data_node_ptr node = find_leaf(x);
        int pos = node->find(x);
        if (pos == node->size)
            return end();         
        return iterator(node, pos);
    }

    /**
     * @brief find the value of key $x$
     * @details the interface support concurrency
     */
    inline bool find(const key_type x, value_type &y){
        if constexpr (traits::AllowConcurrency)
            return find_con(x, y);
        bool ret = false;
        data_node_ptr node = find_leaf(x);
        int pos = node->find(x);
        if (pos < node->size){
            y = node->data[pos];
            ret = true;
        }
        return ret;
    }

    bool find_con(const key_type x, value_type &y);

    /**
     * @brief find the value of key $x$
     * @details the interface support concurrency
     */
    void range_query(const key_type lower_key, const key_type upper_key, std::vector<std::pair<key_type, value_type>>& answer) const ;
    void range_query_con(const key_type lower_key, const key_type upper_key, std::vector<std::pair<key_type, value_type>>& answer)  ;

    size_t range_query_len(std::pair<key_type, value_type>* results, const key_type lower_key, const size_t key_num) const;
    size_t range_query_len_con(std::pair<key_type, value_type>* results, const key_type lower_key, const size_t key_num) ;

    /**
     * @brief update $map_[x]<-y$
     * @details the interface support concurrency
     */
    inline bool update(const key_type x, value_type &y){
        if constexpr (traits::AllowConcurrency)
            return update_con(x, y);
        bool ret = false;
        data_node_ptr node = find_leaf(x);
        slot_type pos = node->find(x);
        if (pos < node->size){
            node->data[pos] = y;
            ret = true;
        }
        return ret;
    }

    inline bool update_con(const key_type x, value_type &y){
        bool ret = false;
        data_node_ptr node;
        version_type node_version;
        int restart_count = 0;
    update_restart:
        AEX_SGL_ASSERT(restart_count == 0);
        if (restart_count > 0){
            yield(restart_count);
        }
        bool need_restart = 0;
        node = find_leaf_con(x, node_version, need_restart);
        if (need_restart) goto update_restart;
        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
        if (need_restart) goto update_restart;
        slot_type pos = node->find(x);
        if (pos < node->size){
            node->data[pos] = y;
            ret = true;
        }
        XU(node);
        return ret;
    }

    /**
     * @brief find the number of  key $x$
     * @details the interface support concurrency
     */
    inline ULL count(const key_type x) {
        value_type _;
        if (find(x, _)) return 1;
        return 0;
    }

    /**
     * @brief check the key $x$ exists
     * @details the interface support concurrency
     */
    inline bool exists(const key_type x) {
        value_type _;
        return find(x, _);
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    inline bool lower_bound(const key_type x, std::pair<key_type, value_type> &res){
        if constexpr (traits::AllowConcurrency)
            return lower_bound_con(x, res);
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= node->size && node->next == nullptr){
            return false;
        }
        else{
            data_node_ptr next_node = node->next;
            node = next_node;
            AEX_ASSERT(node->key[0] >= x);
            //pos = node->find_lower_pos(x);
            pos = 0;
        }
        res = std::make_pair(node->key[pos], node->data[pos]);
        return true;
    }

    bool lower_bound_con(const key_type x, std::pair<key_type, value_type> &res);

    /**
     * @brief find the minimum key of iterator larger than or equal to x
     * @warning the interface not support concurrency
     */
    inline iterator lower_bound(const key_type x){
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= (LL)node->size){
            pos = 0;
            node = node->next;
            while (node != nullptr && node->size == 0)
                node = node->next;
            if (node == nullptr)
                return end();
        }
        return iterator(node, pos);
    }

    inline const_iterator lower_bound(const key_type x) const {
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos >= (LL)node->size){
            pos = 0;
            node = node->next;
            while (node != nullptr && node->size == 0)
                node = node->next;
            if (node == nullptr)
                return end();
        }
        return const_iterator(node, pos);
    }

    /**
     * @brief find the minimum key of iterator larger than x
     * @warning the interface not support concurrency
     */
    inline iterator upper_bound(const key_type x){
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos < node->size && node->key[pos] <= x)
            ++pos;
        if (pos >= (LL)node->size){
            pos = 0;
            node = node->next;
            while (node != nullptr && node->size == 0)
                node = node->next;
            if (node == nullptr)
                return end();
        }
        return iterator(node, pos);
    }

    inline const_iterator upper_bound(const key_type x)const {
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_lower_pos(x);
        if (pos < node->size && node->key[pos] <= x)
            ++pos;
        if (pos >= (LL)node->size){
            pos = 0;
            node = node->next;
            while (node != nullptr && node->size == 0)
                node = node->next;
            if (node == nullptr)
                return end();
        }
        return const_iterator(node, pos);
    }

    /**
     * @brief find the minimum key larger than or equal to x
     * @details the interface support concurrency
     */
    inline bool upper_bound(const key_type x, const std::pair<key_type, value_type> &res){
        if constexpr (traits::AllowConcurrency)
            return upper_bound_con(x, res);
        data_node_ptr node = this->find_leaf(x);
        slot_type pos = node->find_upper_pos(x);
        if (pos >= node->size && node->next == nullptr){
            SU(node);
            return false;
        }
        else{
            data_node_ptr next_node = node->next;
            node = next_node;
            pos = node->find_upper_pos(x);
        }
        res = std::make_pair(node->key[pos], node->child[pos]);
        return true;
    }

    inline bool upper_bound_con(const key_type x, const std::pair<key_type, value_type> &res){
        // TODO
        return false;
    }

    /**
     * @brief erase the data which key equal to x
     * @details the interface support concurrency
     */
    inline ULL erase(const key_type x){
        AEX_ASSERT(traits::AllowErase);
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

    inline bool erase_one(const key_type x){
        AEX_ASSERT(traits::AllowErase);
        if constexpr (traits::AllowConcurrency)
            this->_erase_con_helper(x);
        bool res = this->_erase(x);
        if (res)
            --this->_size;
        return res;
    }

    inline bool _erase_con_helper(const key_type x){
        // TODO
        return false;
    }

    // TODO
    bool _erase_con(const key_type x);

    /**
     * @brief erase the iterator
     * @warning the interface not support concurrency
     */
    inline void erase(iterator &iter){
        static_assert(traits::AllowConcurrency == false, "The operator not support concurrency");
        AEX_ASSERT(traits::AllowErase);
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
        if constexpr (traits::AllowConcurrency){
            auto if_stats = get_info_stats();
            return if_stats.size;
        }
        else
            return static_cast<size_t>(_size);
    }

    inline bool empty() const {
        return _size == 0;
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const ULL nums);


    // ========== 0. memory reclaim interface ==========
    // new hash_node  memory size != Allocator.allocate_hash_node memory size. Hash node copy must not use free node!!! allocator allocate node must not use clear copy!!!
    inline void clear_copy(hash_node_ptr node){
        AEX_ASSERT(traits::AllowConcurrency);
        if constexpr (traits::AllowConcurrency){
            slot_type cnt = 0;
            for (slot_type i = node->prev_item_find(node->slot_size - 1); i >= 0; i = node->prev_item_find(i - 1)){
                this->hash_table.erase(node, i);
                ++cnt;
                if (i == 0)
                    break;
            }
            node->clear();
            delete node;
        }
    }

    inline void free_node(node_ptr node){
        AEX_ASSERT(check_lock(node));
        switch (node->type){
            case NodeType::LeafNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_data_node_cnt++;
                #endif
                delete l_n(node);
                break;
            }
            case NodeType::DenseNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_dense_node_cnt++;
                #endif
                free(node);
                break;
            }
            case NodeType::HashNode:{
                #ifdef AEX_DEBUG
                opt_stats.free_hash_node_cnt++;
                #endif
                clear(h_n(node));
                if constexpr (traits::AllowConcurrency){
                    delete h_n(node)->copy;
                }
                free(node);
                break;
            }
        }
        node = nullptr;
    }

#ifndef AEX_DEBUG
protected:

private:    
#endif
     
    // ========== 0. construction / init ==========
    // below function implemention at 'aex_init.hpp'
    void _clear();
    void init();
    node_ptr construct(self& other, const node_ptr node, data_node_ptr &tail_leaf);
    inner_node_ptr construct(const key_type* key, const node_ptr* node, const ULL n);
    void construct_hash_node(hash_node_ptr  node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct_dense_node(dense_node_ptr node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct_simple(dense_node_ptr node, const key_type* key, const node_ptr *childs, const ULL n);
    void construct(inner_node_ptr node, const key_type* key, node_ptr *childs, const ULL n);
    void deconstruct(node_ptr node);

    // ========== 1. find ==========

    // below function implemention at 'aex_find.hpp'

    /**
     * @brief 
     * @details keep shared_lock of returned data node;
     */
    inline data_node_ptr find_leaf(const key_type key) const ;
    inline data_node_ptr find_leaf_con(const key_type key, version_type &node_version) ;
    inline data_node_ptr find_leaf_con(const key_type key, version_type &node_version, bool &need_restart) const;
    
    /**
     * @brief 
     * @details keep shared_lock from root to returned data node;
     */
    // inline data_node_ptr find_leaf_with_stack(const key_type key, inner_node_ptr* stack, int &top) const;

    /**
     * @brief find the child node from parent lower bound search with key.
     * @warning these funciton not support concurrency
     */
    node_ptr find(const inner_node_ptr node, const key_type key) const ;
    node_ptr find(const hash_node_ptr  node, const key_type key) const ;
    node_ptr find(const hash_node_ptr  node, const key_type key, bool &need_restart) const;
    node_ptr find(const dense_node_ptr node, const key_type key) const ;
    node_ptr find_con(const dense_node_ptr node, const key_type key) const;

    node_ptr find_insert(const inner_node_ptr node, const key_type key, slot_type &pos) const ;
    node_ptr find_insert(const hash_node_ptr  node, const key_type key, slot_type &pos) const ;
    node_ptr find_insert(const dense_node_ptr node, const key_type key, slot_type &pos) const ;
    node_ptr find_insert(const hash_node_ptr  node, const key_type key, slot_type &pos, bool &need_restart) const ;
    node_ptr find_insert_con(const dense_node_ptr node, const key_type key, slot_type &pos) const ;

    node_ptr find_erase(inner_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const ;
    node_ptr find_erase(hash_node_ptr  node, const key_type key, slot_type &pos, slot_type &next_pos) const ;
    node_ptr find_erase(dense_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const ;

    data_node_ptr find_tail_leaf(node_ptr node) ;
    inline key_type node_zero_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[0]) : (hash_table.find(h_n(node), 0).first);
    }
    inline key_type node_first_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[1]) : (hash_table.find(h_n(node), h_n(node)->next_item(1)).first);
    }
    inline key_type node_last_key(const inner_node_ptr node) const {
        return (node->type == NodeType::DenseNode) ? (d_n(node)->key_ptr[node->size - 1]) : (hash_table.find(h_n(node), h_n(node)->next_item(h_n(node)->slot_size - 1)).first);
    }
    inline node_ptr tail_node(const hash_node_ptr node) {
        key_type last_key;
        node_ptr last_node;
        std::tie(last_key, last_node) = hash_table.find(node, node->prev_item_find(node->slot_size - 1));
        return last_node;
    }

    // ========== 2. insert ==========
    // below function implemention at 'aex_insert.hpp'
    /**
     * @brief insert a child node into parent node.
     */
    void construct_tmp_node(dense_node_ptr node, const key_type old_key, const node_ptr old_node, const key_type new_key, const node_ptr new_node);
    void __construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child);
    void __construct_insert_con(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child);
    node_ptr insert_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child);
    void insert_no_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child);
    void insert(dense_node_ptr node, const key_type key, const node_ptr child);
    void insert_data_node(data_node_ptr node, data_node_ptr new_node, const key_type key, const value_type &value);
    void insert_unlock(inner_node_ptr top_node, inner_node_ptr node) const;
    void split(data_node_ptr old_node, data_node_ptr new_node);
    void split(dense_node_ptr old_node, dense_node_ptr new_node);
    void split_root(dense_node_ptr root); 

    inline data_node_ptr new_and_split(data_node_ptr old_node){
        data_node_ptr new_node = new data_node();
        bool need_restart = false;
        new_node->node_lock.writeLockOrRestart(need_restart);
        AEX_ASSERT(need_restart == false);
        split(old_node, new_node);
        return new_node;
    }

    inline dense_node_ptr new_and_split(dense_node_ptr old_node){
        dense_node_ptr new_node = allocator.allocate_dense_node();
        bool need_restart = false;
        new_node->node_lock.writeLockOrRestart(need_restart);
        AEX_ASSERT(need_restart == false);
        split(old_node, new_node);
        return new_node;
    }

    inline void update_meta(hash_node_ptr node, const node_ptr child, const node_ptr new_tail_node, const size_type add_cnt, const version_type &node_version, bool &need_restart);

    // ========== 3. erase ==========
    // below function implemention at 'aex_erase.hpp'

    bool _erase(const key_type key);
    void erase(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const node_ptr old_node);
    void erase(dense_node_ptr node, const slot_type pos);
    /**
     * @brief check node need do structual modification operation
     * @return true don't restart
     * @return false need restart find child(node)
     */
    bool check_erase_SMO(node_ptr node);
    void merge(data_node_ptr left_node, data_node_ptr right_node);

    // ========== 4. Structure Modify Operation(SMO) ==========
    // below function implemention at 'aex_SMO.hpp'

    // split a ordered key array with data array to node array.
    void split_to_static_data_node(const key_type* const key, const value_type* const data, const ULL n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child);
    void cast_to_hash_node(inner_node_ptr node,  const slot_type slot_size);
    void cast_to_dense_node(inner_node_ptr node);

    void update(hash_node_ptr parent,  const slot_type pos, const slot_type next_pos, const node_ptr old_node, const key_type new_key, const node_ptr new_node);
    /**
     * @brief expand a node slot size
     */
    void expand(hash_node_ptr  node);
    bool expand(dense_node_ptr node);
    /**
     * @brief narrow a node slot size
     */
    //void narrow(inner_node_ptr node);
    void narrow(hash_node_ptr  node);
    //void narrow(dense_node_ptr node);
    /**
     * @brief place split key of node into corresponding slot of hash node.
     */
    //inner_node_ptr __construct_SMO(const key_type* keys, const node_ptr* childs, const ULL n);
    slot_type split(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos);
    void construct_SMO(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n);
    void get_childs(const hash_node_ptr node,  std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const;
    void get_childs(const dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const;
    void get_childs(const inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const;
    void get_childs_recursive(const dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void clear_childs_recursive(dense_node_ptr node);
    void extend_head_nodes(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
    void extend_tail_nodes(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf);
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
        AEX_ASSERT(node->type != NodeType::LeafNode);
        if (node->type == NodeType::HashNode)
            return isfull(h_n(node));
        else
            return isfull(d_n(node));
    }
    
    inline bool isfull(const hash_node_ptr node) const {
        return 1.0 * node->size / node->slot_size >= traits::HASH_NODE_FULL_RATIO && node->slot_size < traits::MAX_HASH_NODE_SLOT_SIZE;
    }

    inline bool isfull(const dense_node_ptr node) const {
        return node->size >= traits::DENSE_NODE_SLOT_SIZE;
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
        if (node->slot_size == traits::DATA_NODE_SLOT_SIZE)
            return false;
        return node->size < node->slot_size * traits::DENSE_NODE_FEW_RATIO;
    }

    // ========== 5. Utility ==========

    inline bool isfull(const data_node_ptr node) const {
        return node->size >= traits::DATA_NODE_SLOT_SIZE;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline void clear_helper(hash_node_ptr node){
        AEX_ASSERT(check_lock(node));
        if constexpr (traits::AllowConcurrency){
            hash_node_ptr node_copy = new hash_node();
            memcpy(node_copy, node, sizeof(hash_node));
            ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashNodeCopy, node_copy));
        }
        else
            clear(node);
    }

    inline void clear(hash_node_ptr node){
        for (slot_type i = node->prev_item_find(node->slot_size - 1); i >= 0; i = node->prev_item_find(i - 1)){
            this->hash_table.erase(node, i);
            if (i == 0)
                break;
        }
        node->clear();
    }

    inline void free_node_helper(node_ptr node){
        AEX_ASSERT(check_lock(node));
        if constexpr (traits::AllowConcurrency)
            ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::NodePtr, node));
        else
            free_node(node);
    }

    inline ID_type get_node_id(){
        if constexpr (traits::AllowConcurrency){
            ID_type expected = this->node_id.load();
            ID_type desired = expected + 1;
            while (!this->node_id.compare_exchange_weak(expected, desired)) {
                expected = this->node_id.load();
                desired   = expected + 1;
            }
            return expected;
        }
        else{
            ID_type expected = this->node_id.load();
            ++this->node_id;
            return expected;
        }
    }

    inline void copy_node(hash_node_ptr node){
        //if constexpr (traits::AllowConcurrency){
        //    hash_node_ptr new_node = new hash_node();
        //    memcpy(new_node, node, sizeof(hash_node));
        //    AEX_ASSERT(check_lock(node));
        //    hash_node_ptr ori_node_copy = node->copy, ori_node_copy_bak = node->copy;
        //    __atomic_store_n(&node->copy, new_node, __ATOMIC_RELEASE);
        //    //while (!node->copy.compare_exchange_weak(ori_node_copy_bak, new_node)) {
        //    //    ori_node_copy_bak = ori_node_copy;
        //    //}
        //    ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashNodeCopy, ori_node_copy));
        //}
        if constexpr (traits::AllowConcurrency){
            hash_node_ptr new_node = new hash_node();
            memcpy(new_node, node, sizeof(hash_node));
            node->copy = new_node;
        }
    }


    // ========== 6. concurrency ==========
    // SL: shared_lock          SU: shared_unlock
    // XL: lock                 XU: unlock
    // UL: upgrade_lock(SL->XL) DL: downgrade_lock(XL->SL)

    inline void XL(node_ptr node);
    inline void TUL(hash_node_ptr node, version_type &node_version, bool &need_restart);
    inline void TUL(node_ptr node, version_type &node_version, bool &need_restart);
    inline void XU(node_ptr node);

    inline bool work_concurrency();

    inline void yield(int count) {
        if (!this->hash_table.work_queue.empty()){
            //while(const_cast<self*>(this)->work_concurrency());
            while(this->hash_table.work_concurrency());
        }
        else if (!this->work_queue.empty()){
            //while(const_cast<self*>(this)->work_concurrency());
            while(this->work_concurrency());
        }
        else
            _yield(count);
    }

    inline void lock_array_unit(LockArrayParams *worker);
    inline void lock_array_con(hash_node_ptr node);
    inline void lock_array(hash_node_ptr node);

    inline void get_childs_unit(GetChildsParams *worker) const;
    inline void get_childs_con(const hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) ;


    slot_type split_con(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos, slot_type &worker_size);
    inline void construct_SMO_unit(ConstructSMOParams *worker);
    inline void construct_SMO_con(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n);


    // ========== 7. test ==========
    bool check_lock(const node_ptr node) const;
    bool check_unlock(const node_ptr node) const;
    bool check_node(node_ptr       node) const ;
    bool check_node(data_node_ptr  node) const ;
    bool check_node(dense_node_ptr node) const ;
    bool check_node(hash_node_ptr  node) const ;
    bool test_lock_array_con(hash_node_ptr node) const ;
    bool test_get_childs_con(hash_node_ptr node) ;
    //bool test_construct_SMO_con(hash_node_ptr node) const ;


};
}

#include "aex_init.hpp"
#include "aex_find.hpp"
#include "aex_insert.hpp"
#include "aex_erase.hpp"
#include "aex_SMO.hpp"
#include "aex_helper.hpp"
#include "aex_test.h"

#include "concurrency/aex_concurrency.hpp"
#include "concurrency/aex_find_con.hpp"
#include "concurrency/aex_insert_con.hpp"
#include "concurrency/aex_erase_con.hpp"


#undef n_n
#undef i_n
#undef h_n
#undef d_n
#undef l_n
#undef ULL
#undef LL
//#undef CACHELINE_SIZE
#undef likely
#undef unlikely

#pragma GCC diagnostic pop
