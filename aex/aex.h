
#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cmath>
#include <cstring>
#include <cassert>

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

    #ifdef AEX_DEBUG
    int debug_level; 
    #endif

private:
    node_ptr root;

    data_node_ptr head_leaf;

    data_node_ptr tail_leaf;

    size_type _size, level;

    size_type max_inner_node_slot_size[8];

public:

    aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), _size(0), level(0){
        AEX_PRINT("BEGIN");
        this->init();
        AEX_PRINT("END");
    }

    template<typename _InputIterator>
    aex_tree(_InputIterator __first, _InputIterator __last):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), _size(0), level(0){
        this->init();
        /* TODO: insert data sequencely */
        for (_InputIterator iter = __first; iter != __last; ++iter){
            this->insert(*iter);
        }
    }

    aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), _size(0), level(0){
        this->init();
        this->construct(_index.root, root);
        this->_size = _index.size;
        this->head_leaf = find_head_leaf(root);
        this->tail_leaf = find_tail_leaf(root);
        this->_size = _index._size;
        this->level = _index.level;
    }

    aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), _size(0), level(0){
        this->deconstruct(this->root);
        this->init();
        this->root = _index.root;
        _index.root = nullptr;
        this->head_leaf = _index.head_leaf;
        _index.head_leaf = nullptr;
        this->tail_leaf = _index.tail_leaf;
        _size = _index->_size;
        this->level = _index.level;
    }

    ~aex_tree(){
        AEX_PRINT("BEGIN");
        AEX_PRINT(this->root->size << " " << this->root->prop);
        this->deconstruct(this->root);
        this->root = this->head_leaf = this->tail_leaf = nullptr;
        AEX_PRINT("END");
    }

    std::pair<iterator, bool> insert(const key_type &key, const value_type &value){
        node *_stack[traits::MAX_LEVEL], *now_node;
        // the new child flag is true if  node splited
        bool new_child_flag = false;
        // the update flag is true if and only if only the key is the max key of the tree,
        bool update_max_key_flag = false;
        if (tail_leaf == nullptr)
            update_max_key_flag = true;
        else if (tail_leaf->key[tail_leaf->size - 1] < key)
            update_max_key_flag = true; 
        // if replace a node instead of allocate a new one
        // bool replace_flag = false;

        size_type  num_buf = 0, new_num_buf;
        int _stack_top = 0;
        /* key_buf: key buffer child_buf: child pointer buffer; store when split node*/
        std::vector<key_type> key_buf, new_key_buf;
        std::vector<node_ptr> child_buf, new_child_buf;
        unsigned int dfs_level;
        std::pair<iterator, bool> ret;

        AEX_PRINT("BEGIN");
        if (root == nullptr){
            root = head_leaf = tail_leaf = node_allocator::allocate_data_node();
        }
        AEX_PRINT("level=" << root->level << ", size=" << this->_size << ", root_size=" << root->size);        

        now_node = root;
        //AEX_PRINT(&root << " " << now_data_node);

        AEX_PRINT("FIND PATH");
        /* find the path*/
        while (!(now_node->prop & LEAF)){
            _stack[_stack_top++] = now_node;
            #ifdef AEX_DEBUG
            if (this->debug_level >= 1){
                if (!(now_node->prop & LEAF)){
                    AEX_PRINT("node=" << now_node << " size=" << now_node->size << " level=" << now_node->level << " is ml node?" << ((now_node->prop & ML_NODE) > 0));
                    
                    if (!(now_node->prop & ML_NODE)){
                        for (size_type i = 0; i < static_cast<inner_node_ptr>(now_node)->size; ++i){
                            AEX_PRINT("key=" << static_cast<inner_node_ptr>(now_node)->key_ptr()[i] << " child=" << static_cast<inner_node_ptr>(now_node)->child_ptr()[i]);
                            unsigned long long p = reinterpret_cast<unsigned long long>(static_cast<inner_node_ptr>(now_node)->child_ptr()[i]);
                            AEX_ASSERT(p > 100000);
                        }
                    }
                    else {
                        //AEX_PRINT("slopt=" << static_cast<inner_node_ptr>(now_node)->model.args.slopt << " inter=" << static_cast<inner_node_ptr>(now_node)->model.args.inter);
                        for (size_type i = 0; i < static_cast<inner_node_ptr>(now_node)->slot_size; ++i)
                        if (bitmap_impl::at(static_cast<inner_node_ptr>(now_node)->bitmap_ptr(), i)){
                            AEX_PRINT("pos=" << i << " key=" << static_cast<inner_node_ptr>(now_node)->key_ptr()[i] << " child=" << static_cast<inner_node_ptr>(now_node)->child_ptr()[i]);
                            unsigned long long p = reinterpret_cast<unsigned long long>(static_cast<inner_node_ptr>(now_node)->child_ptr()[i]);
                            AEX_ASSERT(p > 100000);
                        }
                    }
                }
            }
            size_type pos = find_lower_pos(static_cast<inner_node_ptr>(now_node), key);
            key_type last_key_1 = (pos == static_cast<inner_node_ptr>(now_node)->slot_size) ? 0 : static_cast<inner_node_ptr>(now_node)->key_ptr()[pos];
            #endif

            //AEX_PRINT(key << " " << tail_leaf->key[tail_leaf->size - 1]);
            now_node = (update_max_key_flag) ? (static_cast<inner_node_ptr>(now_node)->child_ptr()[static_cast<inner_node_ptr>(now_node)->last()]) : \
                                         (find_lower(static_cast<inner_node_ptr>(now_node), key));

            #ifdef AEX_DEBUG
            key_type last_key_2 = (now_node->prop & LEAF) ? static_cast<data_node_ptr>(now_node)->key[now_node->size - 1]: static_cast<inner_node_ptr>(now_node)->key_ptr()[static_cast<inner_node_ptr>(now_node)->last()];
            if (!update_max_key_flag && last_key_1 != last_key_2){
                AEX_PRINT("last_key_1=" << last_key_1 << " last_key_2=" << last_key_2);
                exit(0);
            }
            #endif
        }

        _stack[_stack_top] = now_node;
        //if (tail_leaf == nullptr || (tail_leaf != nullptr && tail_leaf->key[tail_leaf->size - 1] < key)){
        if (update_max_key_flag){
            for (int i = 0; i < _stack_top; ++i){
                size_type pos = static_cast<inner_node_ptr>(_stack[i])->last();
                update_childnode_key(static_cast<inner_node_ptr>(_stack[i])->child_ptr()[pos], key, static_cast<inner_node_ptr>(_stack[i]));
            }
        }

        AEX_PRINT("FIND PATH END");
        /* insert to data node */
        {
            AEX_PRINT("INSERT DATA NODE");
            data_node_ptr now_data_node = static_cast<data_node_ptr>(now_node);
            data_node_ptr old_data_node;
            data_node_ptr new_data_node;

            /* find the insert position */
            size_type pos = find_lower_pos(now_data_node, key);
            if (!std::is_same<typename traits::AllowMultiKey, std::true_type>::value && pos < traits::DATA_NODE_SLOT_SIZE && now_data_node->key[pos] == key){
                return std::pair<iterator, bool>(end(), false);
            }
            /* if data node is full, split the node */
            if (isfull(now_data_node)){
                AEX_PRINT("INSERT DATA NODE SPLIT");
                /* data_node => [old_data_node, new_data_node] */
                /* parent_node->ptr = [... data_node ...] => [... old_data_node(need insert), new_data_node ... ] */
                old_data_node = now_data_node;
                new_data_node = node_allocator::allocate_data_node();
                //inner_node_ptr parent_node = () ? nullptr : _stack[_stack_top - 1];
                split(old_data_node, new_data_node);
                if (pos < old_data_node->size){
                    pos = _insert(old_data_node, key, value);
                    ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
                }
                else {
                    pos = _insert(new_data_node, key, value);
                    ret = std::pair<iterator, bool>(iterator(new_data_node, pos), true);
                }
                if (_stack_top > 0) 
                    update_childnode_ptr(old_data_node, new_data_node, static_cast<inner_node_ptr>(_stack[_stack_top - 1]));
                if (root == old_data_node)
                    root = new_data_node;
                new_child_flag = true;
                //replace_flag = false;
                key_buf.push_back(old_data_node->key[old_data_node->size - 1]);
                //key_buf.push_back(new_data_node->key[new_data_node->size - 1]);
                child_buf.push_back(old_data_node);
                //child_buf.push_back(new_data_node);
                num_buf = 1;
            }
            /* else insert the position of the data node*/ 
            else{
                size_type pos = _insert(now_data_node, key, value);
                ret = std::pair<iterator, bool>(iterator(now_data_node, pos), true);
            }

        }

        AEX_PRINT("root->prop=" << this->root->prop << " , size=" << this->root->size);
        dfs_level = 1;

        // if a node is update(happend only insert the largest item) or split, recursive insert in inner node
        while (_stack_top > 0 && new_child_flag){
            inner_node_ptr now_inner_node = nullptr, parent_node = nullptr;

            --_stack_top;
            now_inner_node = static_cast<inner_node_ptr>(_stack[_stack_top]);
            parent_node = (_stack_top == 0) ? nullptr : static_cast<inner_node_ptr>(_stack[_stack_top - 1]);
            {
                AEX_PRINT("INSERT INNER NODE");
                /* if only update key() */
                //if (!replace_flag){
                //    update_childnode_key(static_cast<inner_node_ptr>(child_buf[0]), key_buf[0], now_inner_node);
                //}

                if (new_child_flag){
                    new_child_flag = false;
                    // size_type start = (replace_flag) ? 0 : 1;
                    //size_type start = 1 - replace_flag;
                    //size_type start = 0, end = num_buf;

                    for (size_type i = 0; i < num_buf; ++i){
                        /* if current node is full */
                        if (isfull(now_inner_node)) {
                            AEX_PRINT("INNER NODE FULL");
                            if (expand(now_inner_node, parent_node)) {
                                AEX_PRINT("EXPAND SUCCESS");
                            }
                            else{
                                new_child_flag = true;
                                AEX_PRINT("INNER NODE SPLIT");
                                insert_split(now_inner_node, parent_node, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                                new_num_buf = new_key_buf.size();
                                break;
                            }
                        }

                        {
                            AEX_PRINT("CHECK INSERT");
                            /* if can insert, then insert it */
                            if (check_insert(now_inner_node, key_buf[i])){
                                AEX_PRINT("INSERT INNER NODE 2");
                                _insert(now_inner_node, key_buf[i], child_buf[i]);
                            }
                            /* else check if insert it after rewire it 
                               TODO: bulk insert */
                            else if (rewired(now_inner_node) && check_insert(now_inner_node, key_buf[i])){
                                _insert(now_inner_node, key_buf[i], child_buf[i]);
                            }
                            /* else split it */
                            else{
                                new_child_flag = true;
                                insert_split(now_inner_node, parent_node, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                                new_num_buf = new_key_buf.size();
                                break;
                            }
                        }
                    }
                }
            }
            /* swap buffer */
            //std::swap(key_buf, new_key_buf);
            //std::swap(child_buf, new_child_buf);
            //std::swap(num_buf, new_num_buf);
            key_buf = std::move(new_key_buf);
            child_buf = std::move(new_child_buf);
            num_buf = new_num_buf;
            new_num_buf = 0;
            ++dfs_level;
        }

        /* if new child, create a new root */
        if (new_child_flag){
            AEX_PRINT("RESET ROOT");
            inner_node_ptr now_inner_node = nullptr;
            now_inner_node = node_allocator::allocate_inner_node(num_buf, dfs_level);
            key_buf.push_back(((root->prop & LEAF) ? (static_cast<data_node_ptr>(root)->key[root->size - 1]) : 
                                (static_cast<inner_node_ptr>(root)->key_ptr()[static_cast<inner_node_ptr>(root)->last()])));
            child_buf.push_back(root);
            now_inner_node->construct(key_buf.data(), child_buf.data(), num_buf + 1);
            root = now_inner_node;
            AEX_PRINT("root->prop=" << this->root->prop << " , size=" << this->root->size);
            ++this->level;
        }

        ++_size;
        AEX_PRINT("END");
        return ret;
    }

    /*
    void update_max_key(inner_node_ptr node, const key_type max_key){
        size_type last = node->last();
        if (node->prop & ML_NODE){
            size_type prev_pos = node->prev(last);
            prev_pos = (prev_pos == node->slot_size) ? 0 : (prev_pos + 1);
            bitmap_impl::set_zero(node->bitmap_ptr(), last);
            size_type pos = node->predict(max_key);
            bitmap_impl::set_one(node->bitmap_ptr(), pos);
            key_type *key_ptr = node->key_ptr();
            for (size_type i = prev_pos; i <= pos; ++i) key_ptr[i] = max_key;
        }
        else{
            node->key_ptr()[last] = max_key;
        }
    }
    */


    iterator find(const key_type &x) {
        iterator it = find_lower(x);
        if (it.key() != x) 
            return end();
        return it;
    }

    const_iterator find(const key_type &x) const {
        iterator it = find_lower(x);
        if (it.key() != x) 
            return end();
        return it;
    }

    size_type count(const key_type &x) const{
        if (find(x) != end()) return 1;
        return 0;
    }

    bool exists(const key_type &x) const {
        iterator it = find_lower(x);
        if (it.key() != x) return false;
        return true;
    }

    iterator lower_bound(const key_type &x) const{
        return find_lower(x);
    }

    iterator upper_bound(const key_type &x) const{
        return find_upper(x);
    }

    void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
        size_type n = 0;
        /* 
        * TODO: use buffer instead of vector
        */
        std::vector<key_type> key_buf, new_key_buf;
        std::vector<node_ptr> child_buf, new_child_buf;
        data_node_ptr data_node;
        inner_node_ptr inner_node;
        size_type size, slot_size, cnt = 0;
        unsigned int level = 0;
        
        this->deconstruct(this->root);
        
        for (size_type i = 0, cnt = 0; i < nums; i += traits::DATA_NODE_SLOT_SIZE){
            size_type max_j = std::max(i + traits::DATA_NODE_SLOT_SIZE, nums);
            data_node = node_allocator::allocate_data_node();
            for (size_type j = i; j < max_j; ++j){
                data_node->key[j & traits::DATA_NODE_SLOT_SIZE_BIT] = data[j].first;
                data_node->data[j & traits::DATA_NODE_SLOT_SIZE_BIT] = data[j].second;
            }
            data_node->size = max_j - i;
            key_buf.push_back(data_node->key[data_node->size - 1]);
            child_buf.push_back(data_node);
        }
        this->head_leaf = child_buf[0];
        this->tail_leaf = child_buf[child_buf.size() - 1];

        while (key_buf.size() > 1){
            ++level;
            new_key_buf.clear();
            new_child_buf.clear();
            split(key_buf.data(), child_buf.data(), key_buf.size(), level, new_key_buf, new_child_buf);
            key_buf = std::move(new_key_buf);
            child_buf = std::move(new_child_buf);
            new_key_buf.clear();
            new_child_buf.clear();
        }
        this->root = child_buf[0];
    }

    /*
    void bulk_load(const std::pair<key_type, value_type>* data, const size_type n){
        bulk_load(data, data + n);
    }
    */

    /* erase one key*/
    bool erase(const key_type &x){
        node_ptr _stack[traits::MAX_LEVEL], left[traits::MAX_LEVEL], right[traits::MAX_LEVEL];
        unsigned int top;
        if (!_erase_find(x, left, _stack, right, top)) return false;
        _erase(x, left, _stack, right, top);
    }

    bool erase(const iterator iter){
        node_ptr _stack[traits::MAX_LEVEL], left[traits::MAX_LEVEL], right[traits::MAX_LEVEL];
        unsigned int top;
        if (!_erase_find(iter->key(), left, _stack, right, top)) return false;
        _erase(left, _stack, right, top);
    }

    inline iterator begin() {
        return iterator(head_leaf, 0);
    }

    inline const_iterator begin() const {
        return const_iterator(head_leaf, 0);
    }

    inline iterator end(){
        //return iterator(tail_leaf, traits::DATA_NODE_SLOT_SIZE);
        return iterator(tail_leaf, tail_leaf->size);
    }

    inline const_iterator end() const {
        //return iterator(tail_leaf, traits::DATA_NODE_SLOT_SIZE);
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
        return _size;
    }

    inline bool empty() const {
        return _size == 0;
    }

protected:

private:    
    void construct(node_ptr node, node_ptr &new_node){
        if (node->prop&LEAF){
            new_node = node_allocator::allocate_data_node();
            static_cast<data_node>(new_node)->copy(node);
        }
        else{
            new_node = node_allocator::allocate_inner_node(node->slot_size, node->level, ((node->prop & ML_NODE) > 0));
            static_cast<inner_node_ptr>(new_node)->copy(node);
            bitmap bm = static_cast<inner_node_ptr>(new_node)->bitmap_ptr();
            node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr();
            node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr();
            if (node->prop & ML_NODE){
                size_type prev = 0;
                for (size_type i = 0; i < node->slot_size; ++i)
                if (bitmap_impl::at(bm, i)){
                    construct(child[i], new_child[i]);
                    memcpy(new_child + prev, child + prev, (i - prev) * sizeof(node_ptr));
                    prev = i;
                }
                if (prev != node->slot_size - 1)
                    memcpy(new_child + prev, child + prev, (node->slot_size - prev) * sizeof(node_ptr));
            }
            else{
                for (size_type i = 0; i < node->size; ++i){
                    construct(child[i], new_child[i]);
                }
            }
        }
    }

    void deconstruct(node_ptr node){
        if (node == nullptr) return;
        AEX_PRINT("DECONSTRUCT NODE");
        if (node->prop & LEAF){
            node_allocator::free(static_cast<data_node_ptr>(node));
            return;
        }
        else{
            node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr();
            if (node->prop & ML_NODE){
                bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr();
                for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
                if (bitmap_impl::at(bm, i)){
                    this->deconstruct(child[i]);
                }
            }
            else{
                for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                    this->deconstruct(child[i]);
                }
            }
            node_allocator::free(static_cast<inner_node_ptr>(node));
        }
    }

    node_ptr find_head_leaf(node_ptr node) const{
        while (!(node->prop & LEAF))
            node = static_cast<inner_node_ptr>(node)->child_ptr()[static_cast<inner_node_ptr>(node)->first()];
        
        return node;
    }

    node_ptr find_tail_leaf(node_ptr node) const{
        while (!(node->prop & LEAF))
            node = static_cast<inner_node_ptr>(node)->child_ptr()[static_cast<inner_node_ptr>(node)->last()];
        
        return node;
    }

    // if no item greater than or equal x, return node->slot_size (ml node) or node->size(otherwise)
    size_type find_lower_pos(const inner_node_ptr node, const key_type &x) const{
        AEX_PRINT("BEGIN");
        key_type* key = node->key_ptr();
        if (node->prop & ML_NODE){
            size_type pos = node->predict(x), upper_bound = std::min(pos + traits::ERROR_BOUND + 1, node->slot_size);
            for (size_type i = pos; i < upper_bound; ++i)
            if (key[i] >= x){
                return i;
            }
            return node->slot_size;
        }
        else{
            long long L = 0, R = node->size - 1, ret = node->size, mid;
            while (L <= R){
                mid = (L + R) >> 1;
                if (key[mid] >= x) {
                    ret = mid; 
                    R = mid - 1;
                }
                else {L = mid + 1;}
            }
            return ret;
        }
    }

    // if no item greater than or equal x, return node->size
    size_type find_lower_pos(const data_node_ptr node, const key_type &x) const{
        /* TODO: bineary search*/
        AEX_PRINT("BEGIN");
        for (size_type i = 0; i < node->size; ++i)
        if (node->key[i] >= x){
            return i;
        }
        return node->size;
    }

    // if no item greater than x, return node->slot_size (ml node) or node->size(otherwise)
    size_type find_upper_pos(const inner_node_ptr node, const key_type &x) const{
        key_type* key = node->key_ptr();
        if (node->prop & ML_NODE){
            size_type pos = node->predict(x), upper_bound = std::min(pos + traits::ERROR_BOUND + 1, node->slot_size);
            for (size_type i = pos; i < upper_bound; ++i)
            if (key[i] > x)
                return i;
            return node->slot_size;
        }
        else{
            long long L = 0, R = node->size - 1, ret = node->size, mid;
            while (L <= R){
                mid = (L + R) >> 1;
                if (key[mid] > x) {ret = mid; R = mid - 1;}
                else {L = mid + 1;}
            }
            return ret;
        }
    }

    // if no item greater than  x, return node->size
    size_type find_upper_pos(const data_node_ptr node, const key_type &x) const{
        for (size_type i = 0; i < node->size; ++i)
        if (node->key[i] > x)
            return i;
        return node->size;
    }

    // if no item greater than or equal x, return NULL
    inline node_ptr find_lower(const inner_node_ptr node, const key_type &x) const {
        node_ptr* child = node->child_ptr();
        size_type pos = find_lower_pos(node, x);
        AEX_PRINT("pos=" << pos);
        AEX_PRINT("child[pos]=" << child[pos]);
        return (pos == node->slot_size) ? nullptr : child[pos];
    }

    // if no item greater than or equal x, return end()
    iterator find_lower(const data_node_ptr node, const key_type &key) {
        /* TODO:
        size_type L = 0, R = node->size, mid, ret;
        while (L <= R){
            mid = (L + R) >> 1;
            if (node->key[i] >= key) {
                L = mid + 1;
                ret = mid;
            }
            else R = mid - 1;
        }
        */
        for (size_type i = 0; i < node->size; ++i)
        if (node->key[i] >= key)
            return iterator(node, i);
        return end();
    }

    // find the lowest item greater than or equal x, if no, return end()
    iterator find_lower(const key_type &key){
        node_ptr node = root;
        while (!(node->prop & LEAF)){
            node = find_lower(static_cast<inner_node_ptr>(node), key);
        }
        return find_lower(static_cast<data_node_ptr>(node), key);
    }

    const_iterator find_lower(const key_type &key) const {
        node_ptr node = root;
        while (!(node->prop & LEAF)){
            node = find_lower(static_cast<inner_node_ptr>(node), key);
        }
        return find_lower(static_cast<data_node_ptr>(node), key);
    }

    // find the lowest item greater than x, if no, return end()
    node_ptr find_upper(const inner_node_ptr node, const key_type &x) const {
        node_ptr* child = node->child_ptr();
        size_type pos = find_upper_pos(node, x);
        return (pos == node->slot_size) ? nullptr : child[pos];
    }

    iterator find_upper(const data_node_ptr node, const key_type &x) const {
        for (size_type i = 0; i < node->size; ++i)
        if (node->key[i] > x)
            return iterator(node, i);
        return end();
    }

    iterator find_upper(const key_type &key) const {
        node_ptr node = root;
        while (!(node & LEAF)){
            node = find_upper(static_cast<inner_node_ptr>(node), key);
        }
        return find_upper(static_cast<data_node_ptr>(node), key);
    }

    // layout: [a, old_node, b] -> [a, old_node, new_node, b]
    void split(data_node_ptr old_node, data_node_ptr new_node){
        new_node->prev = old_node;
        new_node->next = old_node->next;
        if (old_node->next != nullptr) old_node->next->prev = new_node;
        old_node->next = new_node;
        if (tail_leaf == old_node) tail_leaf = new_node;
        
        size_type mid = traits::DATA_NODE_SLOT_SIZE >> 1;
        memmove(new_node->key, old_node->key + (old_node->size - mid), (old_node->size - mid) * sizeof(key_type));
        data_memmove(new_node->data, old_node->data + (old_node->size - mid), (old_node->size - mid) * sizeof(value_type));

        //memmove(old_node->key, old_node->key + mid, (old_node->size - mid) * sizeof(key_type));
        //data_memmove(old_node->data, old_node->data + mid, (old_node->size - mid) * sizeof(value_type));
        new_node->size = old_node->size - mid;
        old_node->size = mid;
    }

    bool split(const key_type* const key, const node_ptr* const child, const unsigned int n, const unsigned int level, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr node){
        size_type start = 0, end = n, max_slot_size = this->max_inner_slot_size_func(level);
        Model model;
        bool replace_flag = true;
        AEX_PRINT("target 0 " << node->slot_size);
        if (end >= node->real_slot_size() * traits::INNER_NODE_FEW_RATIO){
            size_type size = static_cast<size_type>(node->real_slot_size() * traits::INNER_NODE_FEW_RATIO);
            if (check_rewired(key, size, node->real_slot_size(), model)){
                AEX_PRINT("target 1 size=" << size);
                replace_flag = false;
                if (node->real_slot_size() >= traits::MIN_ML_INNER_NODE_SLOT_SIZE) 
                    node->prop |= ML_NODE;
                node->construct(key, child, size, model);
                new_key.push_back(key[size - 1]);
                new_child.push_back(node);
                start += size;
            }
        }
        /*
        else if (end >= node->real_slot_size() * traits::INNER_NODE_FULL_RATIO){
            AEX_PRINT("target 2");
            size_type size = node->slot_size * traits::INNER_NODE_FULL_RATIO;
            if (end >= size && check_rewired(key, size, node->slot_size, model)){
                replace_flag = false;
                node->construct(key, child, size, model);
                start += size;
                new_key.push_back(key[size - 1]);
                new_child.push_back(node);
            }
        }
        */
        while (start < end){
            max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
            while (max_slot_size < (end - start)) max_slot_size <<= 1;
            max_slot_size = std::min(max_slot_size, this->max_inner_slot_size_func(level));
            for (size_type slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1){
                size_type size = (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE) ? std::min(slot_size, end - start) : std::min((size_type)(slot_size * traits::INNER_NODE_FEW_RATIO), end - start);
                //size_type size = std::min(slot_size, end - start);
                AEX_PRINT("target 3 start=" << start << " end=" << end << " size=" << size << " slot_size=" << slot_size << " key=" << key[start + size - 1]);
                if (check_rewired(key + start, size, slot_size, model)){
                    inner_node_ptr new_node = node_allocator::allocate_inner_node(slot_size, level);
                    new_node->construct(key + start, child + start, size, model);
                    new_key.push_back(key[start + size - 1]);
                    new_child.push_back(new_node);
                    start += size;
                    break;
                }
            }
        }
        return replace_flag;
    }

    // if the node is replaced, the node will free
    bool insert_split(inner_node_ptr node, inner_node_ptr parent, const key_type* const key, const node_ptr* const child, 
               const unsigned int n,
               std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
        key_type* key_buf = allocator::allocate_key_buffer((node->size + n));
        key_type* node_key = node->key_ptr();
        node_ptr* child_buf = allocator::allocate_nodeptr_buffer((node->size + n));
        node_ptr* node_child = node->child_ptr();
        bitmap bm = node->bitmap_ptr();

        size_type j = 0, n_slot = 0;
        AEX_PRINT("BEGIN");
        /* merge key_buffer and node */
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                while (j < n && key[j] < node_key[i]){
                    key_buf[n_slot] = key[j];
                    child_buf[n_slot] = child[j];
                    n_slot++;j++;
                }
                key_buf[n_slot] = node_key[i];
                child_buf[n_slot] = node_child[i];
                n_slot++;
            }
        }
        else{
            for (size_type i = 0; i < node->slot_size; ++i){
                while (j < n && key[j] < node_key[i]){
                    key_buf[n_slot] = key[j];
                    child_buf[n_slot] = child[j];
                    n_slot++;j++;
                }
                key_buf[n_slot] = node_key[i];
                child_buf[n_slot] = node_child[i];
                n_slot++;
            }
        }

        if (j < n){
            memcpy(key_buf + n_slot, key + j, (n - j) * sizeof(key_type));
            memcpy(child_buf + n_slot, child + j, (n - j) * sizeof(node_ptr));
        }
        AEX_PRINT("size=" <<  node->size + n);
        /* split */
        bool replace_flag = split(key_buf, child_buf, node->size + n, node->level, new_key, new_child, node);
        
        if (node != new_child.back()) {
            if (parent != nullptr)
                update_childnode_ptr(node, new_child.back(), parent);
            if (root == node)
                root = new_child.back();
        }
        new_key.pop_back();
        new_child.pop_back();

        if (replace_flag){
            node_allocator::free(node);
        }

        allocator::_free(key_buf);
        allocator::_free(child_buf);
        AEX_PRINT("END");
        return replace_flag;
    }

    bool erase_split(const inner_node_ptr node, std::vector<key_type> &new_key_buf, std::vector<node_ptr> &new_child_buf){
        key_type* key_buf = allocator::allocate_key_buffer(node->size);
        key_type* key = node->key_ptr();
        node_ptr* child_buf = allocator::allocate_nodeptr_buffer(node->size);
        node_ptr* child = node->child_ptr();
        
        copy_to_buffer(node, key_buf, child_buf);
        
        bool flag = split(key_buf, child_buf, node->size, node->level, new_key_buf, new_child_buf, node);

        allocator::_free(key_buf);
        allocator::_free(child_buf);
        return flag;
    }

    // change the parent key  of the child node
    bool update_childnode_key(node_ptr node, const key_type &key, const inner_node_ptr parent){
        AEX_PRINT("BEGIN");
        node_ptr* child = parent->child_ptr();
        key_type* node_key = parent->key_ptr();
        size_type old_pos = parent->at(node), new_pos;
        AEX_PRINT("node=" << node << ", key= "<< key << ", old pos=" << old_pos);
        bitmap bm = parent->bitmap_ptr();
        bool ret = false;
        if (parent->prop & ML_NODE){
            AEX_PRINT("slopt=" << parent->model.args.slopt << " inter=" << parent->model.args.inter);
            new_pos = parent->predict(key);
            size_type upper_bound = std::min(new_pos + traits::ERROR_BOUND, parent->slot_size);
            for (size_type i = new_pos; i < upper_bound; ++i){
                if (!bitmap_impl::at(bm, i) || old_pos == i){
                    new_pos = i;
                    ret = true;
                    break;
                }
            }
            AEX_ASSERT(ret == true);
            #ifdef AEX_DEBUG
            if (this->debug_level >= 1){
                for (size_type i = 0; i < parent->slot_size; ++i)
                    AEX_PRINT("key=" << parent->key_ptr()[i] << "child=" << parent->child_ptr()[i]);
            }
            #endif
            AEX_PRINT("update ml inner node, new_pos=" << new_pos << "ret=" << ret);

            //if (ret == false) return false;

            if (old_pos <= new_pos){
                size_type prev_pos = parent->prev(old_pos);
                prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
                for (size_type i = prev_pos; i <= new_pos; ++i){
                    node_key[i] = key;
                    child[i] = child[old_pos];
                }
                /*
                for (size_type i = old_pos + 1; i <= new_pos; ++i){
                    node_key[i] = key;
                    child[i] = child[old_pos];
                }
                */
            }
            else if (old_pos > new_pos){
                size_type prev_pos = parent->prev(old_pos);
                prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
                for (size_type i = prev_pos; i <= new_pos; ++i){
                    node_key[i] = key;
                    //child[i] = child[old_pos];
                }
                key_type udpate_key = (old_pos == parent->slot_size) ? 0 : node_key[old_pos + 1];
                node_ptr update_node_ptr = (old_pos == parent->slot_size) ? nullptr : child[old_pos + 1];
                for (size_type i = new_pos + 1; i <= old_pos; ++i){
                    node_key[i] = udpate_key;
                    child[i] = update_node_ptr;
                }
            }
            bitmap_impl::set_zero(bm, old_pos);
            bitmap_impl::set_one(bm, new_pos);

            return ret;
            AEX_PRINT("END");
        }
        else{
            parent->key_ptr()[old_pos] = key;
            if (old_pos < parent->size - 1) return true;
            return false;
        }
    }

    void update_childnode_ptr(const node_ptr old_node, const node_ptr new_node, inner_node_ptr parent){
        AEX_PRINT("update childnode pointer old node=" << old_node << " new node=" << new_node << "parent=" << parent);
        size_type pos = parent->at(old_node);
        if (parent->prop & ML_NODE){
            size_type prev_pos = parent->prev(pos);
            node_ptr* node_child = parent->child_ptr();
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= pos; ++i)
                node_child[i] = new_node;
        }
        else{
            parent->child_ptr()[pos] = new_node;
        }
    }

    size_type _insert(data_node_ptr node, const key_type &key, const value_type &data){
        key_type* key_ptr = node->key;
        size_type pos = find_lower_pos(node, key);
        memmove(key_ptr + pos + 1, key_ptr + pos, (node->size - pos) * sizeof(key_type));
        data_memmove(node->data + pos + 1, node->data + pos, (node->size - pos) * sizeof(value_type));
        key_ptr[pos] = key;
        node->data[pos] = data;
        node->size++;
        AEX_PRINT("END");
        return pos;
    }

    size_type _insert(inner_node_ptr node, const key_type &key, const node_ptr child){
        AEX_PRINT("BEGIN. key=" << key);
        size_type pos = find_upper_pos(node, key);
        if (pos == node->slot_size) pos = std::max(node->predict(key), node->last() + 1);
        key_type* node_key = node->key_ptr();
        node_ptr* node_child = node->child_ptr();
        bitmap bm = node->bitmap_ptr();
        AEX_PRINT("pos=" << pos);
        if (node->prop & ML_NODE){
            size_type empty_slot = pos;

            #ifdef AEX_DEBUG
            if (this->debug_level >= 1){
                for (size_type i = 0; i < node->slot_size; ++i)
                if (bitmap_impl::at(bm, i))
                    AEX_PRINT("key=" << node_key[i] << " pos=" << i << " child=" << node_child[i]);
            }
            #endif

            for (size_type i = pos; i < pos + traits::ERROR_BOUND; ++i)
            if (!bitmap_impl::at(bm, i)){
                empty_slot = i;
                break;
            }
            AEX_PRINT("empty_slot=" << empty_slot);
            // shift item to next position
            
            memmove(node_key + pos + 1, node_key + pos, (empty_slot - pos) * sizeof(key_type));
            memmove(node_child + pos + 1, node_child + pos, (empty_slot - pos) * sizeof(node_ptr));            
            
            size_type prev_pos = node->prev(pos);
            AEX_PRINT("prev_pos=" << prev_pos << "pos=" << pos);
            prev_pos = (prev_pos == node->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= pos; ++i){
                node_key[i] = key;
                node_child[i] = child;
            }
            if (bitmap_impl::at(bm, pos)){
                bitmap_impl::set_one(bm, empty_slot);
            }
            else{
                bitmap_impl::set_one(bm, pos);
            }
            node->size++;
            
            
            return pos;
        }
        else{
            memmove(node_key + pos +  1, node_key + pos, (node->size - pos) * sizeof(key_type));
            memmove(node_child + pos + 1, node_child + pos, (node->size - pos) * sizeof(node_ptr));
            node_key[pos] = key;
            node_child[pos] = child;
            node->size++;
            AEX_PRINT("pos=" << pos << "\n END");
            return pos;
        }
    }

    bool check_insert(const inner_node_ptr node, const key_type &key){
        if (!(node->prop & ML_NODE)) return true;
        /* TODO: 
        * optimization: use bit operartion
        */
        
        key_type* node_key = node->key_ptr();
        size_type pos = find_upper_pos(node, key), pred_pos = node->predict(key);
        pos = (pos == node->slot_size) ? std::min(node->slot_size - 1, std::max(node->last() + 1, pred_pos)) : pos;
        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            for (size_type i = 0; i < node->slot_size; ++i){
                AEX_PRINT("pos=" << i << " key=" << node->key_ptr()[i] << " child=" << node->child_ptr()[i]);
            }
            AEX_PRINT("key=" << key << " pos=" << pos << " predict=" << node->predict(key));
        }
        #endif
        // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
        if (pos - pred_pos >= traits::ERROR_BOUND) return false;
        bitmap bm = node->bitmap_ptr();
        size_type max_slot = std::min(node->slot_size, pos + traits::ERROR_BOUND);
        for (size_type i = pos; i < max_slot; ++i)
        if (key < node_key[i]){
            pos = i;
            break;
        }
        // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
        max_slot = std::min(pos + traits::ERROR_BOUND, node->slot_size);
        for (size_type i = pos; i < max_slot; ++i)
        if (bitmap_impl::at(bm, i)){
            size_type shift_pos = node->predict(node_key[i]);
            if (i + 1 - shift_pos >= traits::ERROR_BOUND) return false;
        }
        else{
            return true;
        }

        // if need shift move more than ERROR_BOUND item, return false
        return false;
    }
    
    bool check_rewired(const key_type* const key, const size_type size, const size_type slot_size, Model &m){
        if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
            return true;
        size_type pos;
        m.train(key, size, slot_size);
        AEX_PRINT("BEGIN");
        for (size_type i = 0, start=0; i < size; ++i){            
            pos = std::min(m.predict(key[i]), slot_size - 1);
            start = std::max(start, pos);
            #ifdef AEX_DEBUG
            if (this->debug_level >= 1){
                AEX_PRINT("key=" << key[i] << "pos=" << pos << " start=" << start);
            }
            #endif

            //if (start - pos >= traits::ERROR_BOUND || start >= slot_size + traits::ERROR_BOUND) return false;
            // start >= pos + traits::ERROR_BOUND >= slot_size - 1 + ERROR_BOUND return false
            if (start - pos >= traits::ERROR_BOUND) return false;
            ++start;
        }
        AEX_PRINT("RETURN TRUE");
        return true;
    }

    bool rewired(inner_node_ptr node){
        Model model;
        bool flag = true;
        if (!(node->prop & ML_NODE)) return true;
        key_type* new_key = allocator::allocate_key_buffer(node->size);
        node_ptr* new_child = allocator::allocate_nodeptr_buffer(node->size);

        copy_to_buffer(node, new_key, new_child);

        flag = check_rewired(new_key, node->size, node->real_slot_size(), model);
        if (flag) node->construct(new_key, new_child, node->size, model);
        allocator::_free(new_key);
        allocator::_free(new_child);
        return flag;
    }

    // if node is expanded, the old node will free
    inline bool expand(inner_node_ptr &node, const inner_node_ptr &parent){
        AEX_PRINT("BEGIN");
        AEX_PRINT(node->real_slot_size() << " " << traits::EXPAND_RATIO << " " << node->level << " " << this->max_inner_slot_size_func(node->level));
        if (node->real_slot_size() * traits::EXPAND_RATIO > this->max_inner_slot_size_func(node->level)) 
            return false;
        /* TODO: ML_NODE -> NODE */
        //if (node->prop & ML_NODE)
        {
            size_type new_slot_size = node->real_slot_size() * traits::EXPAND_RATIO;
            key_type* key_buffer = allocator::allocate_key_buffer(node->size);
            copy_to_buffer(node, key_buffer);

            #ifdef AEX_DEBUG
            if (this->debug_level >= 1){
                for (size_type i = 0; i < node->size; ++i)
                    AEX_PRINT("key=" << key_buffer[i]);
            }
            #endif

            Model m;
            bool ml_flag = true;
            if (new_slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE && !check_rewired(key_buffer, node->size, new_slot_size, m)){
                ml_flag = false;
            }
            AEX_ASSERT(ml_flag == true);

            allocator::_free(key_buffer);

            inner_node_ptr new_node = node_allocator::allocate_inner_node(new_slot_size, node->level, ml_flag);

            copy_node(node, new_node);

            if (root == node) 
                root = new_node;
            if (parent != nullptr){
                node_ptr* child = parent->child_ptr();
                for (size_type i = 0; i < parent->slot_size; ++i)
                if (child[i] == node) child[i] = new_node;
            }
            node_allocator::free(node);
            AEX_PRINT("target 4");
            node = new_node;
        }
        //else {
        //}
        AEX_PRINT("END");
        return true;
    }

    // if node is expanded, the old node will free
    inline bool narrow(inner_node_ptr &node){
        size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
        key_type* key_buf = allocator::allocate_key_buffer(node->size);
        node_ptr* child_buf = allocator::allocate_nodeptr_buffer(node->size);
        key_type* node_k = node->key_ptr();
        Model model;
        bool flag;

        copy_to_buffer(node, key_buf, child_buf);
        flag = check_rewired(key_buf, node->size, new_slot_size, model);
        if (flag){
            size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
            inner_node_ptr new_node = node_allocator::allocate_inner_node(new_slot_size, node->level);
            new_node->construct(key_buf, child_buf, node->size);
            if (root == node) root = new_node;
            node_allocator::free(node);
            node = new_node;
        }
        allocator::_free(key_buf);
        allocator::_free(child_buf);
        return true;
    }

    void copy_node(inner_node_ptr node, inner_node_ptr new_node){
        new_node->size = node->size;
        if (!(node->prop & ML_NODE) && !(new_node->prop & ML_NODE)){
            AEX_PRINT("copy node 1" << " " << node << " "<< new_node);
            memcpy(new_node->key_ptr(), node->key_ptr(), node->size * sizeof(key_type));
            memcpy(new_node->child_ptr(), node->child_ptr(), node->size * sizeof(node_ptr));
        }
        else if ((node->prop & ML_NODE) && (new_node->prop & ML_NODE)){
            AEX_PRINT("copy node 2");
            key_type* key_buffer = allocator::allocate_key_buffer(node->size);
            node_ptr* child_buffer = allocator::allocate_nodeptr_buffer(node->size);
            copy_to_buffer(node, key_buffer, child_buffer);
            new_node->construct(key_buffer, child_buffer, node->size);
            allocator::_free(key_buffer);
            allocator::_free(child_buffer);
        }
        else if ((node->prop & ML_NODE) && !(new_node->prop & ML_NODE)){
            AEX_PRINT("copy node 3");
            copy_to_buffer(node, new_node->key_ptr(), new_node->child_ptr());
        }
        else if (!(node->prop & ML_NODE) && (new_node->prop & ML_NODE)){
            AEX_PRINT("copy node 4");
            //for (size_type i = 0 )
            new_node->construct(node->key_ptr(), node->child_ptr(), node->size);
        }
    }

    void merge_leaf(data_node_ptr left_node, data_node_ptr right_node, inner_node_ptr parent){
        memmove(right_node->key + left_node->size, right_node->key, left_node->size * sizeof(key_type));
        data_memmove(right_node->data + left_node->size, right_node->key, right_node->size * sizeof(value_type));
        memmove(right_node->key, left_node->key, left_node->size * sizeof(key_type));
        data_memmove(right_node->data, left_node->data, right_node->size * sizeof(value_type));
        if (parent != nullptr){
            size_type pos = parent->at(left_node);
            size_type prev_pos = parent->prev(pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            node_ptr child = parent->child_ptr();
            for (size_type i = prev_pos; i < pos; ++i)
                child[i] = child[pos];
        }
    }

    void shift_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node){
        left_node->key[left_node->size] = right_node->key[right_node->size - 1];
        left_node->data[left_node->size] = right_node->data[right_node->size - 1];
        ++left_node->size;

        memmove(right_node->key, right_node->key + 1, (right_node->size - 1) * sizeof(key_type));
        data_memmove(right_node->data, right_node->data + 1, (right_node->size - 1) * sizeof(value_type));
        --right_node->size;        
    }

    void shift_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node){
        memmove(right_node->key + 1, right_node->key, (right_node->size) * sizeof(key_type));
        data_memmove(right_node->data + 1, right_node->data, (right_node->size) * sizeof(value_type));
        right_node->key[0] = left_node->key[left_node->size - 1];
        right_node->data[0] = left_node->data[left_node->size - 1];
        ++right_node->size;
        --left_node->size;
    }

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
        return node->size >= traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FULL_RATIO;
    }

    inline bool isfew(const data_node_ptr node) const {
        return node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr node) const {
        return (node->prop & ML_NODE) ? (node->size >= node->slot_size * traits::INNER_NODE_FULL_RATIO) : (node->size >= node->slot_size * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node_ptr node) const {
        return (node->prop & ML_NODE) ? (node->size < node->slot_size * traits::INNER_NODE_FEW_RATIO) : (node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO);
    }

    /*
    inline bool isfull(const inner_node* const node, const size_type bias){
        return (node->prop & ML_NODE) ? (node->size + bias >= node->slot_size * traits::INNER_NODE_FULL_RATIO) : (node->size >= node->slot * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node* const node, const size_type bias){
        return (node->prop & ML_NODE) ? (node->size + bias < node->slot_size * traits::INNER_NODE_FEW_RATIO) : (node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO);
    }
    */

    void copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf){
        key_type* key = node->key_ptr();
        node_ptr* child = node->child_ptr();
        bitmap bm = node->bitmap_ptr();
        size_type n_slot = 0;
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                key_buf[n_slot] = key[i];
                child_buf[n_slot] = child[i];
                n_slot++;
            }
        }
        else{
            memcpy(key_buf, key, node->size * sizeof(key_type));
            memcpy(child_buf, child, node->size * sizeof(node_ptr));
        }
    }

    void copy_to_buffer(const inner_node_ptr node, key_type* const key_buf){
        key_type* key = node->key_ptr();
        bitmap bm = node->bitmap_ptr();
        size_type n_slot = 0;
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                key_buf[n_slot] = key[i];
                n_slot++;
            }
        }
        else{
            memcpy(key_buf, key, node->size * sizeof(key_type));
        }
    }

    bool _erase_find(const key_type* const key, node_ptr* left, node_ptr* _stack, node_ptr* right, unsigned int &top){
        /* recursive find data */
        node_ptr node = root;
        size_type pos;
        top = 0;
        while (!(node->prop & LEAF)){
            _stack[top] = node;
            top++;
            pos = find_lower_pos(static_cast<inner_node_ptr>(node), key);
            size_type prev_pos = static_cast<inner_node_ptr>(node)->prev(pos);
            left[top] = (prev_pos == node->slot_size) ? nullptr : node->child_ptr()[prev_pos];
            if (left[top] == nullptr){
                if (left[top - 1] != nullptr)
                    left[top] = static_cast<inner_node_ptr>(left[top-1])->last();
                else left[top] = nullptr;
            }
            size_type next_pos = static_cast<inner_node_ptr>(node)->next(pos);
            right[top] = (next_pos == node->slot_size) ? nullptr : node->child_ptr()[next_pos];
            if (right[top] == nullptr){
                if (right[top - 1] != nullptr)
                    right[top] = static_cast<inner_node_ptr>(right[top - 1])->first();
                else right[top] = nullptr;
            }
            node = node->child_ptr()[pos];
        }
        if (find_lower(static_cast<data_node_ptr>(node, key)) == end()) 
            return false;
        _stack[top] = node;
        left[top] = node->prev;
        right[top] = node->next;
        top++;
        return true;
    }

    bool _erase(const key_type* const key, const node_ptr _stack, const node_ptr* const left, const node_ptr* const right, const unsigned int stack_depth){
        node *node;
        inner_node_ptr parent;
        inner_node_ptr update_node;
        key_type* key_buf;
        size_type pos;
        node_ptr *child_buf;
        iterator iter;
        key_type last_key;
        int top = stack_depth;
        char update_key_flag = 0, new_update_key_flag = 0;
        bool merge_flag=false;

        if (iter == end()) return false;
        _erase(node, key);

        /* if data node is few, shift the data first, otherwise merge the near leaf */
        if (isfew(static_cast<data_node_ptr>(node))){
            if (node->prev != nullptr && !isfew(static_cast<data_node_ptr>(node->prev))){
                update_key_flag = 1;
                last_key = node->prev->data[node->prev->size - 1];
                shift_left_leaf(node, node->prev, _stack[top-1]);
            }
            else if (node->next != nullptr && !isfew(static_cast<data_node_ptr>(node->next))){
                update_key_flag = 2;
                last_key = node->data[node->size - 1];
                shift_right_leaf(node, node->next, _stack[top-1]);
            }
            else{
                /* merge the left leaf to right leaf */
                merge_flag = true;
                if (node->prev != nullptr){
                    update_key_flag = 1;
                    merge_leaf(node->prev, node, parent);
                    node_allocator::_free(node->prev);
                }
                else if (node->next != nullptr){
                    update_key_flag = 2;
                    merge_leaf(node, node->next, parent);
                    node_allocator::_free(node);
                }
            }
        }

        /* if the key is update */
        while (top > 1 && !update_key_flag){
            --top;
            node = _stack[top];
            parent = _stack[top - 1];

            /* if the left node update */
            if (update_key_flag == 1){
                if (parent->at(left[top]) != -1) {new_update_key_flag = 2; update_node = parent;}
                    else {new_update_key_flag = 1; update_node = left[top - 1];}
            }
            /* if the now node update */
            else if (update_key_flag == 2){
                new_update_key_flag = 2;
                update_node = parent;
            }

            /* if no merge, means the node max key update */
            if (merge_flag == false){
                if (update_key_flag == 1){
                    key_type update_key = update_node->key_ptr()[update_node->last()];
                    new_update_key_flag = update_childnode_key(update_node, update_key, left[top]) ? 0 : update_key_flag;
                }
                else if (update_key_flag == 2){
                    key_type update_key = update_node->key_ptr()[update_node->last()];
                    new_update_key_flag = update_childnode_key(update_node, update_key, _stack[top]) ? 0 : update_key_flag;
                }
            }
            /* otherwise erase the node */
            else{
                merge_flag = false;
                if (update_key_flag == 1){
                    new_update_key_flag = _erase(update_node, left[top]) ? 0 : update_key_flag;
                }
                else {
                    new_update_key_flag = _erase(update_node, parent) ? 0 : update_key_flag;
                }
            }

            /* if the node is too few */
            if (is_few(update_node)){
                if (narrow(update_node)){
                    //update_key_flag = narrow(update_node) ? 0 : update_key_flag;
                }
                else{
                    node_ptr new_node = node_allocator::allocate_inner_node(update_node->size, update_node->level, false);
                    copy_node(update_node, new_node);
                    node_allocator::free(update_node);
                    update_node = new_node;
                }
            }

            update_key_flag = new_update_key_flag;
        }
        if (merge_flag){
            /* if the root has one child, change the root*/
            if (root->size == 1){
                node_ptr tmp = root;
                if (!(root->prop & LEAF)){
                    root = root->child[0];
                }
                else{
                    root = head_leaf = tail_leaf = nullptr;
                }
                allocator::_free(tmp);
            }
        }
        
    }

    // no free the node
    bool _erase(inner_node_ptr parent, inner_node_ptr node){
        size_type pos = parent->at(node);
        if (pos == parent->slot_size) return false;
        key_type* key = parent->key_ptr();
        node_ptr* child = parent->child_ptr();
        node_allocator::free(node);
        --parent->size;
        if (parent->prop & ML_NODE){
            size_type prev_pos = parent->next(pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            bitmap_impl::set_zero(parent->bitmap_ptr(), pos);
            for (size_type i = prev_pos; i <= pos; ++i){
                key[i] = key[pos + 1];
                child[i] = child[pos + 1];
            }
        }
        else{
            memmove(key + pos, key + pos + 1, (parent->size - pos - 1) * sizeof(key_type));
            memmove(child + pos, child + pos + 1, (parent->size - pos - 1) * sizeof(node_ptr));
        }
        return true;
    }

    bool _erase(data_node_ptr node, const key_type& key){
        size_type pos = find_lower_pos(node, key);
        if (pos >= node->size || node->key[pos] != key) return false;
        memmove(node->key + pos, node->key + pos + 1, (node->size - pos - 1) * sizeof(key_type));
        memmove(node->child + pos, node->child + pos + 1, (node->size - pos - 1) * sizeof(value_type));
        --node->size;
    }

    void init(){
        AEX_PRINT("BEGIN");
        this->max_inner_node_slot_size[0] = this->max_inner_node_slot_size[1] = traits::MIN_INNER_NODE_SLOT_SIZE;
        for (size_type i = 2; i < 7; ++i)
        if (this->max_inner_node_slot_size[i - 1] < 0x3ffffff) 
            this->max_inner_node_slot_size[i] = this->max_inner_node_slot_size[i - 1] * this->max_inner_node_slot_size[i - 1];
        else this->max_inner_node_slot_size[i] = this->max_inner_node_slot_size[i - 1];
        for (size_type i = 0 ; i < 7; ++i)
            AEX_PRINT(" " << this->max_inner_node_slot_size[i]);
        AEX_PRINT("END");
    }

// debug
#ifdef AEX_DEBUG_MSG
public:
    std::pair<key_type, bool> _debug(node_ptr node){
        bool flag = true;
        key_type last_key;

        if (node->prop & LEAF){
            data_node_ptr dn = static_cast<data_node_ptr>(node);
            last_key = dn->key[dn->size - 1];
            for (size_type i = 0; i < dn->size; ++i){
                if (i > 0 && dn->key[i] < dn->key[i - 1]){
                    AEX_DEBUG_PRINT("Error! node[" << i-1 << "]=" << dn->key[i - 1] << " node[" << i << "]=" << dn->key[i]);
                    flag = false;
                }
            }
        }
        else{
            inner_node_ptr in = static_cast<inner_node_ptr>(node);
            key_type* node_key = in->key_ptr();
            node_ptr* node_child = in->child_ptr();
            if (node->prop & ML_NODE){
                size_type cnt = 0;
                bitmap bm = in->bitmap_ptr();
                size_type last = in->last();
                last_key = node_key[in->last()];
                for (size_type i = 0; i <= last; ++i){
                    // check if the key is larger than prev position key
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_DEBUG_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    if (bitmap_impl::at(bm, i)){
                        ++cnt;
                        auto res = _debug(node_child[i]);
                        flag &= res.second;
                        // check the child last key is equal to the node key
                        if (node_key[i] != res.first){
                            AEX_DEBUG_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                            flag = false;
                        }
                        // check if the key position is smaller than predict position
                        size_type pos = in->predict(node_key[i]);
                        if (i < pos || i - pos >= traits::ERROR_BOUND){
                            AEX_DEBUG_PRINT("pos=" << i << " predict=" << pos);
                            flag = false;
                        }
                    }
                    
                }
                // check node size is equal to set bits of bitmap 
                if (cnt != node->size){
                    AEX_DEBUG_PRINT("cnt=" << cnt << "size=" << node->size);
                    flag = false;
                }
            }
            else{
                last_key = node_key[node->size - 1];
                for (size_type i = 0; i < node->size; ++i){
                    // check if the key is larger than prev position key 
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_DEBUG_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    
                    auto res = _debug(node_child[i]);
                    if (node_key[i] != res.first){
                        flag = false;
                        AEX_DEBUG_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                    }
                    flag &= res.second;
                    
                    //AEX_ASSERT(i < node->predict(node_key[i]));
                    
                }
            }
        }
        return std::make_pair(last_key, flag);
    }

    bool debug_error(){
        std::pair<key_type, bool> res = (this->root == nullptr)? std::make_pair(0LL, true) : _debug(this->root);
        size_type cnt = 0;
        bool flag = res.second;
        key_type prev_key;
        for (iterator it = begin(); it != end(); ++it){
            if (cnt > 0){
                /* check item is ordered */
                if (it.key() < prev_key){
                    flag = false;
                }
            }
            else prev_key = it.key();
            ++cnt;
        }
        /* check the data item is correct */
        if (cnt != this->_size){
            flag = false;
            AEX_DEBUG_PRINT("Error!");
        }
        return flag;
    }

private:
    //ostream 
#endif


};

};
