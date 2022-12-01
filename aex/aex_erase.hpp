#pragma once
#include "aex/aex.h"

namespace aex{

/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_node(node_ptr _node){
    bool merge_flag = true;
    double cost;
    inner_node_ptr node = _node;
    
    /* if data node is few, shift the data first, otherwise merge the near leaf */

    /* if the key is update */
    while (merge_flag && node != root){
        inner_node_ptr parent = node->parent;
        inner_node_ptr left_node = (node->prev == nullptr) ? nullptr : (node->prev->parent == node->parent ? nullptr : static_cast<inner_node_ptr>(node->prev) );
        //inner_node_ptr right_node = ? nullptr : static_cast<inner_node_ptr>(right[top - 1]);
        inner_node_ptr right_node = (node->next == nullptr) ? nullptr : (node->next->parent == node->parent ? nullptr : static_cast<inner_node_ptr>(node->next) );
        top--;
        merge_flag = false;

        /* if the node is too few */
        if (isfew(node)){
            if (narrow(node)){
                //update_key_flag = narrow(update_node) ? 0 : update_key_flag;
            }
            else{
                if (left_node != nullptr){
                    // if left_node exists and have more item, shift it
                    while (left_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(left_node)) 
                        narrow(left_node);

                    if (!is_few(left_node)){
                        shift_to_right_node(left_node, node);
                        update_childnode_key(parent, left_node, left_node->key_ptr[left_node->last()]);
                        break;
                    }
                }

                if (right_node != nullptr){
                    // if right_node exists and have more item, shift it
                    while (right_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && isfew(right_node)) 
                        narrow(right_node);
                    if (!is_few(right_node)){
                        shift_to_left_node(right_node, node);
                        break;
                    }
                }
                
                merge_flag = true;
                if (left_node != nullptr){
                    merge_to_right_node(left_node, node);
                    erase_son_node(parent, left_node);
                }
                else if (right_node != nullptr){
                    merge_to_left_node(node, right_node);
                    erase_son_node(parent, node);
                }
            }
        }
        node = node->parent;
    }

    if (merge_flag){
        /* if the root has one child, change the root*/
        if (!(root->prop & LEAF)){
            if (root->size == 1){
                node_ptr tmp = root;
                root = root->child_ptr[0];
                --m_stats.inner_node;
                --m_stats.height;
                node_allocator::free(tmp);
            }
        }
        else if (root->size == 0){
            node_ptr tmp = root;
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.data_node;
            --m_stats.height;
            node_allocator::free(tmp);
        }
    }
}

/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_iterator(iterator &iter){
    bool merge_flag = false;
    double cost;
    //_erase(node, key);
    data_node_ptr node = iter._M_node;

    /* if data node is few, shift the data first, otherwise merge the near leaf */
    if (isfew(node)){
        inner_node_ptr parent = node->parent;
        data_node_ptr left_node = (parent->at(node->prev) == parent->slot_size) ? nullptr: node->prev;
        data_node_ptr right_node = (parent->at(node->next) == parent->slot_size) ? nullptr: node->next;

        if (left_node != nullptr && !is_few(left_node)){
            shift_to_right_leaf(left_node, node);
            update_childnode_key(parent, left_node, left_node->key[left_node->size - 1]);
        }
        else if (right_node != nullptr && !isfew(right_node)){
            right_node->read_write_diff -= right_node->slot_size;
            if (cost + right_node->read_write_diff < 0){
                split(right_node);
            }
            shift_to_left_leaf(node, right_node);
        }
        else{
            /* merge the left leaf to right leaf */
            merge_flag = true;
            if (left_node != nullptr){
                merge_to_left_leaf(left_node, node, parent);
                erase_son_node(parent, left_node);
                node_allocator::free(left_node);
                --this->m_stats.data_node;
            }
            else if (right_node != nullptr){
                merge_to_right_leaf(node, right_node, parent);
                erase_son_node(parent, node);
                node_allocator::free(node);
                --this->m_stats.data_node;
            }
        }
    }
    node = node->parent;

    /* if the key is update */
    while (node != nullptr && merge_flag){
        inner_node_ptr parent = node->parent;
        inner_node_ptr left_node = node->prev;
        if (left_node != nullptr && left_node->parent != parent) left_node = nullptr;
        inner_node_ptr right_node = node->next;
        if (right_node != nullptr && right_node->parent != parent) right_node = nullptr;
        merge_flag = false;

        /* if the node is too few */
        if (is_few(node)){
            if (narrow(node)){
                //update_key_flag = narrow(update_node) ? 0 : update_key_flag;
            }
            else{
                if (left_node != nullptr){
                    // if left_node exists and have more item, shift it
                    while (left_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && is_few(left_node)) 
                        narrow(left_node);
                    if (!is_few(left_node)){
                        shift_to_right_node(left_node, node);
                        update_childnode_key(parent, left_node, left_node->key_ptr[left_node->last()]);
                        break;
                    }
                }

                if (right_node != nullptr){
                    // if right_node exists and have more item, shift it
                    while (right_node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE && is_few(right_node)) 
                        narrow(right_node);
                    if (!is_few(right_node)){
                        shift_to_left_node(right_node, node);
                        break;
                    }
                }
                
                merge_flag = true;
                if (left_node != nullptr){
                    merge_to_right_node(left_node, node);
                    erase_son_node(parent, left_node);
                }
                else if (right[top] != nullptr){
                    merge_to_right_node(node, right[top]);
                    erase_son_node(parent, node);
                }
            }
        }
        node = parent;
    }

    if (merge_flag){
        /* if the root has one child, change the root*/
        if (!(root->prop & LEAF)){
            if (root->size == 1){
                node_ptr tmp = root;
                root = root->child_ptr[0];
                --m_stats.inner_node;
                --m_stats.height;
                node_allocator::free(tmp);
            }
        }
        else if (root->size == 0){
            node_ptr tmp = root;
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.data_node;
            --m_stats.height;
            node_allocator::free(tmp);
        }
    }
    //return res;
}

/*  */
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_son_node(node_ptr parent, inner_node_ptr node){
    if (parent == nullptr){
        return false;
    }
    size_type pos = parent->at(node);
    if (pos == parent->slot_size)
        return false;
    key_type* key = parent->key_ptr;
    node_ptr* child = parent->child_ptr;
    node_allocator::free(node);
    --parent->size;
    if (parent->prop & ML_NODE){
        size_type prev_pos = parent->next(pos);
        prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
        bitmap_impl::set_zero(parent->bitmap_ptr, pos);
        for (size_type i = prev_pos; i <= pos; ++i){
            key[i] = key[pos + 1];
            child[i] = child[pos + 1];
        }
    }
    else{
        memmove(key + pos, key + pos + 1, (parent->size - pos - 1) * sizeof(key_type));
        memmove(child + pos, child + pos + 1, (parent->size - pos - 1) * sizeof(node_ptr));
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_son_data(data_node_ptr node, iterator &iter){
    memmove(node->key + iter.offset, node->key + iter.offset + 1, (node->size - iter.offset - 1) * sizeof(key_type));
    data_memmove(node->child + iter.offset, node->child + iter.offset + 1, (node->size - iter.offset - 1) * sizeof(value_type));
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    if (node == nullptr) return;
    AEX_PRINT("ERASE NODE");
    if (node->prop & LEAF){
        node_allocator::free(static_cast<data_node_ptr>(node));
        return;
    }
    else{
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        if (node->prop & ML_NODE){
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
        node_allocator::free(static_cast<inner_node_ptr>(node));
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_subtree(node_ptr node){
    erase_son_node(node->parent, node);
    erase_tree_recursive(node);
}



}