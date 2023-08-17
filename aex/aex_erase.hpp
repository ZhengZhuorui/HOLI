#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_ascend(inner_node_ptr node){
    if (node == nullptr) 
        return;
    bool merge_flag = true; 

    /* if the key is update */
    while (merge_flag == true && static_cast<node_ptr>(node) != this->root){
        merge_flag = false;
        if (!isfew(node)) 
            break;

        // now node item is few. rescale it
        if (rescale(node, node->real_slot_size() >> 1))
            break;

        // if rescale failed, the model can't fix the data or the item is fewer then min size
        // if items is large than min size, train it again
        if (node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE){
            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            split(node, key_buf, child_buf);
            if (key_buf.size() > 1)
                insert_ascend(node->parent, key_buf, child_buf);
            return;
        }
        // otherwise merge to near node
        else{
            merge_flag = true;
            inner_node_ptr parent = node->parent;          
            inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
            if (left_node != nullptr)
                if (left_node->parent != parent) left_node = nullptr;

            inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
            if (right_node != nullptr)
                if (right_node->parent != parent) right_node = nullptr;

            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            if (left_node != nullptr && right_node != nullptr){
                if (left_node->slot_size < right_node->slot_size)
                    merge_flag = erase_split(left_node, node);
                else
                    merge_flag = erase_split(node, right_node);
            }
            else if (left_node != nullptr)
                merge_flag = erase_split(left_node, node);
            else
                merge_flag = erase_split(node, right_node);
        }
        node = node->parent;
    }

    if (merge_flag){
        /* if the root has one child, change the root*/
        if (!IS_LEAF_NODE(node)){
            if (root->size == 1){
                node_ptr tmp = root;
                root = static_cast<inner_node_ptr>(root)->child_ptr[0];
                --m_stats.level_node[this->m_stats.height - 1];
                --m_stats.inner_node;
                --m_stats.height;
                node_allocator.free_node(tmp);
            }
        }
        else if (node->size == 0){
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.level_node[this->m_stats.height - 1];
            --m_stats.data_node;
            --m_stats.height;
            node_allocator.free_node(node);
        }
    }
}

// return merge flag
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_split(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    std::vector<key_type> key_buf(left_node->size + right_node->size), new_key;
    std::vector<node_ptr> child_buf(left_node->size + right_node->size), new_child;
    inner_node_ptr parent = right_node->parent;
    copy_to_buffer(left_node, key_buf.data(), child_buf.data());
    copy_to_buffer(right_node, key_buf.data() + left_node->size, child_buf.data() + left_node->size);
    split(key_buf.data(), child_buf.data(), left_node->size + right_node->size, right_node->level, new_key, new_child);
    erase_link(left_node);
    right_node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(right_node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(right_node, new_child);
    new_key.pop_back();
    new_child.pop_back();
    if (parent != nullptr){
        parent->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
        erase_child_node(parent, left_node);
    }
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
    std::vector<key_type> key_buf(left_node->size + right_node->size), new_key;
    std::vector<value_type> data_buf(left_node->size + right_node->size);
    std::vector<node_ptr> new_child;
    inner_node_ptr parent = right_node->parent;

    std::copy(left_node->key, left_node->key + left_node->size, key_buf.data());
    std::copy(left_node->data, left_node->data + left_node->size, data_buf.data());
    std::copy(right_node->key, right_node->key + right_node->size, key_buf.data() + left_node->size);
    std::copy(right_node->data, right_node->data + right_node->size, data_buf.data() + left_node->size);
    split_with_linear_probe(key_buf.data(), data_buf.data(), right_node->level, new_key, new_child);
    right_node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(right_node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(right_node, new_child);
    new_key.pop_back();
    new_child.pop_back();
    erase_link(left_node);
    parent->erase(left_node);
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

    this->balance_stats.update_timestamp();

    data_node_ptr node = iter._M_node;

    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    node->erase(iter.offset);
    --this->m_stats.size;

    if (isfew(node))
        rescale(node, node->slot_size >> 1);

    /* if data node is few, means rescale failed.  merge the near leaf */
    if (isfew(node)){
        inner_node_ptr parent = node->parent;
        if (parent == nullptr){
            AEX_ASSERT(this->root == node);
            if (node->size == 0){
                this->root = this->head_leaf = this->tail_leaf = nullptr;
                this->m_stats = aex_stats();
            }
            return;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr)
            if (left_node->parent != parent) left_node = nullptr;

        if (right_node != nullptr)
            if (right_node->parent != parent) right_node = nullptr;

        if (left_node != nullptr)
            AEX_ASSERT(isfew(left_node) == true);
            
        if (right_node != nullptr)
            AEX_ASSERT(isfew(right_node) == true);

        bool merge_flag = true;

        if (left_node != nullptr && right_node != nullptr){
            if (left_node->size < right_node->size)
                merge_flag = erase_split(left_node, node);
            else
                merge_flag = erase_split(node, right_node);
        }
        else if (left_node != nullptr)
            merge_flag = erase_split(left_node, node);
        else
            merge_flag = erase_split(node, right_node);
        
        if (merge_flag && node->parent != nullptr){
            if (isfew(node->parent))
                erase_ascend(node->parent);
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
        --this->m_stats.level_node[node->level];
        if (IS_LEAF_NODE(node)) --this->m_stats.data_node;
        else --this->m_stats.inner_node;
        node_allocator.free_node(node);
    }
    AEX_ASSERT(flag == false);
    return flag;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
    
    if (node == nullptr) return;
    if (IS_LEAF_NODE(node)){
        --this->m_stats.level_node[0];
        --this->m_stats.data_node;
        node_allocator.free_node(static_cast<data_node_ptr>(node));
        return;
    }
    else{
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        if (IS_ML_NODE(node)){
            bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
            for (slot_type i = 0; i < node->slot_size; ++i)
                if (bitmap_impl::at(bm, i))
                    this->erase_tree_recursive(child[i]);
        }
        else{
            AEX_PRINT("node=" << node << ", level=" << node->level);
            for (slot_type i = 0; i < node->size; ++i){
                AEX_PRINT("child=" << child[i]);
                this->erase_tree_recursive(child[i]);
            }
        }
        --this->m_stats.inner_node;
        --this->m_stats.level_node[node->level];
        node_allocator.free_node(static_cast<inner_node_ptr>(node));
    }
}

}