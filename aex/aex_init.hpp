#pragma once
//#include "aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), work_queue(), allocator(){
    ebr = new EpochBasedMemoryReclamationStrategy(this);
    this->init();
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), work_queue(), allocator(){
//inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), node_id(0), hash_table(traits::MIN_HASH_TABLE_SIZE), allocator(){
    ebr = new EpochBasedMemoryReclamationStrategy(this);
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.emplace_back(*it);
    if (!std::is_sorted(data.begin(), data.end()))
        std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), work_queue(), allocator(){
//inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), node_id(0), hash_table(traits::MIN_HASH_TABLE_SIZE), allocator(){
    ebr = new EpochBasedMemoryReclamationStrategy(this);
    this->init();
    this->root = i_n(this->construct(_index, _index.root));
    this->_size = _index._size;
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(),  work_queue(), allocator(){
//inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), _size(0), opt_stats(), con_stats(), node_id(0), hash_table(traits::MIN_HASH_TABLE_SIZE), allocator(){
    ebr = new EpochBasedMemoryReclamationStrategy(this);
    this->deconstruct(this->root);    
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->_size = _index._size;
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::~aex_tree(){
    AEX_PRINT("root=" << this->root);
    if (root->type == NodeType::HashNode){
        AEX_PRINT("hash_table.slot_size=" << h_n(root)->hash_table.slot_size);
        AEX_DEBUG_BLOCK({h_n(root)->hash_table.print_stats(); });
    }
    this->deconstruct(this->root);
    AEX_PRINT("???");
    delete this->ebr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::_clear(){
    if (this->root != nullptr)
        this->deconstruct(this->root);
    this->_size = 0;
    this->opt_stats = operation_stats();
    this->root = nullptr;
    this->head_leaf = nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init(){
    this->root = allocator.allocate_dense_node(false, 0);
    this->head_leaf = new data_node();
    if constexpr (traits::AllowConcurrency){
        this->head_leaf->next_min_key = std::numeric_limits<key_type>::max();
    }
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
            node->node_lock.writeLock();
            free_node(node);
            break;
        }
        case NodeType::DenseNode:{
            for (slot_type i = 0; i < d_n(node)->size; ++i){
                deconstruct(d_n(node)->child_ptr[i]);
            }
            node->node_lock.writeLock();
            free_node(node);
            break;
        }
        case NodeType::HashNode:{
            XL(h_n(node));
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                std::tie(key, child) = h_n(node)->hash_table.find(i);
                AEX_ASSERT(child != nullptr);
                deconstruct(child);
            }
            h_n(node)->clear();
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
            data_node_ptr new_node = new data_node();
            *l_n(new_node) = *l_n(node);
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
            dense_node_ptr new_node = allocator.allocate_dense_node(d_n(node)->is_train, d_n(node)->level);
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
            hash_node_ptr new_node = allocator.allocate_hash_node(h_n(node)->slot_size);
            new_node->model = h_n(node)->model;
            new_node->node_lock.writeLock();
            key_type key;
            node_ptr child, new_child;
            slot_type pos;
            for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                if (i != 0)
                    __construct_insert(new_node, pos, i, key, new_child);
                std::tie(key, child) = h_n(node)->hash_table.find(i);
                new_child = construct(other, child, tail_leaf);
                pos = i;
            }
            AEX_ASSERT(new_child != nullptr);
            __construct_insert(new_node, pos, h_n(node)->slot_size, key, new_child);
            new_node->tail_node = new_child;
            AEX_MUL_ASSERT(h_n(node)->copy == nullptr);
            copy_node(h_n(new_node));
            new_node->node_lock.writeUnlock();
            AEX_ASSERT(node->size == new_node->size);
            AEX_DEBUG_BLOCK({for (slot_type i = 0; i < h_n(node)->slot_size / 64 + 1; ++i) AEX_ASSERT(new_node->bitmap_ptr[i] == h_n(node)->bitmap_ptr[i]);});
            return new_node;
        }
    }
    return nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const ULL nums){
    AEX_PRINT("[bulk load]");
    EpochGuard guard(this);
    this->_clear();
    if (nums == 0){
        this->init();
        return;
    }
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<data_node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    for (ULL i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
    }
    AEX_DEBUG_BLOCK({if constexpr(!traits::AllowMultiKey) for (ULL i = 0; i < nums - 1; ++i){
            if (key_buf[i] >= key_buf[i + 1]){
                AEX_PRINT("i=" << i << ", " << key_buf[i] << ", " << key_buf[i + 1]);
            }
            AEX_ASSERT(key_buf[i] < key_buf[i + 1]);
        }
    });
    
    split_to_static_data_node(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    ULL m = new_child_buf.size();
    AEX_PRINT("m=" << m);
    new_child_buf[m - 1]->next = nullptr;
    for(ULL i = 0; i < m - 1; ++i)
        new_child_buf[i]->next = new_child_buf[i + 1];
    this->_size = nums;
    this->head_leaf = new_child_buf[0];
    new_key_buf[0] = std::numeric_limits<key_type>::lowest();
    this->root = this->construct(new_key_buf.data(), reinterpret_cast<node_ptr*>(new_child_buf.data()), new_child_buf.size());
    AEX_PRINT("finish...");
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::inner_node_ptr aex_tree<_Key, _Val, traits>::construct(const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(n > 0);
    slot_type slot_size = 0;
    AEX_DEBUG_BLOCK({if (!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    if (n <= traits::DENSE_NODE_SLOT_SIZE){
        dense_node_ptr new_node = allocator.allocate_dense_node(false, 0);
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
    //slot_size = this->train(keys, n, model, 0.5 * traits::HASH_NODE_FULL_RATIO);
    slot_size = this->train(keys, n, model);
    //AEX_PRINT("slot_size=" << slot_size);
    if (slot_size == 0){
        dense_node_ptr new_node = allocator.allocate_dense_node(true, 0);
        new_node->node_lock.writeLock();
        construct_dense_node(new_node, keys, childs, n);
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        AEX_ASSERT(check_node(new_node));
        #endif
        new_node->node_lock.writeUnlock();
        return new_node;
    }
    else{
        hash_node_ptr new_node = allocator.allocate_hash_node(slot_size);
        new_node->node_lock.writeLock();
        AEX_PRINT("hash table slot size=" << h_n(new_node)->hash_table.slot_size << ", node slot size=" << h_n(new_node)->slot_size);
        construct_hash_node(new_node, keys, childs, n);
        #ifdef AEX_DEBUG
        opt_stats.allocate_hash_node_cnt++;
        AEX_ASSERT(check_node(new_node));
        #endif
        AEX_MUL_ASSERT(h_n(new_node)->copy == nullptr);
        copy_node(h_n(new_node));
        new_node->node_lock.writeUnlock();
        return new_node;
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct(inner_node_ptr node, const key_type *keys, node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(n > 0);
    AEX_DEBUG_BLOCK({if (!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    slot_type slot_size = 0;
    Model model;
    if (n < traits::DENSE_NODE_SLOT_SIZE){
        cast_to_dense_node(node);
        construct_simple(d_n(node), keys, childs, n);
        return;
    }
    slot_size = train(keys, n, model);
    if (slot_size == 0){
        cast_to_dense_node(node);
        d_n(node)->is_train = true;
        construct_dense_node(d_n(node), keys, childs, n);
    }
    else{
        cast_to_hash_node(node, slot_size);
        AEX_ASSERT(node->size == 0);
        h_n(node)->model = model;
        construct_hash_node(h_n(node), keys, childs, n);
        AEX_MUL_ASSERT(h_n(node)->copy == nullptr);
        copy_node(h_n(node));
    }
    #ifdef AEX_DEBUG
    AEX_ASSERT(check_node(node));
    #endif
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_simple(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(n <= traits::DENSE_NODE_SLOT_SIZE);
    AEX_ASSERT(n > 0);
    std::copy(keys, keys + n, node->key_ptr);
    std::copy(childs, childs + n, node->child_ptr);
    node->size = n;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_dense_node(dense_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(n + 1 > traits::DENSE_NODE_SLOT_SIZE);
    slot_type child_size = std::max((slot_type)traits::DENSE_NODE_SLOT_SIZE / 2, (slot_type)n / traits::DENSE_NODE_SLOT_SIZE + (n % traits::DENSE_NODE_SLOT_SIZE != 0));
    node->size = n / child_size;
    AEX_ASSERT(node->size <= traits::DENSE_NODE_SLOT_SIZE);
    ULL start = 0;
    for (slot_type i = 0; i < node->size - 1; ++i, start += child_size){
        node->key_ptr[i] = keys[start];
        node->child_ptr[i] = construct(keys + start, childs + start, child_size);
    }
    AEX_ASSERT(n - start > 1);
    node->key_ptr[node->size - 1]   = keys[start];
    node->child_ptr[node->size - 1] = construct(keys + start, childs + start, n - start);
    node->level = d_n(node->child_ptr[0])->level + 1;
    for (slot_type i = 1; i < node->size; ++i)
        node->level = std::min(node->level, (unsigned char)(d_n(node->child_ptr[i])->level + 1));
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_hash_node(hash_node_ptr node, const key_type *keys, const node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    if constexpr(traits::AllowConcurrency)
    AEX_ASSERT(n > 1);
    node->model.train(keys, n, node->slot_size);
    slot_type prev_pos = node->predict(keys[0]), pos = 0;
    AEX_ASSERT(prev_pos == 0);
    ULL start = 0;
    AEX_PRINT("n=" << n);
    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        AEX_ASSERT(pos >= prev_pos);
        if (pos != prev_pos){
            if (i - start > 1)
                __construct_insert(node, prev_pos, pos, keys[start], this->construct(keys + start, childs + start, i - start));
            else
                __construct_insert(node, prev_pos, pos, keys[start], childs[start]);
            start = i;
            prev_pos = pos;
        }
    }
    if (n - start > 1)
        __construct_insert(node, pos, node->slot_size, keys[start], construct(keys + start, childs + start, n - start));
    else
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
    node->tail_node = tail_node(node);
    AEX_ASSERT(node->size >= traits::MIN_HASH_NODE_CNT);
}



}