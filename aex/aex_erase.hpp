#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_ascend(inner_node_ptr node){
    AEX_ASSERT(node == nullptr);
    if (top < 1) 
        return;
    bool merge_flag = true;

    /* if data node is few, shift the data first, otherwise merge the near leaf */

    /* if the key is update */
    while (merge_flag && node != nullptr){
        if (!isfew(node)) 
            break;
        inner_node_ptr parent = node->parent;
        if (parent == nullptr)
            break;
        merge_flag = false;
        if (node->slot_size > traits::MIN_ML_INNER_NODE_SLOT_SIZE && (node->prop & node_property::ML_NODE)){
            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            split(node, key_buf, child_buf);
            insert_ascend(node->parent, key_buf, child_buf);
            return;
        }
        else{
            merge_flag = true;
            inner_node_ptr parent = node->parent;          
            inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
            if (left_node != nullptr)
                if (left_node->parent == node) left_node = nullptr;

            inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
            if (right_node != nullptr)
                if (right_node->right_node == node) right_node = nullptr;

            bool new_node_flag;
            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            if (left_node != nullptr && right_node != nullptr){
                if (left_node->slot_size < right_node->slot_size){
                    merge_flag = erase_split(left_node, node);
                }
                else{
                    merge_flag = erase_split(node, right_node);
                }
            }
            else if (left_node != nullptr){
                merge_flag = erase_split(left_node, node);
            }
            else{
                merge_flag = erase_split(node, right_node);
            }
        }
        node = parent;
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

// return merge flag
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_split(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    std::vector<key_buf> key_buf(left_node->size + right_node->size), new_key;
    std::vector<node_ptr> child_buf(left_node->size + right_node->size), new_child;
    copy_to_buffer(left_node, key_buf.data(), child_buf.data());
    copy_to_buffer(right_node, key_buf.data() + left_node->size, child_buf.data() + left_node->size);
    split(key_buf, child_buf, right_node->level, new_key, new_child);
    erase_link(left_node);
    link_node_list(node, new_child);
    erase_child_node(parent, left_node);
    if (new_key.size() > 0){
        insert_ascend(parent, new_key, new_child);
        return false;
    }
    else{
        return true;
    }
}

// return merge flag
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_split(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    std::vector<key_buf> key_buf(left_node->size + right_node->size), new_key;
    std::vector<value_type> data_buf(left_node->size + right_node->size);
    std::vector<node_ptr> new_child;
    std::copy(left_node->key, left_node->key + left_node->size, key_buf.data());
    std::copy(left_node->data, left_node->data + left_node->size, data_buf.data());
    std::copy(right_node->key, right_node->key + right_node->size, key_buf.data() + left_node->size);
    std::copy(right_node->data, right_node->data + right_node->size, data_buf.data() + left_node->size);
    split_with_linear_probe(key_buf, data_buf, right_node->level, new_key, new_child);
    link_node_list(right_node, new_child);
    erase_link(left_node);
    erase_child_node(parent, left_node);
    if (new_key.size() > 0){
        insert_ascend(parent, new_key, new_child);
        return false;
    }
    else{
        return true;
    }
}


/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_iterator(iterator &iter){
    update_tree_frequency();
    this->m_stats.timestamp++;

    data_node_ptr node = iter._M_node;

    for (node_ptr inode = node; inode != nullptr; inode = inode->parent){
        update_node_frequency(inode);
        inode->base_stats.write_times += 1;
        inode->base_stats.update_times++;
    }

    erase_data(iter);
    --this->m_stats.size;

    if (isfew(node))
        rescale(node, traits::NARROW_RATIO);

    /* if data node is few, shift the data first, otherwise merge the near leaf */
    if (isfew(node)){
        if (parent == nullptr){
            AEX_ASSERT(this->root == node);
            if (node->size == 0){
                this->root = this->head_leaf = this->tail_leaf = nullptr;
                this->m_stats = aex_stats();
            }
            return;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr && parent != nullptr)
            if (node->parent != left_node->parent) left_node = nullptr;

        if (right_node != nullptr)
            if (node->parent == right_node->parent) right_node = nullptr;

        if (left_node != nullptr){
            AEX_ASSERT(isfew(left_node) == true);
        }
        if (right_node != nullptr){
            AEX_ASSERT(isfew(right_node) == true);
        }

        bool merge_flag = true;
        if (left_node != nullptr && !isfew(left_node, -1) && update_childnode_key(parent, left_node, left_node->key[left_node->size - 1])){
            shift_to_right_leaf(left_node, node);
        }
        else if (right_node != nullptr && !isfew(right_node, -1) && update_childnode_key(parent, node, right_node->key[0])){
            shift_to_left_leaf(node, right_node);
        }
        else{
            bool merge_flag;
            if (left_node != nullptr && right_node != nullptr){
                if (left_node->size < right_node->size){
                    merge_flag = erase_split(left_node, node);
                }
                else{
                    merge_flag = erase_split(node, right_node);
                }
            }
            else if (left_node != nullptr){
                merge_flag = erase_split(left_node, node);
            }
            else {
                merge_flag = erase_split(node, right_node);
            }
            
            if (merge_flag && node->parent != nullptr){
                if (isfew(node->parent))
                    erase_decend(node->parent);
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
    AEX_ASSERT(flag == false);
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
            for (pos_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                this->erase_tree_recursive(child[i]);
            }
        }
        else{
            for (pos_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                this->erase_tree_recursive(child[i]);
            }
        }
        node_allocator.free_node(static_cast<inner_node_ptr>(node));
        --this->m_stats.inner_node;
    }
}

}