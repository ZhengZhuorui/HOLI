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
    std::false_type fp;
    if (node == root){
        if (node->size == 1){
            node_ptr tmp = root;
            root = static_cast<inner_node_ptr>(root)->child_ptr[0];
            --m_stats.level_node[this->m_stats.height - 1];
            --m_stats.height;
            allocator.free_node(tmp);
            return;
        }
        return;
    }

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
        insert_split_helper(stack, nullptr, nullptr, 0);
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
                        merge(parent, node, right_node, fp);
                }
            // if left node is in same subtree
            inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
            if (left_node != nullptr)
                if (parent->child_ptr[0] != node){
                    if (IS_ML_NODE(left_node) == false && left_node->size + node->size <= node->slot_size)
                        merge(parent, left_node, node, fp);
                }
        }
            
        if (parent != nullptr)
            if (isfew(parent))
                erase_recursive(stack - 1);
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
    this->_erase(stack, node);
}

/*  */
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node){
    //if (parent != nullptr)
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(node != this->empty_leaf);
    SET_FLAG(node, IS_DELETE);
    parent->erase(node);
    --this->m_stats.level_node[node->level];
    allocator.free_node(node);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node, const slot_type node_pos){
    //if (parent != nullptr)
    AEX_ASSERT(parent != nullptr);
    AEX_ASSERT(node != this->empty_leaf);
    SET_FLAG(node, IS_DELETE);
    parent->erase(node_pos);
    --this->m_stats.level_node[node->level];
    allocator.free_node(node);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    if (node == nullptr) return;
    if (IS_LEAF_NODE(node)){
        --this->m_stats.level_node[0];
        this->allocator.free_node(static_cast<data_node_ptr>(node));
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
        allocator.free_node(_node);
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
    this->_erase(stack, node);
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::_erase(inner_node_ptr* stack, data_node_ptr node){
    
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
            return;
        }
        inner_node_ptr parent = *stack;
        if (node->size == 0){
            erase_link(node);
            erase_child_node(parent, node);
            if (isfew(parent))
                erase_recursive(stack);
            return;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);
        left_node = (parent->child_ptr[0] == node) ? nullptr : left_node;
        right_node = (parent->child_ptr[parent->last()] == node) ? nullptr : right_node;

        if (left_node == nullptr && right_node == nullptr){
            AEX_PRINT("node=" << node << ", left_node=" << left_node << ", right_node=" << right_node << ", pf=" << parent->child_ptr[0] << ", pl=" << parent->child_ptr[parent->last()] << ", size=" << parent->size);
            AEX_ASSERT(parent->size == 1);
            erase_recursive(stack);
        }
        else{
            bool merge_flag = true;
            if (left_node != nullptr && right_node != nullptr){
                if (left_node->size < right_node->size)
                    merge_flag = merge(parent, left_node, node);
                else
                    merge_flag = merge(parent, node, right_node);
            }
            else if (left_node != nullptr)
                merge_flag = merge(parent, left_node, node);
            else
                merge_flag = merge(parent, node, right_node);

            if (merge_flag && parent != nullptr){
                if (isfew(parent))
                    erase_recursive(stack);
            }
        }
    }
    return;
}

}