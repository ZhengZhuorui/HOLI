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
    AEX_ASSERT(check_lock(node));
    #ifdef AEX_DEBUG
    ++opt_stats.cast_to_hash_node_cnt;
    #endif
    clear(node);
    hash_node_ptr cast_node = reinterpret_cast<hash_node_ptr>(node);
    cast_node->type = NodeType::HashNode;
    cast_node->meta_lock.init();
    cast_node->slot_size = slot_size;
    cast_node->bitmap_ptr = nullptr;
    cast_node->init();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::cast_to_dense_node(inner_node_ptr node, const slot_type slot_size){
    AEX_ASSERT(check_lock(node));
    #ifdef AEX_DEBUG
    ++opt_stats.cast_to_dense_node_cnt;
    #endif
    if (node->type == NodeType::DenseNode && node->slot_size == slot_size)
        return;
    clear(node);
    dense_node_ptr cast_node = reinterpret_cast<dense_node_ptr>(node);
    cast_node->type = NodeType::DenseNode;
    cast_node->slot_size = slot_size;
    cast_node->key_ptr = nullptr;
    cast_node->child_ptr = nullptr;
    cast_node->init();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update(hash_node_ptr parent, const slot_type pos, const slot_type next_pos, const key_type new_key, const node_ptr new_node){
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(parent->type == NodeType::HashNode);
    AEX_ASSERT(check_lock_shared(parent));

    hash_table.update(parent, pos, new_key, new_node);
    for (slot_type j = highbit<slot_type, traits::SLOT_PER_SHORT_CUT>(pos + 1); j < next_pos; j += traits::SLOT_PER_SHORT_CUT)
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
    AEX_WARNING("[dense node expand], slot_size=" << node->slot_size); 
    AEX_ASSERT(node->type == NodeType::DenseNode);
    AEX_ASSERT(check_lock(node));
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_expand_cnt;
    opt_stats.dense_node_expand_size += node->size;
    #endif
    
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    if constexpr (traits::AllowExtend)
        extend(node, key_buf, child_buf);
    if (node->size >= traits::MAX_DENSE_NODE_SLOT_SIZE){
        construct(i_n(node), key_buf.data(), child_buf.data(), key_buf.size());
    }
    else{
        node->clear();
        node->slot_size <<= 1;
        node->init();
        std::copy(key_buf.data(),   key_buf.data()   + key_buf.size(), node->key_ptr);
        std::copy(child_buf.data(), child_buf.data() + key_buf.size(), node->child_ptr);
        node->size = key_buf.size();
    }
    for (ULL i = 0; i < key_buf.size(); ++i)
        XU(child_buf[i]);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(hash_node_ptr node){
    AEX_WARNING("[hash node expand], node=" << node << "slot_size=" << node->slot_size); 
    AEX_ASSERT(node->size > 2);
    AEX_ASSERT(check_lock(node));
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    clear(node);
    if constexpr (traits::AllowExtend)
        extend(node, key_buf, child_buf);
    node->slot_size <<= 1;
    const ULL child_size = child_buf.size();
    #ifdef AEX_DEBUG
    ++opt_stats.hash_node_expand_cnt;
    opt_stats.hash_node_expand_size += child_size;
    #endif
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
    AEX_WARNING("[dense node narrow]");
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
    AEX_WARNING("[hash node narrow]");
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
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::split(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos){
    AEX_ASSERT(check_lock(split_node));
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(node->type != NodeType::LeafNode);
    AEX_ASSERT(split_node->type != NodeType::LeafNode);
    key_type key;
    node_ptr child;
    std::vector<key_type> key_buf(0);
    std::vector<node_ptr> child_buf(0);
    slot_type prev_pos, pos, start = 0, ret = end_pos;

    if (split_node->type == NodeType::HashNode){
        std::tie(key, child) = hash_table.find(split_node, h_n(split_node)->prev_item_find(h_n(split_node)->slot_size - 1));
        if (node->predict(key) == start_pos)
            return end_pos;
    }
    else{
        if (node->predict(d_n(split_node)->key_ptr[split_node->size - 1]) == start_pos)
            return end_pos;
    }

    get_childs(i_n(split_node), key_buf, child_buf);
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_cnt;
    opt_stats.inner_node_split_size += key_buf.size();
    #endif
    prev_pos = node->predict(key_buf[0]);
    AEX_ASSERT(prev_pos == start_pos);
    AEX_ASSERT(prev_pos < end_pos);
    const ULL size = key_buf.size();
    for (ULL i = 1; i < size; ++i){
        pos = node->predict(key_buf[i]);
        AEX_ASSERT(prev_pos < end_pos);
        if (pos != prev_pos){
            if (pos >= end_pos)
                break;
            if (start == 0){
                ret = pos;
                AEX_ASSERT(prev_pos == start_pos);
                if (i == 1){
                    free_node(split_node);
                    split_node = child_buf[0];
                }
                else
                    construct(i_n(split_node), key_buf.data(), child_buf.data(), i);
            }
            else{
                if (i - start > 1){
                    inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, i - start);
                    __construct_insert(node, prev_pos, pos, key_buf[start], new_node);
                }
                else
                    __construct_insert(node, prev_pos, pos, key_buf[start], child_buf[start]);
            }
            start = i;
            prev_pos = pos;
        }
    }
    if (start == 0)
        ret = end_pos;
    else{
        AEX_ASSERT(prev_pos < end_pos);
        if (size - start > 1){
            const inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, size - start);
            __construct_insert(node, prev_pos, end_pos, key_buf[start], new_node);
        }
        else
            __construct_insert(node, prev_pos, end_pos, key_buf[start], child_buf[start]);
    }
    if (split_node != child_buf[0])
        XU(child_buf[0]);
    for (ULL i = 1; i < size; ++i)
        XU(child_buf[i]);
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO(hash_node_ptr node, const key_type* const keys, node_ptr* childs, const ULL n){
    AEX_ASSERT(check_lock(node));
    AEX_DEBUG_BLOCK({if constexpr(!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    slot_type pos, prev_pos = node->predict(keys[0]), start = 0, next_pos;
    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (prev_pos != pos){
            next_pos = pos;
            if (pos - prev_pos > 1 && childs[i - 1]->type != NodeType::LeafNode)
                next_pos = split(node, childs[i - 1], prev_pos, pos);            
            AEX_ASSERT(prev_pos < next_pos);
            if (i - start > 1){
                const inner_node_ptr new_node = construct(keys + start, childs + start, i - start);
                __construct_insert(node, prev_pos, next_pos, keys[start], new_node);
            }
            else
                __construct_insert(node, prev_pos, next_pos, keys[start], childs[start]);

            prev_pos = pos;
            start = i;
        }
    }
    AEX_ASSERT(pos < node->slot_size);
    if (n - start > 1){
        const inner_node_ptr new_node = construct(keys + start, childs + start, n - start);
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
        //if (child->size > 0){
            key_buf.emplace_back(key);
            child_buf.emplace_back(child);
        //}
        //else{
        //    AEX_WARNING("!");
        //    free_node(child);
        //}
    }
    AEX_ASSERT(node->size == (long long)key_buf.size() && key_buf.size() == child_buf.size());
    //AEX_ASSERT(key_buf.size() == child_buf.size());
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
        if (now_node != node)
            free_node(now_node);
        for (slot_type i = tmp_key.size() - 1; i >= 0; --i){
            key_buf.emplace_back(tmp_key[i]);
            child_buf.emplace_back(tmp_child[i]);
        }
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
            free_node(now_node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_model(const Model &m, const key_type* const keys, const ULL n, const slot_type slot_size) const {
    if (1.0 * n / slot_size < traits::HASH_NODE_FEW_RATIO)
        return false;
    slot_type cnt = 0, prev_pos = -1, pos;
    [[maybe_unused]]slot_type max_gap = 0;
    for (ULL i = 0; i < n; ++i){
        pos = std::max(0LL, static_cast<slot_type>(std::min(m.predict(keys[i]), (long double)(slot_size - 1))));
        #ifdef AEX_DEBUG
        max_gap = std::max(max_gap, pos - prev_pos);
        #endif
        //if (pos - prev_pos > traits::HASH_NODE_MAX_GAP){
        //    AEX_WARNING("pos=" << pos << ", prev_pos=" << prev_pos);
        //    //return false;
        //}
        if (pos != prev_pos){
            cnt += 1;
            prev_pos = pos;
        }
    }
    AEX_ASSERT(pos < slot_size);
    double ratio = 1.0 * cnt / slot_size;
    AEX_PRINT("ratio=" << ratio << ", cnt=" << cnt << ", slot_size=" << slot_size);
    return ratio >= traits::HASH_NODE_FEW_RATIO;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::train(const key_type* const keys, const ULL n, Model &m) {
    #ifdef AEX_DEBUG
    ++opt_stats.model_train_cnt;
    opt_stats.model_train_size += n;
    #endif
    slot_type slot_size = traits::MIN_HASH_NODE_SLOT_SIZE, ans = 0;
    while (slot_size <= traits::MAX_INNER_NODE_SLOT_SIZE){
        if (!m.train(keys, n, slot_size))
            break;
        if (check_model(m, keys, n, slot_size))
            ans = slot_size;
        else
            break;
        slot_size <<= 1;
    }
    if (ans > 0){
        m.train(keys, n, ans);
        ULL cnt = 0, occupied = 1;
        slot_type prev_pos = 0;
        for (ULL i = 1; i < n; ++i){
            slot_type pos = std::max(0LL, static_cast<slot_type>(std::min(m.predict(keys[i]), (long double)(ans - 1))));
            if (pos == prev_pos) ++cnt;
            else{
                cnt = 1;
                ++occupied;
            }
            if (cnt > n / traits::MIN_DENSE_NODE_SLOT_SIZE){
                ans = 0;
                break;
            }
        }
        if (occupied < traits::MIN_HASH_NODE_CNT)    
            ans = 0;
    }
    AEX_PRINT("n=" << n << ", ans=" << ans);
    return ans;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::rebuild(inner_node_ptr node){
    AEX_WARNING("[rebuild], slot_size=" << node->slot_size);
    AEX_ASSERT(check_lock(node));
    std::vector<key_type> key_buf(0);
    std::vector<node_ptr> child_buf(0);
    key_buf.emplace_back(std::numeric_limits<key_type>::lowest());
    child_buf.emplace_back(node);
    extend_head_nodes(node, key_buf, child_buf);
    extend_tail_nodes(node, key_buf, child_buf);
    AEX_ASSERT(is_sorted(key_buf.data(), key_buf.size()));
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_rebuild_cnt;
    opt_stats.inner_node_rebuild_size += key_buf.size();
    #endif
    construct(node, key_buf.data(), child_buf.data(), key_buf.size());
    for (ULL i = 0; i < key_buf.size(); ++i)
        XU(child_buf[i]);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_extend_head(const key_type first_key, const key_type last_key, const slot_type node_size, const node_ptr child) const {
    AEX_ASSERT(check_lock(child));
    if (child->type == NodeType::LeafNode || child->size <= 1)
        return false;
    const key_type child_first_key = node_first_key(i_n(child));
    const long double slope = 1.0 * (last_key - first_key) / (node_size - 1);
    if (slope > 1.0 * (first_key - child_first_key) / (child->size) && 
        (first_key - child_first_key) > slope)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_extend_tail(const key_type first_key, const key_type last_key, const slot_type node_size, const node_ptr child) const {
    AEX_ASSERT(check_lock(child));
    if (child->type == NodeType::LeafNode || child->size <= 1)
        return false;
    const key_type child_last_key = node_last_key(i_n(child));
    const long double slope = 1.0 * (last_key - first_key) / (node_size - 1);
    if (slope > 1.0 * (child_last_key - last_key) / (child->size) && 
        (child_last_key - last_key) > slope)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend(const inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) {
    AEX_ASSERT(node->size == (slot_type)key_buf.size());
    std::vector<key_type> tmp_key;
    std::vector<node_ptr> tmp_child;
    //if (node->size <= 2) 
    if (node->type != NodeType::HashNode)
        return;
    node_ptr child = child_buf[0];
    key_type first_key = key_buf[1], last_key = key_buf.back();
    bool flag = false;
    while (check_extend_head(first_key, last_key, key_buf.size() - 1, child)){
        AEX_WARNING("extend_head");
        if (!flag){
            std::reverse(key_buf.begin(), key_buf.end());
            std::reverse(child_buf.begin(), child_buf.end());
            flag = true;
        }
        AEX_ASSERT(child == child_buf.back());
        tmp_key.clear();
        tmp_child.clear();
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(i_n(child), tmp_key, tmp_child);
        AEX_PRINT("child=" << child << ", child->type=" << to_string(child->type) << ", child->size=" << child->size);
        free_node(child);
        first_key = tmp_key[1];
        child = tmp_child[0];
        for (slot_type i = tmp_key.size() - 1; i >= 0; --i){
            key_buf.emplace_back(tmp_key[i]);
            child_buf.emplace_back(tmp_child[i]);
        }
    }
    if (flag){
        std::reverse(key_buf.begin(), key_buf.end());
        std::reverse(child_buf.begin(), child_buf.end());
    }
    AEX_PRINT("extend_head end");

    while (check_extend_tail(key_buf[1], key_buf.back(), key_buf.size() - 1, child_buf.back())){
        AEX_WARNING("extend_tail");
        tmp_key.clear();
        tmp_child.clear();
        child = child_buf.back();
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(i_n(child), key_buf, child_buf);
        free_node(child);
    }
    AEX_PRINT("extend_tail end");
    AEX_ASSERT(is_sorted(key_buf.data(), key_buf.size()));
}

}