#pragma once

namespace aex{

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_to_static_data_node(const key_type* const key, const value_type* const data, const ULL n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    new_key.clear();
    new_child.clear();
    for (ULL i = 0; i < n; i += traits::MIN_DATA_NODE_SLOT_SIZE){
        #ifdef AEX_DEBUG
        opt_stats.allocate_data_node_cnt++;
        #endif
        data_node_ptr new_node = new data_node(this->version);
        ULL size = std::min(static_cast<ULL>(traits::MIN_DATA_NODE_SLOT_SIZE), n - i);
        new_node->construct(key + i, data + i, size);
        new_key.emplace_back(key[i]);
        new_child.emplace_back(new_node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::cast_to_hash_node(inner_node_ptr node, const slot_type slot_size){
    #ifdef AEX_DEBUG
    ++opt_stats.cast_to_hash_node_cnt;
    #endif
    if (node->type == NodeType::HashNode && node->slot_size == slot_size)
        return;
    clear(node);
    hash_node_ptr cast_node = reinterpret_cast<hash_node_ptr>(node);
    cast_node->type = NodeType::HashNode;
    cast_node->slot_size = slot_size;
    cast_node->init();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::cast_to_dense_node(inner_node_ptr node, const slot_type slot_size){
    #ifdef AEX_DEBUG
    ++opt_stats.cast_to_dense_node_cnt;
    #endif
    if (node->type == NodeType::DenseNode && node->slot_size == slot_size)
        return;
    clear(node);
    dense_node_ptr cast_node = reinterpret_cast<dense_node_ptr>(node);
    cast_node->type = NodeType::DenseNode;
    cast_node->slot_size = slot_size;
    cast_node->init();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update(hash_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node){
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(parent->type == NodeType::HashNode);
    AEX_ASSERT(check_lock_shared(parent));

    hash_table.update(parent, pos, new_key, new_node);
    for (slot_type j = highbit_64(pos + 1); j < next_pos; j += traits::SLOT_PER_LOCK)
        hash_table.update(parent, j, new_key, new_node);
    parent->array_downgrade_lock(pos, next_pos);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update(dense_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node){
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(parent->type == NodeType::HashNode);
    AEX_ASSERT(check_lock(parent));

    parent->key_ptr[pos] = new_key;
    parent->child_ptr[pos] = new_node;
    DL(parent);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update(inner_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node){
    switch (parent->type){
        case NodeType::HashNode  : { return update(h_n(parent), pos, next_pos, new_key, new_node); }
        case NodeType::DenseNode : { return update(d_n(parent), pos, next_pos, new_key, new_node); }
        default : { AEX_ASSERT(0 == 1); }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(dense_node_ptr node){
    AEX_ASSERT(node->type == NodeType::DenseNode);
    AEX_ASSERT(check_lock(node));
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_expand_cnt;
    opt_stats.dense_node_expand_size += node->size;
    #endif
    std::vector<key_type> key_buf(node->size);
    std::vector<node_ptr> child_buf(node->size);
    std::copy(node->key_ptr,   node->key_ptr   + node->size, key_buf.data());
    std::copy(node->child_ptr, node->child_ptr + node->size, child_buf.data());
    if (node->size >= traits::MAX_DENSE_NODE_SLOT_SIZE){
        construct(node, key_buf.data(), child_buf.data(), key_buf.size());
    }
    else{
        node->clear();
        node->slot_size <<= 1;
        node->init();
        std::copy(key_buf.data(),   key_buf.data()   + node->size, node->key_ptr);
        std::copy(child_buf.data(), child_buf.data() + node->size, node->child_ptr);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(hash_node_ptr node){
    AEX_ASSERT(node->size > 2);
    AEX_ASSERT(check_lock(node));
    node->slot_size <<= 1;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    const ULL child_size = child_buf.size();
    #ifdef AEX_DEBUG
    ++opt_stats.hash_node_expand_cnt;
    opt_stats.hash_node_expand_size += key_buf.size();
    #endif
    clear(node);
    node->slot_size <<= 1;
    node->init();
    node->model.train(key_buf.data(), child_size, node->slot_size);
    construct_SMO(node, key_buf.data(), child_buf.data(), child_size);
    for (ULL i = 0; i < child_size; ++i)
        XU(child_buf[i]);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(inner_node_ptr node){
    switch (node->type){
        case NodeType::HashNode  : { return expand(h_n(node)); }
        case NodeType::DenseNode : { return expand(d_n(node)); }
        default : { AEX_ASSERT(0 == 1); }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::narrow(dense_node_ptr node){
    AEX_ASSERT(node->size > 1);
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_narrow_cnt;
    opt_stats.dense_node_narrow_size += node->size;
    #endif
    if (node->slot_size == traits::MIN_DENSE_NODE_SLOT_SIZE)
        return;
    std::vector<key_type> key_buf(node->size);
    std::vector<node_ptr> child_buf(node->size);
    std::copy(node->key_ptr,   node->key_ptr   + node->size, key_buf.data());
    std::copy(node->child_ptr, node->child_ptr + node->size, child_buf.data());
    node->clear();
    node->slot_size >>= 1;
    node->init();
    std::copy(key_buf.data(),   key_buf.data()   + node->size, node->key_ptr);
    std::copy(child_buf.data(), child_buf.data() + node->size, node->child_ptr);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::narrow(hash_node_ptr node){
    
    AEX_ASSERT(node->type == NodeType::HashNode);
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    const ULL child_size = child_buf.size();

    #ifdef AEX_DEBUG
    ++opt_stats.hash_node_narrow_cnt;
    opt_stats.hash_node_narrow_size += key_buf.size();
    #endif

    if (key_buf.size() <= traits::MAX_DENSE_NODE_SLOT_SIZE / 2){
        slot_type slot_size = min_slot_size(key_buf.size(), traits::MIN_DENSE_NODE_SLOT_SIZE);
        cast_to_dense_node(i_n(node), slot_size);
        construct_simple(reinterpret_cast<dense_node_ptr>(node), key_buf.data(), child_buf.data(), child_size);
    }
    else{
        clear(node);
        node->slot_size >>= 1;
        node->init();
        node->model.train(key_buf.data(), key_buf.size(), node->slot_size);
        construct_SMO(node, key_buf.data(), child_buf.data(), child_size);
    }
    for (ULL i = 0; i < child_buf.size(); ++i)
        XU(child_buf[i]);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::narrow(inner_node_ptr node){
    switch (node->type){
        case NodeType::HashNode  : { return narrow(h_n(node)); }
        case NodeType::DenseNode : { return narrow(d_n(node)); }
        default : { AEX_ASSERT(0 == 1); }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(hash_node_ptr node, inner_node_ptr split_node, const slot_type start_pos, const slot_type end_pos){
    AEX_ASSERT(check_lock(split_node));
    key_type key;
    node_ptr child;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    ULL size;
    slot_type prev_pos = -1, pos, start;

    if (split_node->type == NodeType::HashNode){
        std::tie(key, child) = hash_table.find(split_node, h_n(split_node)->prev_item_find(split_node->slot_size - 1));
        if (node->predict(key) == start_pos)
            return;
    }
    else{
        if (node->predict(d_n(split_node)->key_ptr[d_n(split_node)->size - 1]) == start_pos)
            return;
    }

    get_childs(split_node, key_buf, child_buf);
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_cnt;
    opt_stats.inner_node_split_size += key_buf.size();
    #endif
    size = key_buf.size();
    for (ULL i = 0; i < size; ++i){
        pos = node->predict(key_buf[i]);
        if (pos != prev_pos){
            if (start == 0){
                AEX_ASSERT(pos < end_pos);
                AEX_ASSERT(prev_pos == start_pos);
                construct(i_n(split_node), key_buf.data(), child_buf.data(), i);
                start = i;
                prev_pos = pos;
            }
            else{
                if (pos == end_pos)
                    break;
                if (i - start > 1){
                    inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, i - start);
                    __insert(node, prev_pos, pos, key_buf[start], new_node);
                }
                else
                    __insert(node, prev_pos, pos, key_buf[start], child_buf[start]);
            }
            start = i;
            prev_pos = pos;
        }
    }
    
    if (size - start > 1){
        inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, size - start);
        __insert(node, prev_pos, end_pos, key_buf[start], new_node);
    }
    else
        __insert(node, prev_pos, end_pos, key_buf[start], child_buf[start]);

    for (ULL i = 0; i < size; ++i)
        XU(child_buf[i]);

    return;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO(hash_node_ptr node, const key_type* const keys, const node_ptr* const childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    inner_node_ptr new_node;
    slot_type pos, prev_pos = node->predict(keys[0]), start = 0;

    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (prev_pos != pos){
            if (i - start > 1){
                new_node = construct(keys + start, childs + start, i - start);
                __construct_insert(node, prev_pos, pos, keys[start], new_node);
            }
            else
                __construct_insert(node, prev_pos, pos, keys[start], childs[start]);
            if (pos - prev_pos > 1 && childs[i - 1]->type != NodeType::LeafNode)
                split(node, i_n(childs[i - 1]), prev_pos, pos);
            prev_pos = pos;
        }
    }
    AEX_ASSERT(pos == node->slot_size);
    if (n - start > 1){
        new_node = construct(keys + start, childs + start, n - start);
        __construct_insert(node, pos, node->slot_size, keys[start], new_node);
    }
    else
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    key_type key;
    node_ptr child;
    AEX_ASSERT(check_lock(node));
    for (slot_type i = 0; i < node->slot_size; i = node->next_item(i + 1)){
        std::tie(key, child) = hash_table.find(node, i);
        XL(child);
        key_buf.emplace_back(key);
        child_buf.emplace_back(child);
    }
    AEX_ASSERT(node->size == (long long)key_buf.size() && key_buf.size() == child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    for (slot_type i = 0; i < node->size; ++i){
        XL(node->child_ptr[i]);
        key_buf.emplace_back(node->key_ptr[i]);
        child_buf.emplace_back(node->child_ptr[i]);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    if (node->type == NodeType::HashNode)
        get_childs(h_n(node), key_buf, child_buf);
    else
        get_childs(d_n(node), key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend_head_nodes(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    AEX_ASSERT(child_buf.back()->type == NodeType::HashNode);
    std::vector<key_type> tmp_key;
    std::vector<node_ptr> tmp_child;
    while(child_buf.back()->type != NodeType::LeafNode){
        tmp_key.clear();
        tmp_child.clear();
        inner_node_ptr now_node = i_n(child_buf.back());
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(now_node, tmp_key, tmp_child);
        for (slot_type i = tmp_key.size() - 1; i >= 0; --i){
            key_buf.emplace_back(tmp_key[i]);
            child_buf.emplace_back(tmp_child[i]);
        }
        if (now_node != node)
            free(now_node);
    }
    std::reverse(key_buf.begin(), key_buf.end());
    std::reverse(child_buf.begin(), child_buf.end());
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend_tail_nodes(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    while(child_buf.back()->type != NodeType::LeafNode){
        inner_node_ptr now_node = i_n(child_buf.back());
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(now_node, key_buf, child_buf);
        if (now_node != node)
            free(now_node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_model(const Model &m, const key_type* const keys, const ULL n, const slot_type slot_size){
    slot_type cnt = 0, prev_pos = -1, pos;
    for (ULL i = 0; i < n; ++i){
        pos = m.predict(keys[i]);
        if (pos - prev_pos > traits::HASH_NODE_MAX_GAP)
            return false;
        if (pos != prev_pos){
            cnt += 1;
            prev_pos = pos;
        }
    }
    AEX_ASSERT(pos < slot_size);
    double ratio = 1.0 * cnt / slot_size;
    AEX_HINT("ratio=" << ratio << ", cnt=" << cnt << ", slot_size=" << slot_size);
    return ratio >= traits::HASH_NODE_FEW_RATIO && cnt >= traits::MIN_HASH_NODE_CNT;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::train(const key_type* const keys, const ULL n, Model &m){
    #ifdef AEX_DEBUG
    ++opt_stats.model_train_cnt;
    opt_stats.model_train_size += n;
    #endif
    slot_type slot_size = traits::MIN_HASH_NODE_SLOT_SIZE, ans = 0;
    while (slot_size <= traits::MAX_INNER_NODE_SLOT_SIZE){
        if (m.train(keys, n, slot_size))
            break;
        if (check_model(m, keys, n, slot_size))
            ans = slot_size;
        else
            break;
        slot_size <<= 1;
    }
    if (ans > 0)
        m.train(keys, n, ans);
    return ans;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::rebuild(inner_node_ptr node){
    AEX_ASSERT(check_lock(node));
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    key_buf.emplace_back(std::numeric_limits<key_type>::lowest());
    child_buf.emplace_back(node);
    extend_head_nodes(node, key_buf, child_buf);
    extend_tail_nodes(node, key_buf, child_buf);
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_rebuild_cnt;
    opt_stats.inner_node_rebuild_size += key_buf.size();
    #endif
    construct(node, key_buf.data(), child_buf.data(), key_buf.size());
    for (ULL i = 0; i < key_buf.size(); ++i)
        XU(child_buf[i]);
}

}