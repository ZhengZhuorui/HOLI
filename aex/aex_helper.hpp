#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::_get_info_stats(const node_ptr node, const unsigned int depth, info_stats& stats){
    stats.max_depth = std::max(stats.max_depth, depth);
    ++stats.level_node[depth];
    if (!check_unlock(node))
        AEX_PRINT("node=" << node << ", type=" << to_string(node->type));
    if (!check_unlock_shared(node))
        AEX_PRINT("node=" << node << ", type=" << to_string(node->type));

    switch (node->type){
        case NodeType::LeafNode:{
            stats.memory_used += sizeof(data_node);
            ++stats.data_node_cnt;
            stats.size += l_n(node)->size;
            stats.tot_depth += depth * l_n(node)->size;
            for (slot_type i = 0; i < l_n(node)->size - 1; ++i)
                AEX_ASSERT(l_n(node)->key[i] <= l_n(node)->key[i + 1]);
            break;
        }
        case NodeType::DenseNode:{
            stats.memory_used += Allocator::DENSE_NODE_MEMORY_USED(d_n(node)->slot_size);
            ++stats.dense_node_cnt;
            if (d_n(node)->size >= traits::MIN_HASH_NODE_SIZE)
                ++stats.try_learn_dense_node_cnt;
            stats.dense_node_childs += d_n(node)->size;
            //if (d_n(node)->try_learn)
            //    ++stats.try_learn_dense_node_cnt;
            for (slot_type i = 0; i < d_n(node)->size - 1; ++i)
                AEX_ASSERT(d_n(node)->key_ptr[i] <= d_n(node)->key_ptr[i + 1]);
            for (slot_type i = 0; i < d_n(node)->size; ++i){
                _get_info_stats(d_n(node)->child_ptr[i], depth + 1, stats);
            }
            break;
        }
        case NodeType::HashNode:{
            stats.memory_used += Allocator::HASH_NODE_MEMORY_USED(h_n(node)->slot_size);
            ++stats.hash_node_cnt;
            stats.hash_node_childs += h_n(node)->size;
            [[maybe_unused]]key_type his_key = std::numeric_limits<key_type>::lowest();
            slot_type cnt = 0;
            for(slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                key_type key;
                node_ptr child;
                std::tie(key, child) = hash_table.find(node, i);
                AEX_ASSERT(his_key <= key);
                ++cnt;
                his_key = key;
                _get_info_stats(child, depth + 1, stats);
            }
            for (slot_type i = 0; i < h_n(node)->slot_size; i += traits::SLOT_PER_LOCK){
                key_type key;
                node_ptr child;
                std::tie(key, child) = hash_table.find(node, i);
                AEX_ASSERT(child != nullptr);
            }
            AEX_ASSERT(cnt == h_n(node)->size);
            break;
        }
    }
}

template<typename _Key, typename _Val, typename traits>
info_stats aex_tree<_Key, _Val, traits>::get_info_stats(){
    info_stats stats;
    XL();
    if (this->root == nullptr){
        XU();
        return stats;
    }
    stats.memory_used = hash_table.memory_used();
    [[maybe_unused]]key_type prev_key = std::numeric_limits<key_type>::lowest();
    long long cnt = 0;
    for (iterator it = this->begin(); it != this->end(); ++it){
        AEX_ASSERT(it.key() >= prev_key);
        prev_key = it.key();
        ++cnt;
    }
    AEX_ASSERT(cnt == this->m_stats.size);
    cnt = 0;
    _get_info_stats(this->root, 0, stats);
    for (data_node_ptr inode = this->head_leaf; inode != nullptr; inode = inode->next)
        ++cnt;
    AEX_ASSERT(cnt == (long long)stats.data_node_cnt);
    return stats;
}

}