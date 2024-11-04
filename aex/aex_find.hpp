namespace aex{

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const hash_node_ptr node, const key_type &key) const {
    AEX_ASSERT(node->is_occcupied(0));
    slot_type pos = node->predict(key), pos1;
    key_type _;
    node_ptr res = nullptr;
    //if (bitmap_impl::at(node->bitmap_ptr, pos)) 
    node->array_lock_shared(pos - 1, pos);
    if (node->is_occupied(pos))
        res = hash_table.find(node, pos, key);
    if (res == nullptr){
        pos1 = node->prev_item_find(pos - 1);
        std::tie(_, res) = hash_table.find(node, pos1);
    }
    AEX_ASSERT(res != nullptr);
    SL(res);
    node->array_unlock_shared(pos - 1, pos);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const dense_node_ptr node, const key_type &key) const {
    //return node->child_ptr[aex:::lower_bound_with_error_bound(node->key_ptr, node->key_ptr + node->size, x) - node->key_ptr];
    slot_type pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    node_ptr res = node->child_ptr[pos];
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const inner_node_ptr node, const key_type &key) const {
    switch (node->type){
        case NodeType::HashNode  : { return find(h_n(node), key); }
        case NodeType::DenseNode : { return find(d_n(node), key); }
        default : { AEX_ASSERT(0 == 1); return nullptr;}
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_con(const hash_node_ptr node, const key_type &key) const {
    slot_type pos = node->predict(key);
    node_ptr res = nullptr;
    key_type _;
    node->array_lock_shared(pos - 1, pos);
    //if (bitmap_impl::at(node->bitmap_ptr, pos)) 
    if (node->is_occupied(pos))
        res = hash_table.find(node, pos, key);
    else{
        slot_type pos1 = node->prev_item_find(pos - 1);
        res = hash_table.find(node, pos1, key);
        std::tie(_, res) = hash_table.find(node, pos1);
    }
    SL(res);
    node->array_unlock_shared(pos - 1, pos);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_con(const dense_node_ptr node, const key_type &key) const {
    slot_type pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    auto res = node->child_ptr[pos];
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_con(const inner_node_ptr node, const key_type &key) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_con(h_n(node), key); }
        case NodeType::DenseNode : { return find_con(d_n(node), key); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer)const{
    if constexpr(traits::AllowConcurrency){
range_query_start:
        answer.clear();
        data_node_ptr inode = find_leaf_con(lower_key);
        slot_type i;
        while (true){
            for (i = 0; i < inode->size; ++i){
                if (inode->key[i] <= upper_key)
                    answer.emplace_back(inode->key[i], inode->value[i]);
            }
            if (inode->next == nullptr || i < inode->size) {
                SU(inode);
                break;
            }
            SL(inode->next);
            SU(inode);
            inode = inode->next;
        }
    }
    else{
        iterator iter = this->find_iterator(lower_key);
        while(iter.key() <= upper_key){
            answer.emplace_back(iter->key(), iter->value());
            iter++;
        }
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf_con(const key_type &key) const {
    node_ptr node, child;
    version_type now_version;
find_leaf_con_start:
    node = root;
    now_version = this->version;
    if (key < this->min_key)
        node = head_leaf;
    SL(node);
    while(node->node_type != NodeType::LeafNode){
        child = find_con(node, key);
        SU(node);
    }
    if (l_n(node)->version > now_version){
        SU(node);
        goto find_leaf_con_start;
    }
    //return static_cast<data_node_ptr>(node);
    return l_n(node);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf(const key_type &key) const {
    if constexpr (traits::AllowConcurrency)
        return find_leaf_con(key);
    node_ptr node = root, child;
    if (key < this->min_key)
        node = n_n(head_leaf);
    while (node->type != NodeType::LeafNode){
        child = find(node, key);
        //AEX_ASSERT(check_lock_shared(child));
        //SU(node);
    }
    return static_cast<data_node_ptr>(node);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(hash_node_ptr node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const {
find_insert_start:
    bool restart = false;
    tail = true;
    pos = node->predict(key);
    node_ptr res = nullptr;
    slot_type prev_pos;
    //if (bitmap_impl::at(node->bitmap_ptr, pos)) 
    node->lock_shared(pos - 1, pos);
    if (next_pos < node->slot_size)
        tail = false;

    if (node->is_occupied(pos)){
        res = hash_table.find(node, pos, key);
        if (res == nullptr){
            next_pos = pos;
            tail = false;
        }
        else
            next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);  
    }
    else
        next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);

    if (res == nullptr){
        pos = node->try_array_lock_shared_until_prev_item(pos - 1, pos - 1, restart);
        AEX_ASSERT(pos == -1);
        if (restart){
            node->array_unlock_shared(pos - 1, next_pos);
            goto find_insert_start;
        }
        res = hash_table.find(node, pos, key);
    }

    AEX_ASSERT(res == nullptr);
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(dense_node_ptr node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const {
    pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    next_pos = pos + 1;
    node_ptr res = node->child_ptr[pos];
    tail = (pos == node->slot_size - 1);
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(inner_node_ptr node, const key_type &key, bool &tail, slot_type &pos, slot_type &next_pos) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_update(h_n(node), key, tail, pos, next_pos); }
        case NodeType::DenseNode : { return find_update(d_n(node), key, tail, pos, next_pos); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}
}