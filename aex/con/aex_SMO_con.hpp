#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
inline void aex_tree_con<_Key, _Val, traits>::split(data_node_ptr new_node, data_node_ptr old_node){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    #ifdef AEX_EXPERIMENT
    ++opt_stats.data_node_split_cnt;
    #endif
    new_node->next = old_node;
    new_node->prev = old_node->prev;
    if (old_node->prev != nullptr){
        XL(old_node->prev);
        old_node->prev->next = new_node;
        XU(old_node->prev);
    }
    old_node->prev = new_node;

    {
        std::shared_lock(std::shared_mutex) lock();
        if (head_leaf == old_node) head_leaf = new_node;
    }
    
    //new_node->parent = old_node->parent;
    
    size_type mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    std::move(old_node->key, old_node->key + mid, new_node->key);
    std::move(old_node->data, old_node->data + mid, new_node->data);
    std::move(old_node->key + mid, old_node->key + old_node->size, old_node->key);
    std::move(old_node->data + mid, old_node->data + old_node->size, old_node->data);

    old_node->size = old_node->size - mid;
    new_node->size = mid;
}

}