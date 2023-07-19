#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_ascend(inner_node_ptr node, node_ptr* stack, int top){
    if (top < 1) 
        return;
    bool merge_flag = true;

    /* if data node is few, shift the data first, otherwise merge the near leaf */

    /* if the key is update */
    while (merge_flag && top > 1){
        inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
        inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
        if (parent != nullptr)
            if (parent->child_ptr[0] == node) left_node = nullptr;

        inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
        if (parent != nullptr)
            if (parent->child_ptr[parent->slot_size - 1] == node) right_node = nullptr;
            
        merge_flag = false;

        /* if the node is too few */
        if (isfew(node)){
            if (rescale(node, traits::NARROW_RATIO)){
            }
            else{
                if (left_node != nullptr){
                    // if left_node exists and have more item, shift it
                    bool narrow_flag = true;
                    while (narrow_flag && left_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(left_node))
                        narrow_flag = rescale(left_node, traits::NARROW_RATIO);

                    if (!isfew(left_node)){
                        if (update_childnode_key(parent, left_node, left_node->key_ptr[left_node->last()])){
                            shift_to_right_node(left_node, node);
                            break;
                        }
                    }
                }

                if (right_node != nullptr){
                    // if right_node exists and have more item, shift it
                    bool narrow_flag = true;
                    while (narrow_flag && right_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(right_node)) 
                        narrow_flag = rescale(right_node, traits::NARROW_RATIO);
                    if (!isfew(right_node)){
                        if (update_childnode_key(parent, node, right_node->key_ptr[0])){
                            shift_to_left_node(right_node, node);
                            break;
                        }
                    }
                }
                
                merge_flag = true;
                if (left_node != nullptr){
                    //merge_to_left_node(left_node, node);
                    merge_to_right_node(left_node, node);
                    erase_child_node(parent, left_node);
                }
                else if (right_node != nullptr){
                    merge_to_right_node(node, right_node);
                    erase_child_node(parent, node);
                }
            }
        }
        --top;
        node = static_cast<inner_node_ptr>(stack[top - 1]);
    }

    if (merge_flag){
        /* if the root has one child, change the root*/
        if (!(root->prop & node_property::LEAF)){
            if (root->size == 1){
                node_ptr tmp = root;
                root = static_cast<inner_node_ptr>(root)->child_ptr[0];
                --m_stats.height;
                node_allocator.free_node(tmp);
                --m_stats.inner_node;
            }
        }
        else if (root->size == 0){
            node_ptr tmp = root;
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.height;
            node_allocator.free_node(tmp);
            --m_stats.data_node;
        }
    }
}

/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_iterator(iterator &iter, node_ptr* stack, int top){
    update_tree_frequency();
    this->m_stats.timestamp++;

    for (int i = 1; i < top; ++i){
        update_node_frequency(stack[i]);
        stack[i]->base_stats.write_times++;
    }
    
    data_node_ptr node = iter._M_node;
    erase_data(iter);
    --this->m_stats.size;

    bool narrow_flag = true;
    inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
    while (narrow_flag && isfew(node)) 
        narrow_flag = rescale(node, traits::NARROW_RATIO);

    /* if data node is few, shift the data first, otherwise merge the near leaf */
    if (isfew(node)){
        if (parent == nullptr){
            AEX_ASSERT(this->root == node);
            if (node->size == 0){
                this->root = this->head_leaf = this->tail_leaf = nullptr;
                this->m_stats = aex_stats();
                return;
            }
        }
        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr && parent != nullptr)
            if (parent->child_ptr[0] != left_node) left_node = nullptr;

        if (right_node != nullptr)
            if (parent->child_ptr[parent->slot_size - 1] == node) right_node = nullptr;

        narrow_flag = true;
        while (narrow_flag && left_node!=nullptr && isfew(left_node)) 
            narrow_flag = rescale(left_node, traits::NARROW_RATIO);
        narrow_flag = true;
        while (narrow_flag && right_node!=nullptr && isfew(right_node)) 
            narrow_flag = rescale(right_node, traits::NARROW_RATIO);

        if (left_node != nullptr && !isfew(left_node) && update_childnode_key(parent, left_node, left_node->key[left_node->size - 1])){
            shift_to_right_leaf(left_node, node);
        }
        else if (right_node != nullptr && !isfew(right_node) && update_childnode_key(parent, node, right_node->key[0])){
            shift_to_left_leaf(node, right_node);
        }
        else{
            /* merge the left leaf to right leaf */
            if (left_node != nullptr){
                merge_to_right_leaf(left_node, node);
                erase_child_node(parent, left_node);
                --this->m_stats.data_node;
                if (parent != nullptr && isfew(parent))
                    erase_node(parent, stack, top - 1);

            }
            else if (right_node != nullptr){
                merge_to_right_leaf(node, right_node);
                update_childnode_ptr(parent, node, right_node);
                erase_child_node(parent, node);
                --this->m_stats.data_node;
                if (parent != nullptr && isfew(parent))
                    erase_node(parent, stack, top - 1);
            }
            else{
                AEX_ASSERT(this->root == node);
                if (this->m_stats.size == 0){
                    node_allocator.free_node(node);
                    this->root = this->head_leaf = this->tail_leaf = nullptr;
                }
            }
        }
    }
}

/*  */
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_child_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node){
    if (parent == nullptr)
        return false;
        
    bool flag = parent->erase(node);
    if (flag){
        if (node->prop & node_property::LEAF) --this->m_stats.data_node;
        else --this->m_stats.inner_node;
        node_allocator.free_node(node);
    }
    return flag;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_data(iterator &iter){
    data_node_ptr node = iter._M_node;
    size_type offset = iter.offset;
    std::move(node->key + offset + 1, node->key + offset + node->size, node->key + offset);
    std::move(node->data + offset + 1, node->data + offset + node->size, node->data + offset);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    if (node == nullptr) return;
    if (node->prop & node_property::LEAF){
        node_allocator.free_node(static_cast<data_node_ptr>(node));
        --this->m_stats.data_node;
        return;
    }
    else{
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        if (node->prop & node_property::ML_NODE){
            bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
            for (slot_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                this->erase_tree_recursive(child[i]);
            }
        }
        else{
            for (slot_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                this->erase_tree_recursive(child[i]);
            }
        }
        node_allocator.free_node(static_cast<inner_node_ptr>(node));
        --this->m_stats.inner_node;
    }
}

}