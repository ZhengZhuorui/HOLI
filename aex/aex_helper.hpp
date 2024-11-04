#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::_get_info_stats(const node_ptr node, const int depth, info_stats& stats){
    stats.max_depth = std::max(stats.max_depth, depth);
    ++stats.level_node[depth];
    if (!check_unlock(node))
        AEX_PRINT("node=" << node << ", type=" << type);
    if (!check_unlock_shared(node))
        AEX_PRINT("node=" << node << ", type=" << type);

    switch (node->type){
        case NodeType::LeafNode:{
            stats.memory_used += sizeof(data_node);
            ++stats.data_node_cnt;
            size += node->size;
            stats.tot_depth += depth * node->size;
            break;
        }
        case NodeType::DenseNode:{
            stats.memory_used += Allocator::DENSE_NODE_MEMORY_USED(d_n(node)->slot_size);
            ++stats.dense_node_cnt;
            stats.dense_node_childs += node->size;
            if (d_n(node)->try_learn)
                ++stats.try_learn_dense_node_cnt;
            for (slot_type i = 0; i < node->size; ++i)
                _get_info_stats(d_n(node)->child_ptr[i], depth + 1);
            break;
        }
        case NodeType::HashNode:{
            stats.memory_used += Allocator::HASH_NODE_MEMORY_USED(h_n(node)->slot_size);
            ++stats.hash_node_cnt;
            stats.hash_node_childs += node->size;
            for(slot_type i = 0; i < h_n(node)->slot_size; i = h_n(node)->next_item(i + 1)){
                key_type key;
                node_ptr child;
                std::tie(key, child) = hash_table.find(node, i);
                _get_info_stats(child, depth + 1);
            }
            break;
        }
    }
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::info_stats aex_tree<_Key, _Val, traits>::get_info_stats(){
    XL();
    info_stats stats;
    if (this->root != nullptr){
        stats.memory_used = hash_table.memory_used();
        _get_infomation_stats(this->root, 0, stats);
    }
    XU();
}

}