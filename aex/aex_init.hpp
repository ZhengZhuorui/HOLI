#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree():version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE){
    AEX_HINT("BEGIN");
    this->init();
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE){
    this->init();
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.emplace_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE){
    _index.XL();
    this->init();
    this->root = this->construct(_index, _index.root);
    this->m_stats = _index.m_stats;
    _index.XU();
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE){
    _index.XL();
    this->deconstruct(this->root);    
    this->version = _index.version;
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->m_stats = _index.m_stats;
    this->hash_table = std::move(_index.hash_table);
    _index.XU();
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::~aex_tree(){
    XL();
    this->deconstruct(this->root);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init(){
    this->version = 0;
    if (this->root != nullptr){
        this->deconstruct(this->root);
        this->hash_table.clear();
    }
    this->root = this->head_leaf = nullptr;
    this->m_stats.size = 0;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::deconstruct(node_ptr node){
    key_type key;
    node_ptr child;
    switch (node->type){
        case NodeType::LeafNode:{
            free_node(node);
            break;
        }
        case NodeType::DenseNode:{
            for (slot_type i = 0; i < d_n(node)->size; ++i)
                deconstruct(d_n(node)->child_ptr[i]);
            free_node(node);
            break;
        }
        case NodeType::HashNode:{
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                std::tie(key, child) = hash_table.find(h_n(node), i);
                deconstruct(node);
            }
            free_node(node);
            break;
        }
        default:
            break;
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::construct(self &other, const node_ptr node, data_node_ptr &tail_leaf){
    //AEX_PRINT("node=" << node << ", IS LEAF?" << IS_LEAF_NODE(node));
    if (node == nullptr)
        return nullptr;
    switch (node->type){
        case NodeType::LeafNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_data_node_cnt++;
            #endif
            data_node_ptr new_node = new data_node(this->version);
            *l_n(new_node) = *l_n(node);
            new_node->version = this->version;
            if (head_leaf == nullptr)
                head_leaf = new_node;
            else
                tail_leaf->next = new_node;
            tail_leaf = new_node;
            return new_node;
        }
        case NodeType::DenseNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_dense_node_cnt++;
            #endif
            dense_node_ptr new_node = Allocator::allocate_dense_node(d_n(node)->slot_size);
            std::copy(d_n(node)->key_ptr, d_n(node)->key_ptr + d_n(node)->size, new_node->key_ptr);
            d_n(new_node)->size = d_n(node)->size;
            for (slot_type i = 0; i < d_n(node)->size; ++i)
                new_node->child_ptr[i] = construct(other, d_n(node)->child_ptr[i], tail_leaf);
            return new_node;
        }
        case NodeType::HashNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_hash_node_cnt++;
            #endif
            hash_node_ptr new_node = Allocator::allocate_hash_node(h_n(node)->slot_size);
            key_type key;
            node_ptr child;
            slot_type pos;
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                if (i != 0)
                    __construct_insert(new_node, pos, i, key, child);
                std::tie(key, child) = other.hash_table.find(node, i);
                pos = i;
            }
            __construct_insert(new_node, pos, h_n(node)->slot_size, key, child);
            h_n(new_node)->size = h_n(node)->size;
            return new_node;
        }
    }
    return nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const ULL nums){
    this->deconstruct(this->root);
    if (nums == 0)
        return;
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<data_node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    for (ULL i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
    }
    split_to_static_data_node(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    ULL m = new_child_buf.size();
    new_child_buf[m - 1]->next = nullptr;
    for(ULL i = 0; i < m - 1; ++i)
        new_child_buf[i]->next = new_child_buf[i + 1];

    this->m_stats.size = nums;
    this->head_leaf = new_child_buf[0];
    new_key_buf[0] = std::numeric_limits<key_type>::lowest();
    this->root = this->construct(new_key_buf.data(), reinterpret_cast<node_ptr*>(new_child_buf.data()), new_child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::inner_node_ptr aex_tree<_Key, _Val, traits>::construct(const key_type *keys, const node_ptr* childs, const ULL n){
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
inline void aex_tree<_Key, _Val, traits>::construct(inner_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    slot_type slot_size = 0;
    Model model;
    if (n < traits::MIN_HASH_NODE_SIZE){
        slot_size = min_slot_size(n, traits::MIN_DENSE_NODE_SLOT_SIZE);
        cast_to_dense_node(node, slot_size);
        std::copy(keys,   keys + n,   d_n(node)->key_ptr);
        std::copy(childs, childs + n, d_n(node)->child_ptr);
        d_n(node)->size = n;
        return;
    }
    slot_size = train(keys, n, model);
    if (slot_size == 0){
        cast_to_dense_node(node, traits::MIN_DENSE_NODE_SLOT_SIZE);
        construct(d_n(node), keys, childs, n, model);
    }
    else{
        cast_to_hash_node(node, slot_size);
        construct_SMO(h_n(node), keys, childs, n);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n, const Model &m){
    slot_type prev_pos = m.predict(keys[0]), pos;
    ULL start = 0;
    for (ULL i = 1; i < n; ++i){
        pos = m.predict(keys[i]);
        if (pos != prev_pos){
            if (i - start > 1){
                node->key_ptr[node->size] = keys[i - 1];
                node->child_ptr[node->size] = construct(keys + start, childs + start, i - start);
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
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    std::copy(keys, keys + n, node->key_ptr);
    std::copy(childs, childs + n, node->child_ptr);
    node->size = n;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(hash_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    node->model.train(keys, n, node->slot_size);
    slot_type prev_pos = node->predict(keys[1]), pos;
    ULL start = 0;
    node_ptr new_node;
    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (pos != prev_pos){
            if (i - start > 1){
                new_node = this->construct(keys + start, childs + start, i - start);
                __construct_insert(node, prev_pos, pos, keys[start], new_node);
            }
            else
                __construct_insert(node, prev_pos, pos, keys[start], childs[start]);
            start = i;
            prev_pos = pos;
        }
    }

    if (n - start > 1){
        new_node = construct(keys + start, childs + start, n - start);
        __construct_insert(node, pos, node->slot_size, keys[start], new_node);
    }
    else
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
}



}