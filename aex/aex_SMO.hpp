#pragma once

namespace aex{

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_to_static_data_node(const key_type* const key, const value_type* const data, const ULL n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    new_key.clear();
    new_child.clear();
    int data_node_size = traits::DATA_NODE_SLOT_SIZE * traits::INIT_DATA_NODE_DENSITY;
    if constexpr (traits::AllowConcurrency){
        data_node_size = traits::DATA_NODE_SLOT_SIZE * traits::INIT_DATA_NODE_DENSITY_CON;
    }
    //for (ULL i = 0; i < n; i += traits::DATA_NODE_SLOT_SIZE){
    for (ULL i = 0; i < n; i += data_node_size){
        #ifdef AEX_DEBUG
        //opt_stats.allocate_data_node_cnt++;
        #endif
        data_node_ptr new_node = new data_node();
        //ULL size = std::min(static_cast<ULL>(traits::DATA_NODE_SLOT_SIZE), n - i);
        ULL size = std::min(static_cast<ULL>(data_node_size), n - i);
        new_node->construct(key + i, data + i, size);
        if constexpr (traits::AllowConcurrency){
            new_node->next_min_key = (i + data_node_size >= n) ? std::numeric_limits<key_type>::max() : key[i + size];
        }
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
    if (node->type == NodeType::HashNode)
        clear_helper(h_n(node));
    hash_node_ptr cast_node = reinterpret_cast<hash_node_ptr>(node);

    cast_node->type = NodeType::HashNode;
    cast_node->slot_size = slot_size;
    cast_node->bitmap_ptr = nullptr;
    cast_node->id = this->get_node_id();
    cast_node->init();
    if constexpr (traits::AllowConcurrency){
        cast_node->copy = nullptr;
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::cast_to_dense_node(inner_node_ptr node){
    AEX_ASSERT(check_lock(node));
    #ifdef AEX_DEBUG
    ++opt_stats.cast_to_dense_node_cnt;
    #endif
    dense_node_ptr cast_node = reinterpret_cast<dense_node_ptr>(node);
    if (node->type == NodeType::DenseNode){
        cast_node->init();
        return;
    }
    AEX_ASSERT(node->type == NodeType::HashNode);
    clear_helper(h_n(node));
    cast_node->type = NodeType::DenseNode;
    cast_node->init();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update(hash_node_ptr parent, const slot_type pos, const slot_type next_pos, const node_ptr old_node, const key_type new_key, const node_ptr new_node){
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(parent->type == NodeType::HashNode);
    hash_table.compare_and_swap(parent, pos, old_node, new_key, new_node);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        if (!hash_table.compare_and_swap(parent, i, old_node, new_key, new_node)){
            break;        
        }
    parent->array_unlock(pos - 1, next_pos);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::expand(dense_node_ptr node){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(node->type == NodeType::DenseNode);
    AEX_ASSERT(node->size == traits::DENSE_NODE_SLOT_SIZE);
    AEX_ASSERT(node->is_parent == true);
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_expand_cnt;
    opt_stats.dense_node_expand_size += node->size;
    #endif
    
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs_recursive(node, key_buf, child_buf);
    Model m;
    slot_type slot_size = train(key_buf.data(), key_buf.size(), m);
    if (slot_size == 0)
        return false;
    else{
        clear_childs_recursive(node);
        cast_to_hash_node(node, slot_size);
        reinterpret_cast<hash_node_ptr>(node)->model = m;
        construct_SMO(reinterpret_cast<hash_node_ptr>(node), key_buf.data(), child_buf.data(), child_buf.size());
        copy_node(reinterpret_cast<hash_node_ptr>(node));
    }
    AEX_ASSERT(check_node(i_n(node)));
    return true;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(hash_node_ptr node){
    //AEX_WARNING("[hash node expand], root=" << root << ", node=" << node << ", slot_size=" << node->slot_size); 
    //AEX_DEBUG_BLOCK({if (node->slot_size > 512) AEX_WARNING("[hash node expand], root=" << root << ", node=" << node << ", slot_size=" << node->slot_size); });
    AEX_ASSERT(node->size > 2);
    AEX_ASSERT(check_lock(node));
    AEX_DEBUG_BLOCK({if (node == this->root) AEX_WARNING("node->slot_size=" << node->slot_size << ", node->size=" << node->size); });
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    AEX_DEBUG_BLOCK({if (node == this->root) AEX_WARNING("real_size=" << child_buf.size()); });
    if constexpr (traits::AllowExtend)
        extend(node, key_buf, child_buf);
    //construct(i_n(node), key_buf.data(), child_buf.data(), key_buf.size());
    clear_helper(node);
    node->id = this->get_node_id();
    if constexpr (traits::AllowConcurrency)
        node->slot_size *= 8;
    else
        node->slot_size *= 4;
    const ULL child_size = child_buf.size();
    node->init();
    node->model.train(key_buf.data(), child_size, node->slot_size);
    construct_SMO(node, key_buf.data(), child_buf.data(), child_size);
    #ifdef AEX_DEBUG
    ++opt_stats.hash_node_expand_cnt;
    opt_stats.hash_node_expand_size += child_buf.size();
    #endif
    AEX_MUL_ASSERT(node->copy == nullptr);
    copy_node(node);
    AEX_ASSERT(check_node(i_n(node)));
}

/*
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::expand(inner_node_ptr node){
    switch (node->type){
        case NodeType::HashNode  : { return expand(h_n(node)); }
        case NodeType::DenseNode : { return expand(d_n(node)); }
        default : { AEX_ASSERT(0 == 1); }
    }
}
*/

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::narrow(hash_node_ptr node){
    AEX_WARNING("[hash node narrow]");
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(check_lock(node));
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    get_childs(node, key_buf, child_buf);
    const ULL child_size = child_buf.size();

    #ifdef AEX_DEBUG
    ++opt_stats.hash_node_narrow_cnt;
    opt_stats.hash_node_narrow_size += key_buf.size();
    #endif

    if (key_buf.size() <= traits::DENSE_NODE_SLOT_SIZE){
        cast_to_dense_node(i_n(node));
        construct_simple(reinterpret_cast<dense_node_ptr>(node), key_buf.data(), child_buf.data(), child_size);
    }
    else{
        clear_helper(node);
        node->slot_size >>= 1;
        node->id = this->get_node_id();
        node->init();
        node->model.train(key_buf.data(), key_buf.size(), node->slot_size);
        construct_SMO(node, key_buf.data(), child_buf.data(), child_size);
        AEX_MUL_ASSERT(node->copy == nullptr);
        copy_node(node);
    }
    AEX_ASSERT(check_node(i_n(node)));
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::split(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(node->type != NodeType::LeafNode);
    AEX_ASSERT(split_node->type != NodeType::LeafNode);
    AEX_ASSERT(check_lock(node));
    key_type key;
    node_ptr child;
    bool is_deleted = false;
    std::vector<key_type> key_buf(0);
    std::vector<node_ptr> child_buf(0);
    slot_type prev_pos, pos, start = 0, ret = end_pos;

    if (split_node->size == 1){
        return end_pos;
    }
    XL(split_node);
    if constexpr (traits::AllowConcurrency){
        if (split_node->type == NodeType::HashNode)
            for (slot_type i = 0; i < h_n(split_node)->slot_size / traits::SLOT_PER_LOCK; ++i)
                h_n(split_node)->lock_array[i].unlock();
    }
    if (split_node->type == NodeType::HashNode){
        std::tie(key, child) = this->hash_table.find(h_n(split_node), h_n(split_node)->prev_item_find(h_n(split_node)->slot_size - 1));
        if (node->predict(key) == start_pos){
            XU(split_node);return end_pos;
        }
    }
    else{
        if (node->predict(d_n(split_node)->key_ptr[split_node->size - 1]) == start_pos){
            XU(split_node); return end_pos;
        }
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
                    // means all childs insert to the node dependly without the split_node, delete it.
                    is_deleted = true;
                    free_node_helper(split_node);
                    split_node = child_buf[0];
                }
                else{
                    construct(i_n(split_node), key_buf.data(), child_buf.data(), i);
                }
            }
            else{
                AEX_ASSERT(node->is_occupied(prev_pos) == false);
                if (i - start > 1){
                    const inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, i - start);
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
        AEX_ASSERT(node->is_occupied(prev_pos) == false);
        if (size - start > 1){
            const inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, size - start);
            __construct_insert(node, prev_pos, end_pos, key_buf[start], new_node);
        }
        else
            __construct_insert(node, prev_pos, end_pos, key_buf[start], child_buf[start]);
    }
    if (!is_deleted){
        XU(split_node);
    }
    AEX_ASSERT(check_unlock(split_node));
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n){
    if constexpr (traits::AllowConcurrency){
        if (n > traits::THREAD_UNIT_SIZE * 2){
            #ifdef AEX_DEBUG
            ++const_cast<self*>(this)->opt_stats.construct_SMO_con_cnt;
            #endif
            construct_SMO_con(node, keys, childs, n);
            check_node(node);
            return;
        }
    }

    AEX_ASSERT(check_lock(node));
    AEX_DEBUG_BLOCK({if constexpr(!traits::AllowMultiKey) for (ULL i = 0; i < n - 1; ++i) AEX_ASSERT(keys[i] < keys[i + 1]);});
    slot_type pos, prev_pos = node->predict(keys[0]), start = 0, next_pos;
    for (ULL i = 1; i < n; ++i){
        pos = node->predict(keys[i]);
        if (prev_pos != pos){
            next_pos = pos;
            if (pos - prev_pos > 1 && childs[i - 1]->type != NodeType::LeafNode){
                if (childs[i - 1]->size > 1)
                    next_pos = split(node, childs[i - 1], prev_pos, pos);
            }
            AEX_ASSERT(prev_pos < next_pos);
            AEX_ASSERT(node->is_occupied(prev_pos) == false);
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
    AEX_ASSERT(node->is_occupied(pos) == false);
    if (n - start > 1){
        const inner_node_ptr new_node = construct(keys + start, childs + start, n - start);
        __construct_insert(node, pos, node->slot_size, keys[start], new_node);
        node->tail_node = new_node;
    }
    else{
        __construct_insert(node, pos, node->slot_size, keys[start], childs[start]);
        node->tail_node = childs[start];
    }
    AEX_ASSERT(node->tail_node == tail_node(node));
    /*AEX_DEBUG_BLOCK({
        bool flag = false;
        for (int i = 0; i < node->slot_size; i = node->next_item(i + 1)){
            if (!check_unlock(hash_table.find(node, i).second)){
                AEX_ERROR("i=" << i);
                flag = true;
            }
            AEX_SGL_ASSERT(!flag);
            //AEX_SGL_ASSERT(check_unlock(hash_table.find(node, i).second));
        }
    });*/
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs_recursive(const dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    AEX_ASSERT(check_lock(node));
    for (slot_type i = 0; i < node->size; ++i){
        if (node->child_ptr[i]->type != NodeType::DenseNode){
            key_buf.emplace_back(node->key_ptr[i]);
            child_buf.emplace_back(node->child_ptr[i]);
        }
        else{
            XL(d_n(node->child_ptr[i]));
            get_childs_recursive(d_n(node->child_ptr[i]), key_buf, child_buf);
            XU(d_n(node->child_ptr[i]));
        }
    }
    AEX_ASSERT(key_buf.size() == child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::clear_childs_recursive(dense_node_ptr node){
    for (slot_type i = 0; i < node->size; ++i){
        if (node->child_ptr[i]->type == NodeType::DenseNode){
            dense_node_ptr child = d_n(node->child_ptr[i]);
            clear_childs_recursive(child);
            XL(child);
            free_node_helper(child);
        }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(const hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const {
    if constexpr (traits::AllowConcurrency){
        if (node->slot_size > traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK * 2){
            #ifdef AEX_DEBUG
            ++const_cast<self*>(this)->opt_stats.get_childs_con_cnt;
            #endif
            const_cast<self*>(this)->get_childs_con(node, key_buf, child_buf);
            //AEX_ASSERT(const_cast<self*>(this)->test_get_childs_con(node) == true);
            return;
        }
    }
    key_type key;
    node_ptr child;
    AEX_ASSERT(check_lock(node));
    for (slot_type i = 0; i < node->slot_size; i = node->next_item(i + 1)){
        std::tie(key, child) = this->hash_table.find(node, i);
        key_buf.emplace_back(key);
        child_buf.emplace_back(child);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(const dense_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const {
    for (slot_type i = 0; i < node->size; ++i){
        key_buf.emplace_back(node->key_ptr[i]);
        child_buf.emplace_back(node->child_ptr[i]);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs(const inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) const {
    if (node->type == NodeType::HashNode)
        get_childs(h_n(node), key_buf, child_buf);
    else 
        get_childs(d_n(node), key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend_head_nodes(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    if (child_buf[0]->type == NodeType::LeafNode)
        return;
    std::vector<key_type> tmp_key;
    std::vector<node_ptr> tmp_child;
    std::reverse(key_buf.begin(), key_buf.end());
    std::reverse(child_buf.begin(), child_buf.end());
    XL(child_buf.back());
    while(child_buf.back()->type != NodeType::LeafNode){
        tmp_key.clear();
        tmp_child.clear();
        inner_node_ptr now_node = i_n(child_buf.back());
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(now_node, tmp_key, tmp_child);
        free_node_helper(now_node);
        for (slot_type i = tmp_key.size() - 1; i >= 0; --i){
            key_buf.emplace_back(tmp_key[i]);
            child_buf.emplace_back(tmp_child[i]);
        }
        XL(child_buf.back());
    }
    XU(child_buf.back());
    std::reverse(key_buf.begin(), key_buf.end());
    std::reverse(child_buf.begin(), child_buf.end());
    AEX_ASSERT(key_buf.size() == child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend_tail_nodes(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    XL(child_buf.back());
    while(child_buf.back()->type != NodeType::LeafNode){
        inner_node_ptr now_node = i_n(child_buf.back());
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(now_node, key_buf, child_buf);
        free_node_helper(now_node);
        XL(child_buf.back());
    }
    XU(child_buf.back());
    AEX_ASSERT(key_buf.size() == child_buf.size());
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_model(const Model &m, const key_type* const keys, const ULL n, const slot_type slot_size) const {
    if (1.0 * n / slot_size < traits::HASH_NODE_FEW_RATIO)
        return false;
    slot_type occupied = 0, prev_pos = -1, pos;
    [[maybe_unused]]slot_type max_gap = 0;
    for (ULL i = 0; i < n; ++i){
        pos = std::max(0LL, static_cast<slot_type>(std::min(m.predict(keys[i]), (long double)(slot_size - 1))));
        #ifdef AEX_DEBUG
        max_gap = std::max(max_gap, pos - prev_pos);
        #endif
        if (pos != prev_pos){
            occupied += 1;
            prev_pos = pos;
        }
    }
    AEX_ASSERT(pos < slot_size);
    double ratio = 1.0 * occupied / slot_size;
    //AEX_PRINT("ratio=" << ratio << ", n=" << n << ", cnt=" << occupied << ", slot_size=" << slot_size);
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
        double entropy = log(n), train_entropy = 0;
        for (ULL i = 1; i < n; ++i){
            slot_type pos = std::max(0LL, static_cast<slot_type>(std::min(m.predict(keys[i]), (long double)(ans - 1))));
            if (pos == prev_pos) ++cnt;
            else{
                train_entropy += 1.0 * cnt / n * log(cnt);
                cnt = 1;
                ++occupied;
                prev_pos = pos;
            }
        }
        train_entropy += 1.0 * cnt / n * log(cnt);
        //if (occupied < traits::MIN_HASH_NODE_CNT || max_cnt > occupied * 0.5)
        if (entropy - train_entropy < log(traits::MIN_HASH_NODE_CNT) - 0.1){
            AEX_PRINT("entrogy=" << entropy << ", train_entropy=" << train_entropy);
            ans = 0;
        }
        if (occupied < traits::MIN_HASH_NODE_CNT){
            //AEX_PRINT("occupied=" << occupied << ", ans=" << ans);
            ans = 0;
        }
        //else
        //    AEX_PRINT("occupied=" << occupied << ", max_cnt=" << max_cnt);
    }
    //AEX_PRINT("n=" << n << ", ans=" << ans);
    return ans;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::rebuild(inner_node_ptr node){
    AEX_WARNING("[rebuild], slot_size=" << node->slot_size << ", size=" << node->size);
    AEX_ASSERT(check_lock(node));
    std::vector<key_type> key_buf, tmp_key_buf;
    std::vector<node_ptr> child_buf, tmp_child_buf;
    get_childs(node, tmp_key_buf, tmp_child_buf);
    //AEX_ASSERT((slot_type)tmp_key_buf.size() == node->size);
    for (slot_type i = 0; i < tmp_key_buf.size(); ++i){
        XL(tmp_child_buf[i]);
        if (tmp_child_buf[i]->type != NodeType::LeafNode){
            get_childs(i_n(tmp_child_buf[i]), key_buf, child_buf);
            free_node_helper(tmp_child_buf[i]);
        }
        else{
            key_buf.push_back(tmp_key_buf[i]);
            child_buf.push_back(tmp_child_buf[i]);
            XU(tmp_child_buf[i]);
        }
    }
    extend_head_nodes(key_buf, child_buf);
    extend_tail_nodes(key_buf, child_buf);
    AEX_ASSERT(is_sorted(key_buf.data(), key_buf.size()));
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_rebuild_cnt;
    opt_stats.inner_node_rebuild_size += key_buf.size();
    #endif
    construct(node, key_buf.data(), child_buf.data(), key_buf.size());
    AEX_PRINT("child_buf.size=" << child_buf.size() << ", node->slot_size=" << node->slot_size << ", node->type=" << to_string(node->type));
    AEX_PRINT("[rebuild] end.");
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_extend_head(const key_type first_key, const key_type last_key, const slot_type node_size, const node_ptr child) const {
    AEX_ASSERT(check_lock(child));
    if (child->type == NodeType::LeafNode || child->size <= 1)
        return false;
    const key_type child_first_key = node_first_key(i_n(child));
    const long double slope_ni = 1.0 * (last_key - first_key) / (node_size - 1);
    if (slope_ni > 1.0 * (first_key - child_first_key) / (child->size) && 
        (first_key - child_first_key) > slope_ni)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_extend_tail(const key_type first_key, const key_type last_key, const slot_type node_size, const node_ptr child) const {
    AEX_ASSERT(check_lock(child));
    if (child->type == NodeType::LeafNode || child->size <= 1)
        return false;
    const key_type child_last_key = node_last_key(i_n(child));
    const long double slope_ni = 1.0 * (last_key - first_key) / (node_size - 1);
    if (slope_ni > 1.0 * (child_last_key - last_key) / (child->size) && 
        (child_last_key - last_key) > slope_ni)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::extend(const inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) {
    //AEX_ASSERT(node->size == (slot_type)key_buf.size());
    AEX_DEBUG_BLOCK({if constexpr (!traits::AllowConcurrency) AEX_ASSERT(node->size == (slot_type)key_buf.size());});
    std::vector<key_type> tmp_key;
    std::vector<node_ptr> tmp_child;
    //if (node->size <= 2) 
    if (node->type != NodeType::HashNode)
        return;
    node_ptr child = child_buf[0];
    XL(child);
    key_type first_key = key_buf[1], last_key = key_buf.back();
    slot_type node_size = key_buf.size() - 1;
    bool flag = false;
    while (check_extend_head(first_key, last_key, node_size, child)){
        //AEX_WARNING("[extend head] node->type=" << to_string(node->type));
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
        //AEX_PRINT("child=" << child << ", child->type=" << to_string(child->type) << ", child->size=" << child->size);
        free_node_helper(child);
        first_key = tmp_key[1];
        child = tmp_child[0];
        for (slot_type i = tmp_key.size() - 1; i >= 0; --i){
            key_buf.emplace_back(tmp_key[i]);
            child_buf.emplace_back(tmp_child[i]);
        }
        XL(child);
    }
    XU(child);

    if (flag){
        std::reverse(key_buf.begin(), key_buf.end());
        std::reverse(child_buf.begin(), child_buf.end());
    }
    //AEX_PRINT("extend_head end");
    child = child_buf.back();
    XL(child);
    while (check_extend_tail(first_key, last_key, node_size, child)){
        //AEX_WARNING("[extend tail] node->type=" << to_string(node->type));
        tmp_key.clear();
        tmp_child.clear();
        child = child_buf.back();
        child_buf.pop_back();
        key_buf.pop_back();
        get_childs(i_n(child), key_buf, child_buf);
        free_node_helper(child);
        child = child_buf.back();
        XL(child);
    }
    XU(child);
    //AEX_PRINT("extend_tail end");
    AEX_ASSERT(is_sorted(key_buf.data(), key_buf.size()));
}

}