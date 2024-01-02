#pragma once

template<typename _Key, typename _Val, typename traits>
inline void aex_tree_con<_Key, _Val, traits>::add_root(const key_type* key_buf, node_ptr* child_buf, slot_type n){
    lock_guard<std::shared_lock>(this->mutex);
    size_type slot_size = min_slot_size(n + 1, traits::MIN_INNER_NODE_SLOT_SIZE);
    inner_node_ptr now_inner_node = node_allocator.allocate_inner_node_con(slot_size, false);
    ++this->m_stats.level_node[this->m_stats.height];
    now_inner_node->level = this->m_stats.height;
    ++this->m_stats.height;
    now_inner_node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    now_inner_node->prev = now_inner_node->next = nullptr;
    now_inner_node->construct(key_buf, child_buf, n);
    {
        now_inner_node->key_ptr[now_inner_node->size - 1] = key_buf[n - 1];
        now_inner_node->child_ptr[now_inner_node->size] = root;
        root->parent = now_inner_node;
        ++now_inner_node->size;
    }
    this->root = now_inner_node;
}

