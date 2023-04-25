#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_node(inner_node_ptr node, node_ptr* stack, int top){
    if (top < 1) 
        return;
    bool merge_flag = true;

    /* if data node is few, shift the data first, otherwise merge the near leaf */

    /* if the key is update */
    while (merge_flag && top > 1){
        inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
        inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
        if (left_node != nullptr && parent != nullptr)
            if (parent->child_ptr[0] != left_node) left_node = nullptr;

        inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
        if (right_node != nullptr && parent != nullptr)
            if (parent->child_ptr[parent->last()] != right_node) right_node = nullptr;
            
        merge_flag = false;

        /* if the node is too few */
        if (isfew(node)){
            if (rescale(node, parent, traits::NARROW_RATIO)){
            }
            else{
                if (left_node != nullptr){
                    // if left_node exists and have more item, shift it
                    bool narrow_flag = true;
                    while (narrow_flag && left_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(left_node))
                        narrow_flag = rescale(left_node, parent, traits::NARROW_RATIO);

                    if (!isfew(left_node)){
                        if (update_childnode_key(parent, left_node, node->node_max_key())){
                            shift_to_right_node(left_node, node);
                            break;
                        }
                    }
                }

                if (right_node != nullptr){
                    // if right_node exists and have more item, shift it
                    bool narrow_flag = true;
                    while (narrow_flag && right_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(right_node)) 
                        narrow_flag = rescale(right_node, parent, traits::NARROW_RATIO);
                    if (!isfew(right_node)){
                        if (update_childnode_key(parent, node, right_node->key_ptr[0])){
                            shift_to_left_node(right_node, node);
                            break;
                        }
                    }
                }
                
                merge_flag = true;
                if (left_node != nullptr){
                    merge_to_left_node(left_node, node);
                    erase_son_node(parent, node);
                }
                else if (right_node != nullptr){
                    merge_to_right_node(node, right_node);
                    erase_son_node(parent, node);
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
                --m_stats.inner_node;
                --m_stats.height;
                node_allocator.free_node(tmp);
            }
        }
        else if (root->size == 0){
            node_ptr tmp = root;
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.data_node;
            --m_stats.height;
            node_allocator.free_node(tmp);
        }
    }
}

/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_iterator(iterator &iter, node_ptr* stack, int top){
    update_tree_frequency();
    this->m_stats.timestamp++;
    update_node_frequency(iter._M_node);
    iter._M_node->base_stats.write_times++;

    if (top > 2){
        update_node_frequency(stack[top - 2]);
        stack[top - 2]->base_stats.write_times++;
    }
    
    data_node_ptr node = iter._M_node;
    //size_type offset = iter.offset;
    erase_data(iter);
    bool narrow_flag = true;
    inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
    while (isfew(node) && narrow_flag) narrow_flag = rescale(node, parent, traits::NARROW_RATIO);

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
            if (right_node->key[0] > parent->key_ptr[parent->last()]) right_node = nullptr;

        narrow_flag = true;
        while (left_node!=nullptr && narrow_flag) 
            narrow_flag = rescale(left_node, parent, traits::NARROW_RATIO);
        narrow_flag = true;
        while (right_node!=nullptr && isfew(right_node)) 
            narrow_flag = rescale(right_node, parent, traits::NARROW_RATIO);

        if (left_node != nullptr && !isfew(left_node)){
            update_childnode_key(parent, left_node, left_node->key[left_node->size - 2]);
            shift_to_right_leaf(left_node, node);
        }
        else if (right_node != nullptr && !isfew(right_node)){
            update_childnode_key(parent, node, right_node->key[0]);
            shift_to_left_leaf(node, right_node);
            
        }
        else{
            /* merge the left leaf to right leaf */
            if (left_node != nullptr){
                merge_to_left_leaf(left_node, node);
                erase_son_node(parent, node);
                if (isfew(parent))
                    erase_node(parent, stack, top - 1);
                --this->m_stats.data_node;
                
            }
            else if (right_node != nullptr){
                merge_to_right_leaf(node, right_node);
                erase_son_node(parent, node);
                if (isfew(parent))
                    erase_node(parent, stack, top - 1);
                --this->m_stats.data_node;
            }
        }
    }
}

/*  */
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_son_node(inner_node_ptr __restrict__ parent, node_ptr __restrict__ node){
    if (parent == nullptr){
        return false;
    }
    size_type pos = parent->at(node);
    if (pos == parent->slot_size)
        return false;
        
    key_type* key = parent->key_ptr;
    node_ptr* child = parent->child_ptr;
    node_allocator.free_node(node);
    --parent->size;
    if (parent->prop & node_property::ML_NODE){
        size_type prev_pos = parent->prev_item(pos);
        prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
        bitmap_impl::set_zero(parent->bitmap_ptr, pos);
        for (size_type i = prev_pos; i <= pos; ++i){
            key[i] = key[pos + 1];
            child[i] = child[pos + 1];
        }
        if (pos == parent->slot_bound)
            parent->slot_bound = prev_pos;
    }
    else{
        memmove(key + pos, key + pos + 1, (parent->size - pos - 1) * sizeof(key_type));
        memmove(child + pos, child + pos + 1, (parent->size - pos - 1) * sizeof(node_ptr));
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_data(iterator &iter){
    data_node_ptr node = iter._M_node;
    size_type offset = iter.offset;
    if (node->prop & node_property::ML_NODE){
        node->erase(offset);
    }

    memmove(node->key + offset, node->key + offset + 1, (node->size - offset - 1) * sizeof(key_type));
    data_memmove(node->data + iter.offset, node->data + iter.offset + 1, (node->size - iter.offset - 1) * sizeof(value_type));
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    if (node == nullptr) return;
    AEX_FORMAT("ERASE NODE");
    if (node->prop & node_property::LEAF){
        node_allocator.free_node(static_cast<data_node_ptr>(node));
        return;
    }
    else{
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        if (node->prop & node_property::ML_NODE){
            bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
            for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                this->erase_tree_recursive(child[i]);
            }
        }
        else{
            for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                this->erase_tree_recursive(child[i]);
            }
        }
        node_allocator.free_node(static_cast<inner_node_ptr>(node));
    }
}

//template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::erase_subtree(node_ptr __restrict__ parent, node_ptr __restrict__ node){
//    erase_son_node(parent, node);
//    erase_tree_recursive(node);
//}



}