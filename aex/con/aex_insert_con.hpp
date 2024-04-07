#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    
    this->balance_stats.update_timestamp();
    int node_ptr* stack;
    int top;
    data_node_ptr* node = this->find_leaf_lock_con(key, stack, top);
    std::pair<iterator, bool> ret;

    if (root == nullptr){
        std::lock_guard<std::shared_mutex>(this->mtx);
        root = head_leaf = tail_leaf = node_allocator.allocate_data_node();
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
        data_node_ptr new_node = node_allocator.allocate_data_node();
        new_node->node_mutex->lock();
        ++this->m_stats.level_node[0];
        if (node->prev != nullptr){
            node->prev->node_mutex.lock();
        }
        split(new_node, node);
        if (pos < new_node->size)
            new_node->insert(key, value, pos);
        else
            node->insert(key, value, pos - new_node->size);
        key_type new_key = new_node->key[new_node->size - 1];
        //node_ptr _ = static_cast<node_ptr>(new_node);
        insert_recursive_con(stack, top, &new_key, &new_node, 1)
        new_node->node_mutex->unlock();
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

}

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert_split_helper_con(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_split_cnt;
    #endif 
    inner_node_ptr node = stack[top - 1], parent = (top > 1) ? (stack[top - 2]) : nullptr;
    if (node->)
    node->node_mutex.lock();
    if (parent != nullptr)
        parent->node_mutex.lock();
    if (node == root){
        insert_split_by_buffer_con(node, new_key, new_child, n);
        return;
    }

    // node != root
    if (!IS_ML_NODE(node)){
        if (node->slot_size * traits::EXPAND_RATIO < traits::MIN_ML_INNER_NODE_SIZE && n < node->slot_size / 2){
            insert_split_dense_inner_node_con(stack, top, new_key, new_child, n);
        }
        else{
            insert_split_by_buffer_con(stack, top, new_key, new_child, n);
        }
    }
    else if (node->size + n < node->slot_size){
        insert_split_pipeline_con(stack, top, new_key, new_child, n);
    }
    else{
        insert_split_by_buffer_con(stack, top, new_key, new_child, n);
    }
    for (int i = 0; i < n; ++i)
        new_child[i]->node_mutex.unlock();
    node->node_mutex.unlock();
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::insert_recursive_con(inner_node_ptr* stack, int top, const key_type* key_buf, node_ptr* child_buf, const slot_type n){
    if (node == nullptr){
        add_root_con(key_buf, child_buf, n);
        return true;
    }
    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    if (isfull(node, n - 1)) {
        if (check_split(node) == true){
            insert_split_helper_con(stack, top, key_buf, child_buf, n);
            return true;
        }
        if (expand(node) == false){
            insert_split_helper_con(stack, top, key_buf, child_buf, n);
            return true;
        }
    }
    for (slot_type i = 0; i < n; ++i){
        if (!node->insert(key_buf[i], child_buf[i])){
            insert_split_helper_con(stack, top, key_buf + i, child_buf + i, n - i);
            return true;
        }
    }
    return false;
}

}