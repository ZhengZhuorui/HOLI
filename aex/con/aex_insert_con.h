#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    this->balance_stats.update_timestamp();
    std::pair<iterator, bool> ret;

    if (root == nullptr){
        std::lock_guard<std::shared_mutex>(this->mtx);
        root = head_leaf = tail_leaf = node_allocator.allocate_static_data_node();
        static_cast<data_node_ptr>(root)->insert(key, value, 0);
        root->next = empty_leaf;
        empty_leaf->prev = root;
        root->prev = nullptr;
        ++m_stats.level_node[0];
        m_stats.height = 1;
        ++this->m_stats.size;
        return std::pair<iterator, bool>(iterator(head_leaf, 0), true);
    }
    
    data_node_ptr node = find_leaf_lock_con(key);
    
    slot_type pos = node->find_lower_pos(key);

    /* find the insert position */
    if (pos < node->size && node->key[pos] == key){
        return false;
    }

    /* if data node is full, split the node */
    if (isfull(node)){
        data_node_ptr new_node = node_allocator.allocate_static_data_node();
        ++this->m_stats.level_node[0];
        if (node->prev != nullptr){
            node->prev->node_mutex->lock();
        }
        split(new_node, node);
        if (pos < new_node->size)
            new_node->insert(key, value, pos);
        else
            node->insert(key, value, pos - new_node->size);
        key_type new_key = new_node->key[new_node->size - 1];
        node_ptr _ = static_cast<node_ptr>(new_node);
        insert_nodes_con(node->parent, &new_key, &_, 1);
        node->unlock();
    }
    /* else insert the position of the data node*/ 
    else{
        node->insert(key, value, pos);
        node->unlock();
    }
    ++m_stats.size;
    return true;
}

// Split an node when the node insert item and (the size is larger than full ratio or no empty slot to insert)
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_split_pipeline_con(inner_node_ptr node, const key_type* key, const node_ptr* child, const slot_type n){    
    AEX_PRINT("pipeline");
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_split_pipeline_cnt;
    #endif
    inner_node_ptr parent = node->parent;
    bool append_flag = (parent != nullptr);
    slot_type start = 0, ans_size, ans_slot_size, size, left_size;
    bool ml_flag;
    AEX_ASSERT(IS_ML_NODE(node) == true);
    AEX_ASSERT(node->size + n <= node->slot_size);
    copy_to_buffer(node, node->key_ptr, node->child_ptr);
    if (n > 0){
        slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, key[0]) - node->key_ptr;
        std::move_backward(node->key_ptr + pos, node->key_ptr + node->size - 1, node->key_ptr + node->size + n - 1);
        std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + n);
        std::copy(key, key + n, node->key_ptr + pos);
        std::copy(child, child + n, node->child_ptr + pos);
        node->size += n;
    }
    size = node->size;
    
    unsigned long long recent_update_timestamp = this->balance_stats.get_timestamp();
    node->balance_stats.update_train_frequency(recent_update_timestamp);
    double train_times = node->balance_stats.get_train_times(), write_times = node->balance_stats.get_write_times();
        
    if (check_split(node)){
        left_size = size >> 1;
        while (start < left_size){
            std::tie(ans_size, ans_slot_size, ml_flag) = split_with_exponential_probe(node->key_ptr + start, left_size - start, node->level);
            inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
            new_node->node_mutex->lock();
            new_node->level = node->level;
            ++this->m_stats.level_node[new_node->level];
            if (ml_flag)
                new_node->model.train(node->key_ptr + start, ans_size - 1, ans_slot_size);
            new_node->construct(node->key_ptr + start, node->child_ptr + start, ans_size);
            new_node->balance_stats = node_balance_stats(recent_update_timestamp,
                                        train_times, 
                                        write_times * (1.0 * new_node->size / node->size));
            append_flag &= (!isfull(parent));
            if (append_flag)
                append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], node);
            if (!append_flag){
                insert_split_bulk_load_con(node, start + ans_size, node->key_ptr[start + ans_size - 1], new_node, true);
                return;
            }
            link_to_next_node(new_node, node);
            start += ans_size;
        }
    }

    while (start < size){
        std::tie(ans_size, ans_slot_size, ml_flag) = split_with_exponential_probe(node->key_ptr + start, size - start, node->level);
        //AEX_PRINT("ans_size=" << ans_size << ", ans_slot_size=" << ans_slot_size << ", ml_flag=" << ml_flag << ", start=" << start << ", size=" << size);
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
        new_node->node_mutex->lock();
        new_node->level = node->level;
        ++this->m_stats.level_node[new_node->level];
        if (ml_flag)
            new_node->model.train(node->key_ptr + start, ans_size - 1, ans_slot_size);
        new_node->construct(node->key_ptr + start, node->child_ptr + start, ans_size);
        if (ans_size != size){
            new_node->balance_stats = node_balance_stats(recent_update_timestamp,
                                            train_times, 
                                            write_times * (1.0 * new_node->size / node->size));
        }
        else{
            new_node->balance_stats = node->balance_stats;
        }

        if (start + ans_size == size){
            node_ptr prev = node->prev, next = node->next;
            *node = std::move(*new_node);
            --this->m_stats.level_node[new_node->level];
            node_allocator.free_node(new_node);
            node->parent = parent;
            node->prev = prev;
            node->next = next;
        }
        else{
            append_flag &= (!isfull(parent));
            if (append_flag)
                append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], new_node);
            
            if (!append_flag) {
                insert_split_bulk_load_con(node, start + ans_size, node->key_ptr[start + ans_size - 1], new_node, false);
                return;
            }
            link_to_next_node(new_node, node);
        }
        start += ans_size;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert_split_helper_con(inner_node_ptr node, const key_type* key_buf, node_ptr* child_buf, const slot_type n){
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_split_cnt;
    #endif 
    if (node == root){
        insert_split_by_buffer_con(node, new_key, new_child, n);
        return;
    }

    // node != root
    if (!IS_ML_NODE(node)){
        if (node->slot_size * traits::EXPAND_RATIO < traits::MIN_ML_INNER_NODE_SIZE && n < node->slot_size / 2){
            insert_split_dense_inner_node_con(node, new_key, new_child, n);

        }
        else{
            insert_split_by_buffer_con(node, new_key, new_child, n);
        }
    }
    else if (node->size + n < node->slot_size){
        insert_split_pipeline_con(node, new_key, new_child, n);
    }
    else{
        insert_split_by_buffer_con(node, new_key, new_child, n);
    }

    for (int i = 0; i < n; ++i)
        child_buf[i]->node_mutex->unlock();
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert_nodes_con(inner_node_ptr node, const key_type* key_buf, node_ptr* child_buf, const slot_type n){
    if (node == nullptr){
        add_root(key_buf, child_buf, n);
        return true;
    }
    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    node->lock_shared();
    if (isfull(node, n - 1)) {
        if (check_split(node) == true){
            insert_split_helper_con(node, key_buf, child_buf, n);
            return true;
        }
        bool flag;
        if (expand(node) == false){
            insert_split_helper_con(node, key_buf, child_buf, n);
            return true;
        }
    }
    for (slot_type i = 0; i < n; ++i){
        if (!node->insert(key_buf[i], child_buf[i])){
            insert_split_helper_con(node, key_buf + i, child_buf + i, n - i);
            return true;
        }
    }
    return false;
}

}