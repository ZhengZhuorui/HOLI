#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::_get_info_stats(const node_ptr node, const unsigned int depth, info_stats& stats) const {
    stats.max_depth = std::max(stats.max_depth, depth);
    ++stats.level_node[depth];
    if (!check_unlock(node))
        AEX_ERROR("node is lock!, node=" << node << ", type=" << to_string(node->type));
    if (!check_node(node))
        AEX_ERROR("node exists error!, node=" << node << ", type=" << to_string(node->type));
    switch (node->type){
        case NodeType::LeafNode:{
            stats.data_node_memory_used += sizeof(data_node);
            ++stats.data_node_cnt;
            stats.size += l_n(node)->size;
            stats.tot_depth += depth * l_n(node)->size;
            break;
        }
        case NodeType::DenseNode:{
            stats.dense_node_memory_used += Allocator::DENSE_NODE_MEMORY_USED();
            ++stats.dense_node_cnt;
            stats.dense_node_childs += d_n(node)->size;
            for (slot_type i = 0; i < d_n(node)->size; ++i){
                _get_info_stats(d_n(node)->child_ptr[i], depth + 1, stats);
            }
            break;
        }
        case NodeType::HashNode:{
            key_type key;
            node_ptr child;
            //AEX_PRINT(Allocator::HASH_NODE_MEMORY_USED(h_n(node)->slot_size));
            stats.hash_node_memory_used += Allocator::HASH_NODE_MEMORY_USED(h_n(node)->slot_size);
            if constexpr (traits::AllowConcurrency)
                stats.hash_node_memory_used += sizeof(Lock) * (h_n(node)->slot_size / 64);
            ++stats.hash_node_cnt;
            stats.hash_node_childs += h_n(node)->size;
            int cnt = 0;
            for(slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                ++cnt;
                std::tie(key, child) = hash_table.find(h_n(node), i);
                _get_info_stats(child, depth + 1, stats);
            }
            //if (node == this->root)
            //    AEX_PRINT("node->slot_size=" << h_n(node)->slot_size << ", node->size=" << cnt);
            break;
        }
    }
}

template<typename _Key, typename _Val, typename traits>
info_stats aex_tree<_Key, _Val, traits>::get_info_stats() const {
    info_stats stats = info_stats();
    if (this->root == nullptr){
        return stats;
    }
    stats.hash_table_memory_used = hash_table.memory_used();
    [[maybe_unused]]key_type prev_key = std::numeric_limits<key_type>::lowest();
    long long cnt = 0;
    for (auto it = this->begin(); it != this->end(); ++it){
        AEX_DEBUG_BLOCK({if (it.key() < prev_key) AEX_PRINT("cnt=" << cnt << ", it.key()=" << it.key() << ", prev_key=" << prev_key << ", it.offset=" << it.offset);});
        AEX_ASSERT(it.key() >= prev_key);
        prev_key = it.key();
        ++cnt;
    }
    cnt = 0;
    _get_info_stats(this->root, 0, stats);
    for (data_node_ptr inode = this->head_leaf; inode != nullptr; inode = inode->next)
        ++cnt;
    if (cnt != (long long)stats.data_node_cnt){
        AEX_ERROR("Error! node data node error. cnt=" << cnt << ", stats.data_node_cnt=" << stats.data_node_cnt);
    }
    //AEX_DEBUG_BLOCK({if ((LL)stats.hash_node_childs != hash_table.size.load()) AEX_PRINT((LL)stats.hash_node_childs << ", " << hash_table.size.load());});
    //AEX_ASSERT((LL)stats.hash_node_childs == hash_table.size.load());
    stats.memory_used = stats.hash_table_memory_used + stats.data_node_memory_used + stats.hash_node_memory_used + stats.dense_node_memory_used;

    return stats;
}

/*
template<typename _Key, typename _Val, typename traits>
size_t aex_tree<_Key, _Val, traits>::memory_used() const {
    size_t mem = hash_table.memory_used();
    AEX_ASSERT(root != nullptr);
    std::queue<node_ptr> que;
    que.push(root);
    while (!que.empty()){
        node_ptr node = que.front();
        que.pop();
        switch (node->type){
            case NodeType::LeafNode:{
                mem += sizeof(data_node);
                break;
            }
            case NodeType::DenseNode:{
                mem += Allocator::DENSE_NODE_MEMORY_USED(d_n(node)->slot_size);
                for (slot_type i = 0; i < node->size; ++i)
                    que.push(d_n(node)->child_ptr[i]);
                break;
            }
            case NodeType::HashNode:{
                mem += Allocator::HASH_NODE_MEMORY_USED(d_n(node)->slot_size);
                for (slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                    node_ptr child = hash_table.table_[hash_table.get_hash_key(node, i)]->find(node, i).second;
                    AEX_ASSERT(child != nullptr);
                    que.push_back(child);
                }
                break;
            }
        }
    }
}
*/

}