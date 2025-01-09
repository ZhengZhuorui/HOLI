#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_lock(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return node->node_lock.is_lock();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_lock_shared(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return node->node_lock.is_lock_shared();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_unlock(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return !node->node_lock.is_lock();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_unlock_shared(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return !node->node_lock.is_lock_shared();
    else
        return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(node_ptr node) const {
    switch (node->type){
        case NodeType::LeafNode : { return check_node(l_n(node));}
        case NodeType::DenseNode: { return check_node(d_n(node));}
        case NodeType::HashNode : { return check_node(h_n(node));}
        default: { AEX_ERROR("Unknown Type"); return false; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(data_node_ptr node) const {
    bool flag = true;
    //if (node->size == 0){
    //    AEX_ERROR("ERROR! size=0");
    //    flag = false;
    //}
    for (slot_type i = 0; i < node->size - 1; ++i)
    if (node->key[i] > node->key[i + 1]){
        AEX_ERROR("ERROR! key[" << i << "]=" << node->key[i] << ", key[i+1]=" << node->key[i + 1] << ", size=" << node->size);
        flag = false;
    }
    return flag;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(dense_node_ptr node) const {
    bool flag = true;
    if (node->size > traits::DENSE_NODE_SLOT_SIZE){
        AEX_ERROR("ERROR! dense node size larger than threshold, size=" << node->size);
        flag = false;
    }
    if  (node != root && (node->size == 0 || node->size == 1)){
        AEX_ERROR("ERROR! size=" << node->size);
        flag = false;
    }
    for (slot_type i = 0; i < node->size - 1; ++i){
        if (node->key_ptr[i] > node->key_ptr[i + 1]){
            AEX_ERROR("ERROR! key[" << i << "]=" << node->key_ptr[i] << ", key[i+1]=" << node->key_ptr[i + 1] << ", size=" << node->size);
            flag = false;
        }
        if (node->child_ptr[i] == node->child_ptr[i + 1]){
            AEX_ERROR("ERROR! child pointer is same");
            flag = false;
        }
    }
    return flag;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(hash_node_ptr node) const {
    bool flag = true;
    if (node->size < traits::MIN_HASH_NODE_CNT){
        AEX_ERROR("ERROR! hash node size=" << node->size << ", slot size=" << node->slot_size);
        flag = false;
    }
    key_type his_key = std::numeric_limits<key_type>::lowest();
    slot_type cnt = 0;
    for(slot_type i = 0; i < node->slot_size; i = node->next_item(i + 1)){
        key_type key;
        node_ptr child;
        std::tie(key, child) = hash_table.find(node, i);
        if (child == nullptr){
            AEX_ERROR("child == nullptr. slot=" << i << ", slot & 63=" << (i & 63));
            flag = false;
        }
        if (his_key > key){
            AEX_ERROR("key is not unordered. his_key=" << his_key << ", key=" << key);
            flag = false;
        }
        ++cnt;
        his_key = key;
    }
    if (cnt != node->size){
        AEX_ERROR("cnt != node->size. cnt=" << cnt << ", node->size=" << node->size);
        flag = false;
    }
    AEX_DEBUG_BLOCK({
        if constexpr(traits::AllowConcurrency) 
            for (slot_type i = 0; i < pos2slot(node->slot_size); ++i) {
                //if (node->lock_array[i].lockCount.load() != 0)
                //    AEX_ERROR("node=" << node << ", i=" << i << ", " << node->lock_array[i].lockCount.load());
                AEX_ASSERT(!node->lock_array[i].is_lock());
                AEX_ASSERT(!node->lock_array[i].is_lock_shared());
                //AEX_ASSERT(!node->update_lock_array[i].is_lock());
            }
    });
    return flag;
}

}