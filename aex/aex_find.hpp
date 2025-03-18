namespace aex{

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const hash_node_ptr node, const key_type key) const {
    AEX_ASSERT(node->is_occupied(0));
    slot_type pos = node->predict(key), pos1;
    key_type find_key;
    node_ptr res = nullptr;
    if (node->is_occupied(pos) || (pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        std::tie(find_key, res) = hash_table.find(node, pos);
    if (res == nullptr || find_key > key){
        pos1 = node->prev_item_find(pos - 1);
        std::tie(find_key, res) = hash_table.find(node, pos1);
    }
    AEX_ASSERT(find_key <= key);
    AEX_ASSERT(res != nullptr);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const dense_node_ptr node, const key_type key) const {
    slot_type pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    //slot_type pos = std::upper_bound(node->key_ptr, node->key_ptr + node->size, key) - node->key_ptr - 1;
    return node->child_ptr[pos];
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const inner_node_ptr node, const key_type key) const {
    switch (node->type){
        case NodeType::HashNode  : { return find(h_n(node), key); }
        case NodeType::DenseNode : { return find(d_n(node), key); }
        default : { AEX_ASSERT(0 == 1); return nullptr;}
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::range_query(const key_type lower_key, const key_type upper_key, std::vector<std::pair<key_type, value_type>>& answer) const {
    if constexpr (traits::AllowConcurrency){
        const_cast<self*>(this)->range_query_con(lower_key, upper_key, answer);
        return;
    }
    answer.clear();
    data_node_ptr inode = find_leaf(lower_key);
    int pos = inode->find_lower_pos(lower_key);
    while (inode != nullptr){
        //if constexpr(sizeof(key_type) == 8 && sizeof(value_type) == 8){
        //    for (; pos + 8 <= inode->size && inode->key[pos + 7] <= upper_key; pos += 8){
        //        ULL _size = answer.size();
        //        answer.resize(_size + 8);
        //        pack_pair_avxx8(inode->key + pos, inode->data + pos, answer.data() + _size);
        //    }
        //}
        for (; pos < inode->size && inode->key[pos] <= upper_key; ++pos){
            AEX_ASSERT(inode->key[pos] >= lower_key);
            answer.emplace_back(inode->key[pos], inode->data[pos]);
        }
        if (inode->next == nullptr || pos < inode->size) 
            break;
        
        inode = inode->next;
        pos = 0;
    }
}

template<typename _Key, typename _Val, typename traits>
inline size_t aex_tree<_Key, _Val, traits>::range_query_len(std::pair<key_type, value_type>* results, const key_type lower_key, const size_t key_num) const {
    if constexpr (traits::AllowConcurrency){
        return const_cast<self*>(this)->range_query_len_con(results, lower_key, key_num);
    }
    AEX_ASSERT(!traits::AllowConcurrency);
    data_node_ptr inode = find_leaf(lower_key);
    int pos = inode->find_lower_pos(lower_key);
    size_t cnt = 0;
    while (inode != nullptr){
        //if constexpr(sizeof(key_type) == 8 && sizeof(value_type) == 8){
        //    for (; cnt + 8 <= key_num && pos + 8 <= traits::DATA_NODE_SLOT_SIZE && pos <= inode->size; cnt += std::min((size_type)8, inode->size - pos), pos += 8){
        //        pack_pair_avxx8(inode->key + pos, inode->data + pos, results + cnt);
        //    }
        //}
        for (; cnt < key_num && pos < inode->size; ++pos, ++cnt){
            AEX_ASSERT(inode->key[pos] >= lower_key);
            results[cnt++] = std::make_pair(inode->key[pos], inode->data[pos]);
        }
        if (inode->next == nullptr || pos < inode->size || cnt == key_num) {
            return cnt;
        }
        inode = inode->next;
        pos = 0;
    }
    return cnt;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf(const key_type key) const {
    node_ptr node = root;
    while (node->type != NodeType::LeafNode)
        node = find(i_n(node), key);
    return l_n(node);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_insert(const hash_node_ptr node, const key_type key, slot_type &pos) const {
    pos = node->predict(key);
    key_type find_key;
    node_ptr res = nullptr;
    if (node->is_occupied(pos)){
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(res != nullptr);
        if (find_key > key)
            res = nullptr;
    }

    if (res == nullptr){
        pos = node->prev_item_find(pos - 1);
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(find_key <= key);
    }
    AEX_ASSERT(res != nullptr);
    AEX_DEBUG_BLOCK({if (res->type != NodeType::LeafNode) AEX_ASSERT(find_key <= node_zero_key(i_n(res)));});
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_insert(const dense_node_ptr node, const key_type key, slot_type &pos) const {
    //pos = aex::linear_search_upper_bound_avx512x8(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    //pos = linear_search_upper_bound_avx512x8(node->key_ptr, node->size, key) - 1;
    pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    return node->child_ptr[pos];
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_insert(const inner_node_ptr node, const key_type key, slot_type &pos) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_insert(h_n(node), key, pos); }
        case NodeType::DenseNode : { return find_insert(d_n(node), key, pos); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase(hash_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const {
    pos = node->predict(key);
    key_type find_key;
    node_ptr res = nullptr;

    if (node->is_occupied(pos)){
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(res != nullptr);
        if (find_key > key){
            next_pos = pos;
            res = nullptr;
        }
        else
            next_pos = node->next_item(pos + 1);  
    }
    else
        next_pos = node->next_item(pos + 1);

    if (res == nullptr){
        slot_type prev_pos = node->prev_item(pos - 1);
        pos = prev_pos;
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(find_key <= key);
    }
    AEX_ASSERT(res != nullptr);
    AEX_DEBUG_BLOCK({if (res->type != NodeType::LeafNode) AEX_ASSERT(find_key <= node_zero_key(i_n(res)));});
    AEX_DEBUG_BLOCK({if (next_pos < node->slot_size){
        key_type next_key; node_ptr next_child;
        std::tie(next_key, next_child) = hash_table.find(node, next_pos);
        AEX_ASSERT(next_child != nullptr);
        AEX_ASSERT(key < next_key);}});
    return res;
}


template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase(const dense_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const {
    //pos = aex::linear_search_upper_bound_avx512x8(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    node_ptr res = node->child_ptr[pos];
    next_pos = pos + 1;
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase(inner_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_erase(h_n(node), key, pos, next_pos); }
        case NodeType::DenseNode : { return find_erase(d_n(node), key, pos, next_pos); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_tail_leaf(node_ptr node) {
    AEX_ASSERT(check_lock(node));
    node_ptr child;
    node_ptr tail_node = node;

    while (tail_node->type != NodeType::LeafNode){
        if (tail_node->type == NodeType::DenseNode)
            child = d_n(tail_node)->child_ptr[tail_node->size - 1];
        else
            child = h_n(tail_node)->tail_node;
        tail_node = child;
    }
    return l_n(tail_node);
}

}