#pragma once
#include "aex/aex.h"

namespace aex{


// TODO: 
/* erase a node from button to up */
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::erase_ascend(inner_node_ptr node){
    //AEX_PRINT("erase_ascend");
    if (node == nullptr) 
        return;
    bool merge_flag = true; 

    /* if the key is update */
    while (merge_flag && static_cast<node_ptr>(node) != this->root){
        merge_flag = false;
        if (!isfew(node)) 
            break;

        // now node item is few. rescale it
        if (rescale(node, node->real_slot_size() >> 1))
            break;

        inner_node_ptr parent = node->parent;
        // if rescale failed, the model can't fix the data or the item is fewer than min size
        // if items is large than min size, train it again
        if (node->slot_size > traits::MIN_INNER_NODE_SLOT_SIZE){
            std::vector<key_type> key_buf;
            std::vector<node_ptr> child_buf;
            split(node, key_buf, child_buf);
            if (child_buf.size() > 1)
                insert_ascend(node->parent, key_buf, child_buf);
            return;
        }
        // otherwise merge to near node
        else{
            merge_flag = true;     
            inner_node_ptr left_node = static_cast<inner_node_ptr>(node->prev);
            if (left_node != nullptr)
                if (left_node->parent != parent) left_node = nullptr;

            inner_node_ptr right_node = static_cast<inner_node_ptr>(node->next);
            if (right_node != nullptr)
                if (right_node->parent != parent) right_node = nullptr;

            //std::vector<key_type> key_buf(node->size);
            //std::vector<node_ptr> child_buf(node->size);
            if (right_node != nullptr)
                merge_flag = erase_merge(node, right_node);
            else if (left_node != nullptr && left_node->slot_size == right_node->slot_size)
                merge_flag = erase_merge(left_node, node);
            if (!merge_flag)
                return;
        }
        node = parent;
    }

    if (merge_flag){
        /* if the root has one child, change the root*/
        if (!IS_LEAF_NODE(node)){
            if (root->size == 1){
                node_ptr tmp = root;
                root = static_cast<inner_node_ptr>(root)->child_ptr[0];
                --m_stats.level_node[this->m_stats.height - 1];
                --m_stats.height;
                node_allocator.free_node(tmp);
            }
        }
        else if (node->size == 0){
            root = head_leaf = tail_leaf = nullptr;
            --m_stats.level_node[this->m_stats.height - 1];
            --m_stats.height;
            node_allocator.free_node(node);
        }
    }
}

// return merge flag
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_merge(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_WARNING("erase merge inner node");
    AEX_ASSERT(left_node->parent == right_node->parent);
    inner_node_ptr parent = right_node->parent;
    std::vector<key_type> key_buf(left_node->size), new_key;
    std::vector<node_ptr> child_buf(left_node->size), new_child;
    AEX_ASSERT(parent != nullptr);
    copy_to_buffer(left_node, key_buf.data(), child_buf.data());
    key_buf[left_node->size - 1] = parent->key_ptr[parent->at(left_node)];
    erase_link(left_node);
    erase_child_node(parent, left_node);
    insert_ascend(right_node, key_buf, child_buf);
    return isfew(right_node->parent);
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_merge(static_data_node_ptr __restrict__ left_node, static_data_node_ptr __restrict__ right_node){
    AEX_ASSERT(left_node->parent == right_node->parent);
    AEX_ASSERT(left_node != right_node);
    //AEX_WARNING("erase merge data node, " << left_node->size << ", " << right_node->size);
    if (left_node->size + right_node->size <= traits::MIN_DATA_NODE_SLOT_SIZE){
        //AEX_WARNING("!! erase merge data node, " << left_node->size << ", " << right_node->size);
        std::move_backward(right_node->key, right_node->key + right_node->size, right_node->key + right_node->size + left_node->size);
        std::move_backward(right_node->data, right_node->data + right_node->size, right_node->data + right_node->size + left_node->size);
        std::move(left_node->key, left_node->key + left_node->size, right_node->key);
        std::move(left_node->data, left_node->data + left_node->size, right_node->data);
        right_node->size += left_node->size;
        erase_link(left_node);
        erase_child_node(left_node->parent, left_node);
        return true;
    }
    else return false;
}

// return merge flag
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::erase_merge(dynamic_data_node_ptr __restrict__ left_node, dynamic_data_node_ptr __restrict__ right_node){
    std::vector<key_type> key_buf(left_node->size + right_node->size), new_key;
    std::vector<value_type> data_buf(left_node->size + right_node->size);
    std::vector<node_ptr> new_child;
    inner_node_ptr parent = right_node->parent;
    std::copy(left_node->key, left_node->key + left_node->size, key_buf.data());
    std::copy(left_node->data, left_node->data + left_node->size, data_buf.data());
    std::copy(right_node->key, right_node->key + right_node->size, key_buf.data() + left_node->size);
    std::copy(right_node->data, right_node->data + right_node->size, data_buf.data() + left_node->size);
    //split_with_linear_probe(key_buf.data(), data_buf.data(), right_node->level, new_key, new_child);
    split_with_exponential_probe(key_buf.data(), data_buf.data(), right_node->level, new_key, new_child);
    right_node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(right_node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(right_node, new_child);
    new_key.pop_back();
    new_child.pop_back();
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
void aex_tree<_Key, _Val, traits>::erase_iterator(const_iterator &iter){
    this->balance_stats.update_timestamp();

    data_node_ptr node = iter._M_node;
    
    if constexpr (traits::AllowDynamicDataNode::value){
        ((dynamic_node_ptr)node)->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    }

    AEX_ASSERT(node != empty_leaf);
    node->erase(iter.offset);
    --this->m_stats.size;

    if (traits::AllowDynamicDataNode::value && isfew(node)){
        rescale(node, ((dynamic_node_ptr)node)->slot_size >> 1);
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
                erase_ascend(parent);
            return;
        }

        data_node_ptr left_node = static_cast<data_node_ptr>(node->prev), right_node = static_cast<data_node_ptr>(node->next);

        if (left_node != nullptr)
            if (left_node->parent != parent) left_node = nullptr;

        if (right_node != nullptr)
            if (right_node->parent != parent) right_node = nullptr;

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

        if (traits::AllowDynamicDataNode::value && left_node != nullptr)
            while (isfew(left_node)) 
                rescale(left_node, ((dynamic_node_ptr)node)->slot_size >> 1);
            
        if (traits::AllowDynamicDataNode::value && right_node != nullptr)
            while (isfew(right_node)) 
                rescale(right_node, ((dynamic_node_ptr)node)->slot_size >> 1);

        bool merge_flag = true;
        if (left_node != nullptr && right_node != nullptr){
            if (left_node->size < right_node->size)
                merge_flag = erase_merge(left_node, node);
            else
                merge_flag = erase_merge(node, right_node);
        }
        else if (left_node != nullptr)
            merge_flag = erase_merge(left_node, node);
        else
            merge_flag = erase_merge(node, right_node);
        
        if (merge_flag && parent != nullptr){
            if (isfew(parent))
                erase_ascend(parent);
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
void aex_tree<_Key, _Val, traits>::erase_tree_recursive(node_ptr node){
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

}