#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_recursive(inner_node_ptr* stack){
    if (*stack == nullptr) 
        return;
        
    inner_node_ptr node = *stack;
    if (node == root && node->size == 1){
        node_ptr tmp = root;
        root = static_cast<inner_node_ptr>(root)->child_ptr[0];
        --m_stats.level_node[this->m_stats.height - 1];
        --m_stats.height;
        node_allocator.free_node(tmp);
        return;
    }
    // Node isn't root. the node has parent

    // now node item is few. rescale it
    while (isfew(node)){
        if (!narrow(node)){
            break;
        }
    }

    // if node isn't few, finish.
    if (!isfew(node))
        return;

    inner_node_ptr parent = *(stack - 1);
    // if rescale failed, the model can't fix the data or the item is fewer than min size
    // if items is large than min ml node size, train it again
    if (node->slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE){
        insert_split_pipeline(stack, nullptr, nullptr, 0);
    }
    // otherwise node->slot_size < MIN_INNER_NODE_SLOT_SIZE, means node->size < MIN_INNER_NODE_SIZE / 2
    else{
        if (node->size == 0){
            erase_link(node);
            erase_child_node(parent, node);
        }
        else{
            inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
            // if right node is in same subtree
            if (right_node != nullptr)
                if (parent->child_ptr[parent->slot_size - 1] != node){
                    // if node can be merge to right node, merge it.
                    if (IS_ML_NODE(right_node) == false && node->size + right_node->size <= right_node->slot_size)
                        erase_merge(parent, node, right_node);
                }
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
                erase_recursive(stack - 1);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_merge(inner_node_ptr __restrict__ parent, inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_WARNING("erase merge inner node");
    AEX_ASSERT(left_node->size + right_node->size <= right_node->slot_size);
    AEX_ASSERT(IS_ML_NODE(left_node) == false);
    AEX_ASSERT(IS_ML_NODE(right_node) == false);
    AEX_ASSERT(parent != nullptr);
    key_type split_key =  parent->key_ptr[parent->at(left_node)];
    std::move_backward(right_node->key_ptr, right_node->key_ptr + right_node->size - 1, right_node->key_ptr + left_node->size + right_node->size);
    std::move_backward(right_node->child_ptr, right_node->child_ptr + right_node->size, right_node->child_ptr + left_node->size + right_node->size);
    std::move(left_node->key_ptr, left_node->key_ptr + left_node->size - 1, right_node->key_ptr);
    std::move(left_node->child_ptr, left_node->child_ptr + left_node->size, right_node->child_ptr);
    right_node->key_ptr[left_node->size] = split_key;
    erase_link(left_node);
    erase_child_node(parent, left_node);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_merge(inner_node_ptr __restrict__ parent, data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    if constexpr (!traits::AllowDynamicDataNode){
        AEX_ASSERT(left_node != right_node);
        if (left_node->size + right_node->size <= traits::MIN_DATA_NODE_SLOT_SIZE){
            //AEX_WARNING("!! erase merge data node, " << left_node->size << ", " << right_node->size);
            std::move_backward(right_node->key, right_node->key + right_node->size, right_node->key + right_node->size + left_node->size);
            std::move_backward(right_node->data, right_node->data + right_node->size, right_node->data + right_node->size + left_node->size);
            std::move(left_node->key, left_node->key + left_node->size, right_node->key);
            std::move(left_node->data, left_node->data + left_node->size, right_node->data);
            right_node->size += left_node->size;
            erase_link(left_node);
            erase_child_node(parent, left_node);
            return true;
        }
        else return false;
    }
    else{
        std::vector<key_type> key_buf(left_node->size + right_node->size), new_key;
        std::vector<value_type> data_buf(left_node->size + right_node->size);
        std::vector<node_ptr> new_child;
        std::copy(left_node->key, left_node->key + left_node->size, key_buf.data());
        std::copy(left_node->data, left_node->data + left_node->size, data_buf.data());
        std::copy(right_node->key, right_node->key + right_node->size, key_buf.data() + left_node->size);
        std::copy(right_node->data, right_node->data + right_node->size, data_buf.data() + left_node->size);
        //split_with_linear_probe(key_buf.data(), data_buf.data(), right_node->level, new_key, new_child);
        split_with_exponential_probe(key_buf.data(), data_buf.data(), right_node->level, new_key, new_child);
        right_node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
        update_node_list_frequency(right_node, new_child.data(), new_child.size());
        link_node_list_and_replace_last_node(right_node, new_child.data(), new_child.size());
        new_key.pop_back();
        new_child.pop_back();
        erase_link(left_node);
        erase_child_node(parent, left_node);
        if (new_key.size() > 0){
            insert_recursive(parent, new_key.size(), new_child.data(), new_child.size());
            return false;
        }
        else{
            return true;
        }
    }
}

/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_iterator(const_iterator &iter){
    
    this->balance_stats.update_timestamp();
    inner_node_ptr* stack;
    data_node_ptr node = find_leaf_with_stack(iter.key, stack);
    if constexpr (traits::AllowDynamicDataNode){
        ((dynamic_node_ptr)node)->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    }

    AEX_ASSERT(node != empty_leaf);
    node->erase(iter.offset);
    --this->m_stats.size;

    if constexpr(traits::AllowDynamicDataNode){
        if (isfew(node)){
            rescale(node, ((data_node_ptr)node)->slot_size >> 1);
        }
    }

    /* if data node is few, means rescale failed.  merge the near leaf */
    if (isfew(node)){
        if (node == this->root){
            if (node->size == 0){
                this->root = this->head_leaf = this->tail_leaf = nullptr;
                this->m_stats = aex_stats();
            }
            return;
        }
        inner_node_ptr parent = node->parent;
        if (node->size == 0){
            erase_link(node);
            erase_child_node(parent, node);
            if (isfew(parent))
                erase_recursive(parent);
            return;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr)
            if (parent->child_ptr[0] == node) left_node = nullptr;

        if (right_node != nullptr)
            if (parent->child_ptr[parent->last()] == node) right_node = nullptr;

        if (left_node == nullptr && right_node == nullptr){
            AEX_PRINT(node << ", size=" << node->size << ", prev=" << node->prev << ", next=" << node->next << ", parent=" << parent << ", " << tail_leaf << ", " << parent->size);
            AEX_PRINT("empty_leaf=" << empty_leaf);
            AEX_PRINT("pos=" << parent->at(node));
            if (node->prev != nullptr){
                AEX_PRINT("left_node parent=" << node->prev->parent);
                AEX_PRINT("pos=" << node->prev->parent->at(node->prev));
            }
            if (node->next != nullptr){
                AEX_PRINT("right_node parent=" << node->next->parent);
                AEX_PRINT("pos=" << node->next->parent->at(node->next));
            }
        }

        //if (traits::AllowDynamicDataNode && left_node != nullptr)
        //    while (isfew(left_node)) 
        //        rescale(left_node, ((dynamic_node_ptr)node)->slot_size >> 1);
        //    
        //if (traits::AllowDynamicDataNode && right_node != nullptr)
        //    while (isfew(right_node)) 
        //        rescale(right_node, ((dynamic_node_ptr)node)->slot_size >> 1);

        bool merge_flag = true;
        if (left_node != nullptr && right_node != nullptr){
            if (left_node->size < right_node->size)
                merge_flag = erase_merge(parent, left_node, node);
            else
                merge_flag = erase_merge(parent, node, right_node);
        }
        else if (left_node != nullptr)
            merge_flag = erase_merge(parent, left_node, node);
        else
            merge_flag = erase_merge(parent, node, right_node);
        
        if (merge_flag && parent != nullptr){
            if (isfew(parent))
                erase_recursive(stack - 1);
        }
    }
}

/*  */
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node){
    //if (parent != nullptr)
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(node != this->empty_leaf);
    parent->erase(node);

    if (IS_LEAF_NODE(node))
        --this->m_stats.level_node[0];
    else
        --this->m_stats.level_node[static_cast<inner_node_ptr>(node)->level];
    node_allocator.free_node(node);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    if (node == nullptr) return;
    if (IS_LEAF_NODE(node)){
        --this->m_stats.level_node[0];
        node_allocator.free_node(static_cast<data_node_ptr>(node));
        return;
    }
    else{
        inner_node_ptr _node = static_cast<inner_node_ptr>(node);
        node_ptr* child = _node->child_ptr;
        if (IS_ML_NODE(node)){
            bitmap bm = _node->bitmap_ptr;
            slot_type slot_size = _node->slot_size;
            for (slot_type i = 0; i < slot_size; ++i)
                if (bitmap_impl::at(bm, i))
                    this->erase_tree_recursive(child[i]);
            this->erase_tree_recursive(child[slot_size - 1]);
        }
        else{
            for (slot_type i = 0; i < node->size; ++i)
                this->erase_tree_recursive(child[i]);
        }
        --this->m_stats.level_node[_node->level];
        node_allocator.free_node(_node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_one(const key_type &x){
    //const_iterator find_iter = find_iterator(x);
    inner_node_ptr *stack;
    data_node_ptr node = find_leaf_with_stack(x, stack);
    slot_type pos = node->find_lower_pos(x);
    if (pos >= node->size)
        return false;
    if (node->key[pos] != x)
        return false; 
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
        if (node == this->root){
            if (node->size == 0){
                this->root = this->head_leaf = this->tail_leaf = nullptr;
                this->m_stats = aex_stats();
            }
            return true;
        }
        inner_node_ptr parent = *stack;
        if (node->size == 0){
            erase_link(node);
            erase_child_node(parent, node);
            if (isfew(parent))
                erase_recursive(stack);
            return true;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr)
            if (parent->child_ptr[0] == node) left_node = nullptr;

        if (right_node != nullptr)
            if (parent->child_ptr[parent->last()]) right_node = nullptr;

        if (left_node == nullptr && right_node == nullptr){
            AEX_ERROR("ERROR!");
        }

        bool merge_flag = true;
        if (left_node != nullptr && right_node != nullptr){
            if (left_node->size < right_node->size)
                merge_flag = erase_merge(parent, left_node, node);
            else
                merge_flag = erase_merge(parent, node, right_node);
        }
        else if (left_node != nullptr)
            merge_flag = erase_merge(parent, left_node, node);
        else
            merge_flag = erase_merge(parent, node, right_node);
        
        if (merge_flag && parent != nullptr){
            if (isfew(parent))
                erase_recursive(stack);
        }
    }
    return true;
}

}