#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree():version(0), root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), empty_leaf(nullptr), hash_table(traits::MIN_HASH_TABLE_SIZE){
    AEX_HINT("BEGIN");
    this->init();
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): version(0), root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), empty_leaf(nullptr), hash_table(traits::MIN_HASH_TABLE_SIZE){
    this->init();
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.emplace_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):version(0), root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), empty_leaf(nullptr), hash_table(traits::MIN_HASH_TABLE_SIZE){
    _index.XL();
    this->init();
    this->root = this->construct(_index, _index.root);
    this->m_stats = _index.m_stats;
    _index.XU();
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):version(0), root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), empty_leaf(nullptr), hash_table(traits::MIN_HASH_TABLE_SIZE){
    _index.XL();
    this->erase_tree_recursive(this->root);    

    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->tail_leaf = _index.tail_leaf;
    _index.tail_leaf = nullptr;
    this->m_stats = _index.m_stats;
    this->hash_table = std::move(_index.hash_table);
    _index.XU();
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::~aex_tree(){
    XL();
    this->erase_tree_recursive(this->root);
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init(){
    XL();
    if (this->root != nullptr){
        this->erase_tree_recursive(this->root);
        this->hash_table.clear();
    }
    if (empty_leaf == nullptr){
        empty_leaf = new data_node();
        empty_leaf->next = empty_leaf;
        empty_leaf->size = 0;
    }
    this->root = this->head_leaf = this->tail_leaf = nullptr;
    this->m_stats = aex_stats();
    XU();
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::construct(self &other, const node_ptr node){
    //AEX_PRINT("node=" << node << ", IS LEAF?" << IS_LEAF_NODE(node));
    if (node == nullptr)
        return nullptr;
    switch (node->type){
        case NodeType::LeafNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_data_node_cnt++;
            #endif
            data_node_ptr new_node = new data_node();
            *l_n(new_node) = *l_n(node);
            if (head_leaf == nullptr)
                head_leaf = new_node;
            if (tail_leaf == nullptr)
                tail_leaf = new_node;
            else{
                tail_leaf->next = new_node;
                tail_leaf = new_node;
            }
            return new_node;
        }
        case NodeType::DenseNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_dense_node_cnt++;
            #endif
            dense_node_ptr new_node = Allocator::allocate_dense_node(node->slot_size);
            std::copy(node->key_ptr, node->key_ptr + node->size, new_node->key_ptr);
            node->size = new_node->size;
            for (slot_type i = 0; i < node->size; ++i)
                new_node->child_ptr[i] = construct(other, d_n(node)->child_ptr[i]);
            return new_node;
        }
        case NodeType::HashNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_hash_node_cnt++;
            #endif
            hash_node_ptr new_node = Allocator::allocate_hash_node(node->slot_size);
            std::copy(node->bitmap_ptr, node->bitmap_ptr + node->slot_size / traits::SLOT_PER_LOCK + 1, node->bitmap_ptr);
            for (slot_type i = 0; i < node->slot_size;){
                key_type key;
                node_ptr child;
                std::tie(key, child) = other.hash_table.find(node, i);
                node_ptr new_child_node = construct(other, child);
                hash_table.insert(new_node, i, key, new_child_node);
                slot_type j = node->next_item(i + 1);
                for (slot_type k = highbit_64(i + 1); k < j; )
                    hash_table.insert(new_node, k, key, new_child_node);

                i = j;
            }
            new_node->size = node->size;
            return new_node;
        }
    }
    return nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const size_t nums){
    this->deconstruct(this->root);
    if (nums == 0)
        return;
    this->m_stats.height = 1;
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<data_node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    for (size_t i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
    }

    split_to_static_data_node(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    
    size_t m = new_child_buf.size();
    new_child_buf[m - 1]->next = this->empty_leaf;
    for(size_t i = 0; i < m - 1; ++i)
        new_child_buf[i]->next = new_child_buf[i + 1];

    this->m_stats.size = nums;
    this->head_leaf = new_child_buf[0];
    this->tail_leaf = new_child_buf[m - 1];
    this->root = this->construct(new_key_buf.data(), new_child_buf.data(), new_child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::construct(const key_type *keys, const node_ptr* childs, const size_t n){
    slot_type slot_size = 0;
    if (n < traits::MIN_HASH_NODE_SIZE){
        slot_size = min_slot_size(n, traits::MIN_DENSE_NODE_SLOT_SIZE);
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        #endif
        dense_node_ptr new_node = Allocator::allocate_dense_node(slot_size);
        std::copy(keys,   keys + n,   new_node->key_ptr);
        std::copy(childs, childs + n, new_node->child_ptr);
        new_node->size = n;
        return new_node;
    }
    Model model;
    slot_size = train(keys, n, model);
    if (slot_size == 0){
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        #endif
        dense_node_ptr new_node = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
        construct(new_node, keys, childs, n, model);
        return new_node;
    }
    else{
        #ifdef AEX_DEBUG
        opt_stats.allocate_hash_node_cnt++;
        #endif
        hash_node_ptr new_node = Allocator::allocate_hash_node(slot_size);
        construct(new_node, keys, childs, n);
        return new_node;
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(inner_node_ptr node, const key_type *keys, const node_ptr* childs, const size_t n){
    slot_type slot_size = 0;
    Model model;
    if (n < traits::MIN_HASH_NODE_SIZE){
        slot_size = min_slot_size(n, traits::MIN_DENSE_NODE_SLOT_SIZE);
        cast_to_dense_node(node, slot_size);
        std::copy(keys,   keys + n,   d_n(node)->key_ptr);
        std::copy(childs, childs + n, d_n(node)->child_ptr);
        node->size = n;
        return;
    }
    slot_size = train(keys, n, model);
    if (slot_size == 0){
        cast_to_dense_node(node, traits::MIN_DENSE_NODE_SLOT_SIZE);
        construct(d_n(node), keys, childs, n, model);
    }
    else{
        cast_to_hash_node(node, slot_size);
        construct(h_n(node), keys, childs, n);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const size_t n, const Model &m){
    slot_type prev_pos = m.predict(keys[0]), pos;
    size_t start = 0;
    for (size_t i = 1; i < n; ++i){
        pos = m.predict(keys[i]);
        if (pos != prev_pos){
            if (i - start > 1){
                node->key_ptr[node->size] = keys[i - 1];
                node->child_ptr[node->size] = construct(keys + start, keys + i - 1, i - start);
                ++node->size;
            }
            else{
                node->key_ptr[node->size] = keys[i - 1];
                node->child_ptr[node->size] = childs[i - 1];
                ++node->size;
            }
            prev_pos = pos;
        }
    }

    if (n - start > 1){
        node->key_ptr[node->size] = keys[n - 1];
        node->child_ptr[node->size] = construct(keys + start, childs + start, n - start);
        ++node->size;
    }
    else{
        node->key_ptr[node->size] = keys[n - 1];
        node->child_ptr[node->size] = childs[n - 1];
        ++node->size;
    }
    node->try_learn = true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const size_t n){
    std::copy(keys, keys + n, node->key_ptr);
    std::copy(childs, childs + n, node->child_ptr);
    node->size = n;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(hash_node_ptr node, const key_type *keys, const node_ptr* childs, const size_t n){
    node->model.train(keys, n, node->slot_size);
    slot_type prev_pos = node->predict(keys[1]), pos;
    size_t start = 0;
    for (size_t i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (pos != prev_pos){
            if (i - start > traits::MAX_COLLISION){
                inner_node_ptr new_node = this->construct(keys + start, childs + start, i - start);
                __construct_insert(node, prev_pos, pos, keys[start], childs[start]);
            }
            else
                __construct_insert(node, prev_pos, pos, keys[start], childs[start]);
            start = i;
            prev_pos = pos;
        }
    }

    if (n - start > traits::MAX_COLLISION){
        inner_node_ptr new_node = construct(keys + start, childs + start, n - start);
        __construct_insert(node, pos, node->slot_size, keys[start], new_node);
    }
    else
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
}



}