#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree():version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE), opt_stats(){
    this->init_index();
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE), opt_stats(){
    this->init();
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.emplace_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE), opt_stats(){
    this->init();
    this->root = i_n(this->construct(_index, _index.root));
    this->m_stats = _index.m_stats;
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):version(0), root(nullptr), head_leaf(nullptr), m_stats(), hash_table(traits::MIN_HASH_TABLE_SIZE), opt_stats(){
    this->deconstruct(this->root);    
    this->version = _index.version;
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->m_stats = _index.m_stats;
    this->hash_table = std::move(_index.hash_table);
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::~aex_tree(){
    this->deconstruct(this->root);
    AEX_ASSERT(hash_table.size.load() == 0);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init(){
    this->version = 0;
    if (this->root != nullptr){
        this->deconstruct(this->root);
    }
    this->m_stats.size = 0;
    this->hash_table.clear();
    this->opt_stats = operation_stats();
    this->root = nullptr;
    this->head_leaf = nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init_index(){
    this->init();
    this->root = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
    this->head_leaf = new data_node(this->version);
    d_n(this->root)->key_ptr[0] = std::numeric_limits<key_type>::lowest();
    d_n(this->root)->child_ptr[0] = this->head_leaf;
    this->root->size = 1;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::deconstruct(node_ptr node){
    key_type key;
    node_ptr child;
    if (node == nullptr)
        return;
    switch (node->type){
        case NodeType::LeafNode:{
            XL(node);
            free_node(node);
            break;
        }
        case NodeType::DenseNode:{
            XL(node);
            for (slot_type i = 0; i < d_n(node)->size; ++i)
                deconstruct(d_n(node)->child_ptr[i]);
            free_node(node);
            break;
        }
        case NodeType::HashNode:{
            XL(node);
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                std::tie(key, child) = hash_table.find(h_n(node), i);
                AEX_ASSERT(child != nullptr);
                deconstruct(child);
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
            XL(new_node);
            std::copy(d_n(node)->key_ptr, d_n(node)->key_ptr + d_n(node)->size, new_node->key_ptr);
            d_n(new_node)->size = d_n(node)->size;
            for (slot_type i = 0; i < d_n(node)->size; ++i)
                new_node->child_ptr[i] = construct(other, d_n(node)->child_ptr[i], tail_leaf);
            XU(new_node);
            return new_node;
        }
        case NodeType::HashNode:{
            #ifdef AEX_DEBUG
            opt_stats.allocate_hash_node_cnt++;
            #endif
            hash_node_ptr new_node = Allocator::allocate_hash_node(h_n(node)->slot_size);
            XL(new_node);
            new_node->model = h_n(node)->model;
            key_type key;
            node_ptr child, new_child;
            slot_type pos;
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                if (i != 0)
                    __construct_insert(new_node, pos, i, key, new_child);
                std::tie(key, child) = other.hash_table.find(node, i);
                new_child = construct(other, child, tail_leaf);
                pos = i;
            }
            AEX_ASSERT(new_child != nullptr);
            __construct_insert(new_node, pos, h_n(node)->slot_size, key, new_child);
            AEX_ASSERT(node->size == new_node->size);
            AEX_DEBUG_BLOCK({for (slot_type i = 0; i < h_n(node)->slot_size / 64 + 1; ++i) AEX_ASSERT(new_node->bitmap_ptr[i] == h_n(node)->bitmap_ptr[i]);});
            XU(new_node);
            return new_node;
        }
    }
    return nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const ULL nums){
    AEX_PRINT("[bulk load]");
    this->init();
    if (nums == 0){
        this->init_index();
        return;
    }
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<data_node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    for (ULL i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
    }
    AEX_DEBUG_BLOCK({if constexpr(!traits::AllowMultiKey) for (ULL i = 0; i < nums - 1; ++i) AEX_ASSERT(key_buf[i] < key_buf[i + 1]);});
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
    AEX_DEBUG_BLOCK({if (!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    if (n <= traits::MAX_DENSE_NODE_SLOT_SIZE){
        slot_size = min_slot_size(n, traits::MIN_DENSE_NODE_SLOT_SIZE);
        dense_node_ptr new_node = Allocator::allocate_dense_node(slot_size);
        std::copy(keys,   keys + n,   new_node->key_ptr);
        std::copy(childs, childs + n, new_node->child_ptr);
        new_node->size = n;
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        AEX_ASSERT(check_node(new_node));
        #endif
        return new_node;
    }
        
    Model model;
    slot_size = this->train(keys, n, model);
    if (slot_size == 0){
        slot_size = min_slot_size(n + 1, traits::MIN_DENSE_NODE_SLOT_SIZE);
        dense_node_ptr new_node = Allocator::allocate_dense_node(slot_size);
        XL(new_node);
        construct_dense_node(new_node, keys, childs, n);
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        AEX_ASSERT(check_node(new_node));
        #endif
        XU(new_node);
        return new_node;
    }
    else{
        hash_node_ptr new_node = Allocator::allocate_hash_node(slot_size);
        XL(new_node);
        construct_hash_node(new_node, keys, childs, n);
        #ifdef AEX_DEBUG
        opt_stats.allocate_hash_node_cnt++;
        AEX_ASSERT(check_node(new_node));
        #endif
        XU(new_node);
        return new_node;
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(inner_node_ptr node, const key_type *keys, node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_DEBUG_BLOCK({if (!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    slot_type slot_size = 0;
    Model model;
    if (n < traits::MAX_DENSE_NODE_SLOT_SIZE){
        slot_size = min_slot_size(n, traits::MIN_DENSE_NODE_SLOT_SIZE);
        cast_to_dense_node(node, slot_size);
        construct_simple(d_n(node), keys, childs, n);
        return;
    }
    slot_size = train(keys, n, model);
    if (slot_size == 0){
        cast_to_dense_node(node, traits::MIN_DENSE_NODE_SLOT_SIZE << 1);
        construct_dense_node(d_n(node), keys, childs, n);
    }
    else{
        cast_to_hash_node(node, slot_size);
        AEX_ASSERT(node->size == 0);
        h_n(node)->model = model;
        construct_SMO(h_n(node), keys, childs, n);
    }
    #ifdef AEX_DEBUG
    AEX_ASSERT(check_node(node));
    #endif
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_simple(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(n <= (ULL)node->slot_size);
    std::copy(keys, keys + n, node->key_ptr);
    std::copy(childs, childs + n, node->child_ptr);
    node->size = n;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_dense_node(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(n + 1 > traits::MAX_DENSE_NODE_SLOT_SIZE);
    slot_type start = 0;
    for (ULL i = 0; i < traits::MIN_DENSE_NODE_SLOT_SIZE - 1; ++i){
        node->key_ptr[i] = keys[start];
        node->child_ptr[i] = construct(keys + start, childs + start, n / traits::MIN_DENSE_NODE_SLOT_SIZE);
        start += n / traits::MIN_DENSE_NODE_SLOT_SIZE;
    }
    node->key_ptr[traits::MIN_DENSE_NODE_SLOT_SIZE - 1] = keys[start];
    node->child_ptr[traits::MIN_DENSE_NODE_SLOT_SIZE - 1] = construct(keys + start, childs + start, n - start);
    node->size = traits::MIN_DENSE_NODE_SLOT_SIZE;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_hash_node(hash_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(n > traits::MAX_DENSE_NODE_SLOT_SIZE / 2);
    AEX_ASSERT(n > 1);
    //AEX_HINT("construct hash node...");
    node->model.train(keys, n, node->slot_size);
    slot_type prev_pos = node->predict(keys[0]), pos = 0;
    //AEX_PRINT("n=" << n);
    //AEX_PRINT(keys[0]);
    //AEX_PRINT("prev_pos=" << prev_pos << ", model predict=" << node->model.predict(keys[0]));
    ULL start = 0;
    node_ptr new_node;
    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (pos != prev_pos){
            //AEX_PRINT("i=" << i << ", pos=" << pos << ", prev_pos=" << prev_pos << ", size=" << i - start << ", keys[start]=" << keys[start]);
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
    //AEX_PRINT("pos=" << pos << ", prev_pos=" << prev_pos << ", size=" << n - start << ", keys[start]=" << keys[start]);
    if (n - start > 1){
        new_node = construct(keys + start, childs + start, n - start);
        __construct_insert(node, pos, node->slot_size, keys[start], new_node);
    }
    else
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
    //if constexpr(traits::AllowConcurrency)
    //    AEX_PRINT("node=" << node << "node->lock_array[0]=" << node->lock_array[0].lockCount.load());
}



}