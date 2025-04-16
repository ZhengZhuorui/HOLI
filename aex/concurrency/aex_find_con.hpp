#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const hash_node_ptr node, const key_type key, bool &need_restart) const {
    AEX_ASSERT(node != nullptr);
    AEX_ASSERT(node->is_occupied(0));
    slot_type pos = node->predict(key), pos1;
    //__builtin_prefetch((char*)(node->bitmap_ptr) + ((pos >> 3) & (~63)));
    key_type find_key;
    node_ptr res = nullptr;
    if (node->is_occupied(pos) || (pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        std::tie(find_key, res) = hash_table.find(node, pos, need_restart);
    if (need_restart) return res;
    
    if (res == nullptr || find_key > key){
        pos1 = node->prev_item_find(pos - 1);
        std::tie(find_key, res) = hash_table.find(node, pos1, need_restart);
    }
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_con(const dense_node_ptr node, const key_type key) const {
    //slot_type pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    //slot_type pos = std::upper_bound(node->key_ptr, node->key_ptr + node->size, key) - node->key_ptr - 1;
    slot_type pos;
    for (pos = 0; pos < node->size && node->key_ptr[pos] <= key; ++pos);
    --pos;
    return node->child_ptr[pos];
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_insert(const hash_node_ptr node, const key_type key, slot_type &pos, bool &need_restart) const {
    AEX_ASSERT(node != nullptr);
    pos = node->predict(key);
    key_type find_key;
    node_ptr res = nullptr;
    if (node->is_occupied(pos)){
        std::tie(find_key, res) = hash_table.find(node, pos, need_restart);
        if (need_restart) return res;
        AEX_ASSERT(res != nullptr);
        if (find_key > key)
            res = nullptr;
    }

    if (res == nullptr){
        pos = node->prev_item_find(pos - 1);
        std::tie(find_key, res) = hash_table.find(node, pos, need_restart);
    }
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_insert_con(const dense_node_ptr node, const key_type key, slot_type &pos) const {
    //pos = aex::linear_search_upper_bound_avx512x8(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    //pos = linear_search_upper_bound_avx512x8(node->key_ptr, node->size, key) - 1;
    //pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    for (pos = 0; pos < node->size && node->key_ptr[pos] <= key; ++pos);
    --pos;
    return node->child_ptr[pos];
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf_con(const key_type key, version_type &node_version) {
    int restart_count = 0;
find_leaf_con_start:
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    node_ptr node = root, child;
    while(node->type != NodeType::LeafNode){
        node_version = node->node_lock.readLockOrRestart(need_restart); // SL(child)
        if (need_restart) goto find_leaf_con_start;

        if (node->type == NodeType::HashNode){
            hash_node_ptr node_copy = h_n(node)->copy;
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto find_leaf_con_start;
            //child = find(&node_copy, key);
            child = find(node_copy, key, need_restart);
            if (need_restart) goto find_leaf_con_start;
        }
        else{
            child = find(d_n(node), key); // child is not lock shared
            node->node_lock.checkOrRestart(node_version, need_restart);  // check child exists
        }
        if (need_restart) goto find_leaf_con_start;
        node = child;
    }
    node_version = node->node_lock.readLockOrRestart(need_restart);
    if (need_restart) goto find_leaf_con_start;

    if (l_n(node)->next_min_key < key){
        need_restart = true;
        goto find_leaf_con_start;
    }

    return l_n(node); // node is lock shared with node_version
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf_con(const key_type key, version_type &node_version, bool &need_restart) const{
    node_ptr node = root, child;
    while(node->type != NodeType::LeafNode){
        node_version = node->node_lock.readLockOrRestart(need_restart); // SL(child)
        if (need_restart) return nullptr;
        if (node->type == NodeType::HashNode){
            hash_node_ptr node_copy = h_n(node)->copy;
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) return nullptr;
            child = find(node_copy, key, need_restart);
        }
        else{
            child = find_con(d_n(node), key); // child is not lock shared
            node->node_lock.checkOrRestart(node_version, need_restart);  // check child exists
        }
        if (need_restart) return nullptr;
        node = child;
    }
    node_version = node->node_lock.readLockOrRestart(need_restart);
    if (need_restart || l_n(node)->next_min_key < key){
        need_restart = true;
        return nullptr;
    }
    return l_n(node); // node is lock shared with node_version
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::find_con(const key_type key, value_type &value) {
    EpochGuard guard(this);
    int restart_count = 0;
    version_type node_version;
find_con_start:
    AEX_SGL_ASSERT(restart_count == 0);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool ret = true;
    bool need_restart = false;
    //data_node_ptr node = find_leaf_con(key, node_version);
    data_node_ptr node = find_leaf_con(key, node_version, need_restart);
    if (need_restart) goto find_con_start;

    slot_type pos = node->find(key);
    if (pos > node->size || pos < 0)
        ret = false;
    else
        value = node->data[pos];
    node->node_lock.checkOrRestart(node_version, need_restart);
    if (need_restart) goto find_con_start;
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::lower_bound_con(const key_type key, std::pair<key_type, value_type> &value) {
    EpochGuard guard(this);
    int restart_count = 0;
    version_type node_version, next_node_version;
lower_bound_con_start:
    AEX_SGL_ASSERT(restart_count == 0);
    AEX_ASSERT(restart_count < 100000000);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    data_node_ptr node, next_node;
    node = find_leaf_con(key, node_version, need_restart);
    if (need_restart) goto lower_bound_con_start;
    slot_type pos = node->find_lower_pos(key);
    node->node_lock.checkOrRestart(node_version, need_restart);
    if (need_restart) goto lower_bound_con_start;
    if (pos > node->size){
        next_node = node->next;
        if (next_node != nullptr){
            next_node_version = next_node->node_lock.readLockOrRestart(need_restart);
            if (need_restart) goto lower_bound_con_start;
        }
        node->node_lock.readUnlockOrRestart(node_version, need_restart);
        if (need_restart) goto lower_bound_con_start;
        if (next_node == nullptr)            
            return false;
        node = next_node;
        node_version = next_node_version;
        value = std::make_pair(node->key[0], node->data[0]);
    }    
    else value = std::make_pair(node->key[pos], node->data[pos]);
    node->node_lock.readUnlockOrRestart(node_version, need_restart);
    if (need_restart) goto find_con_start;
    return true;

}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::range_query_con(const key_type lower_key, const key_type upper_key, std::vector<std::pair<key_type, value_type>>& answer)  {        
    EpochGuard guard(this);
    int restart_count = 0;
range_query_con_start:
    AEX_SGL_ASSERT(restart_count == 0);
    AEX_ASSERT(restart_count < 100000000);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    answer.clear();
    version_type node_version, next_node_version;
    data_node_ptr inode = find_leaf_con(lower_key, node_version, need_restart), next_node;
    if (need_restart) goto range_query_con_start;
    int pos = inode->find_lower_pos(lower_key);
    node_version = inode->node_lock.readLockOrRestart(need_restart);
    if (need_restart) goto range_query_con_start;
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
        if (inode->next == nullptr || pos < inode->size) {
            inode->node_lock.readUnlockOrRestart(node_version, need_restart);
            if (need_restart) goto range_query_con_start;
            break;   
        }
        next_node = inode->next;
        if (next_node != nullptr)
            next_node_version = next_node->node_lock.readLockOrRestart(need_restart);
        if (need_restart) goto range_query_con_start;
        inode->node_lock.readUnlockOrRestart(node_version, need_restart);
        if (need_restart) goto range_query_con_start;
        inode = next_node;
        node_version = next_node_version;
        pos = 0;
    }
}

template<typename _Key, typename _Val, typename traits>
inline size_t aex_tree<_Key, _Val, traits>::range_query_len_con(std::pair<key_type, value_type>* results, const key_type lower_key, const size_t key_num) {
    EpochGuard guard(this);
    int restart_count = 0;
range_query_len_con_start:
    AEX_SGL_ASSERT(restart_count == 0);
    AEX_ASSERT(restart_count < 100000000);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    size_t cnt = 0;
    version_type node_version, next_node_version;
    data_node_ptr inode = find_leaf_con(lower_key, node_version, need_restart), next_node;
    if (need_restart) goto range_query_len_con_start;
    int pos = inode->find_lower_pos(lower_key);
    //node_version = inode->node_lock.readLockOrRestart(need_restart); //SL(inode)
    //if (need_restart) goto range_query_len_con_start;

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
            inode->node_lock.readUnlockOrRestart(node_version, need_restart);
            if (need_restart) goto range_query_len_con_start;
            return cnt;
        }
        next_node = inode->next;
        if (next_node != nullptr)
            next_node_version = next_node->node_lock.readLockOrRestart(need_restart); // SL(next_node);
        if (need_restart) goto range_query_len_con_start;
        inode->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node);
        if (need_restart) goto range_query_len_con_start;
        inode = next_node;
        node_version = next_node_version;
        pos = 0;
    }
    return cnt;
}

/*
template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase_con(hash_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const {
    int restart_count = 0;
find_erase_start:
    if (restart_count > 0)
        yield(restart_count);
    restart_count++;
    bool restart = false;
    pos = node->predict(key);
    key_type find_key;
    node_ptr res = nullptr;
    node->array_lock_shared(pos - 1, pos);

    if (node->is_occupied(pos)){
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(res != nullptr);
        if (find_key > key){
            next_pos = pos;
            res = nullptr;
        }
        else
            next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);  
    }
    else
        next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);

    if (res == nullptr){
        slot_type prev_pos = node->try_array_lock_shared_until_prev_item(pos - 1, restart);
        if (restart){
            node->array_unlock_shared(pos - 1, next_pos);
            goto find_erase_start;
        }
        pos = prev_pos;
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(find_key <= key);
    }
    AEX_ASSERT(res != nullptr);
    AEX_DEBUG_BLOCK({if (res->type != NodeType::LeafNode) AEX_ASSERT(find_key <= node_zero_key(i_n(res)));});
    AEX_DEBUG_BLOCK({if (next_pos < node->slot_size){
        key_type next_key;
        node_ptr next_child;
        std::tie(next_key, next_child) = hash_table.find(node, next_pos);
        if (key >= next_key){
            AEX_PRINT("key=" << key << ", start=" << node->model.args.start << ", find_key=" << find_key << ", next_key=" << next_key << ", pos=" << pos << ", next_pos=" << next_pos << ", next_child=" << next_child);
            AEX_PRINT("predict=" << node->predict(key) << ", slope=" << node->model.args.slope << ", predict - 1 =" << (key - node->model.args.start) * node->model.args.slope << ", predict=" << (key - node->model.args.start) * node->model.args.slope + 1);
        }
        AEX_ASSERT(next_child != nullptr);
        AEX_ASSERT(key < next_key);}});
    AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency) if (pos - 1 >= 0) if (!node->lock_array[pos2slot(pos - 1)].is_lock_shared()) {
        AEX_ERROR("node=" << node << ", pos=" << pos);AEX_ASSERT(node->lock_array[pos2slot(pos - 1)].is_lock_shared());}});
    AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency) if (next_pos < node->slot_size) AEX_ASSERT(node->lock_array[pos2slot(next_pos)].is_lock_shared());});
    SL(res);
    return res;
}


template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase(const dense_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos) const {
    //pos = aex::linear_search_upper_bound_avx512x8(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    pos = linear_search_upper_bound_avx512<traits::DENSE_NODE_SLOT_SIZE>(node->key_ptr, node->size, key) - 1;
    node_ptr res = node->child_ptr[pos];
    next_pos = pos + 1;
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_erase_con(inner_node_ptr node, const key_type key, slot_type &pos, slot_type &next_pos, bool &need_restart) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_erase(h_n(node), key, pos, next_pos, node_version, need_restart); }
        case NodeType::DenseNode : { return find_erase(d_n(node), key, pos, next_pos, node_version, need_restart); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}
*/

}
