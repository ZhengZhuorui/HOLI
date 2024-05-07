#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
bool aex_tree_con<_Key, _Val, traits>::erase(const key_type &key){
    //const_iterator find_iter = find_iterator(x);
    inner_node_ptr stack[traits::MAX_DEPTH];
    int top;
    data_node_ptr node = find_node_lock_con(x, stack, top);
    slot_type pos = node->find_lower_pos(x);
    if (pos >= node->size)
        return false;
    if (node->key[pos] != x){
        XU(node);
        return false; 
    }
    this->balance_stats.update_timestamp();
    if constexpr (traits::AllowDynamicDataNode){
        ((dynamic_node_ptr)node)->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    }

    AEX_ASSERT(node != empty_leaf);
    node->erase(pos);
    --this->m_stats.size;

    if constexpr(traits::AllowDynamicDataNode){
        if (isfew(node)){
            rescale(node, ((dynamic_node_ptr)node)->slot_size >> 1);
        }
    }

    /* if data node is few, means rescale failed.  merge the near leaf */
    if (isfew(node)){
        {
            std::shared_lock<std::shared_mutex> lk(this->tree_mutex);
            if (node == this->root){
                if (node->size == 0){
                    this->root = this->head_leaf = this->tail_leaf = nullptr;
                    this->m_stats = aex_stats();
                }
                XU(node);
                return true;
            }
        }
        int top;
        TXL(key, node->level + 1, stack, top);
        inner_node_ptr parent = stack[top - 2];
        
        if (parent != nullptr){
            
            if (node->size == 0){
                erase_link(node);
                erase_child_node(parent, node);
                if (isfew(parent))
                    erase_recursive(stack, top - 1);
                XU(parent);
                XU(node);
                return true;
            }
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev);

        if (left_node != nullptr){
            TXL(parent);
            if (parent->child_ptr[0] == node) left_node = nullptr;
            else XL(left_node);
            bool merge_flag = erase_merge(parent, left_node, node);
            if (merge_flag && parent != nullptr){
                if (isfew(parent))
                erase_recursive_con(key, stack, top - 1);
            }
            XU(parent);
            XU(node);
        }
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_recursive_con(cosnt key_type &key, inner_node_ptr* stack, int top){
    if (top == 0) 
        return;
        
    inner_node_ptr node = stack[top - 1];
    if (node == root && node->size == 1){
        std::shared_lock<std::shared_mutex> lk(this->tree_mutex);
        node_ptr tmp = root;
        root = static_cast<inner_node_ptr>(root)->child_ptr[0];
        --m_stats.level_node[this->m_stats.height - 1];
        --m_stats.height;
        allocator.free_node(tmp);
        return;
    }
    // Node isn't root. the node has parent, i.e. top >= 2

    // now node item is few. rescale it
    while (isfew(node)){
        if (!narrow(node)){
            break;
        }
    }

    // if node isn't few, finish.
    if (!isfew(node))
        return;

    TXL(key, , stack, top - 2);
    inner_node_ptr parent = stack[top - 2];
    // if rescale failed, the model can't fix the data or the item is fewer than min size
    // if items is large than min ml node size, train it again
    if (node->slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE){
        insert_split_pipeline_con(stack, top, nullptr, nullptr, 0);
    }
    // otherwise node->slot_size < MIN_INNER_NODE_SLOT_SIZE, means node->size < MIN_INNER_NODE_SIZE / 2
    else{

        if (node->size == 0){
            erase_link(node);
            erase_child_node(parent, node);
        }
        else{
            inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
            // if left node is in same subtree
            inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
            if (left_node != nullptr)
                if (parent->child_ptr[0] != node){
                    if (IS_ML_NODE(left_node) == false && left_node->size + node->size <= node->slot_size)
                        erase_merge(parent, left_node, node);
                }
        }
            
        if (parent != nullptr)
            if (isfew(parent))
                erase_recursive_con(stack, top - 1);
    }
}
}