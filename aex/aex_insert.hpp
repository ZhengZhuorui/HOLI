#pragma once
#include "aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    //AEX_PRINT("_insert_single_thread");
    bool tail, flag, top_flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child;
    slot_type pos, split_pos;
    top_node = nullptr;
    node = root; 
    if (isfull(node)){
        if (node->type == NodeType::HashNode)
            expand(h_n(node));
        else{
            if (!d_n(node)->is_parent || !expand(d_n(node)))
                split_root(d_n(node));
        }
    }

    while (true){
        child = find_insert(node, key, pos);
        //AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        // TODO: record hash node tail node
        //tail = (node->type == NodeType::HashNode) ? (tail_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        tail = (node->type == NodeType::HashNode) ? (h_n(node)->tail_node == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild)
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                //AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", slot_size=" << node->slot_size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size)
                rebuild(node);
            }
        if (!tail){
            if (top_node != nullptr)
                top_node = nullptr;
            if (node->type == NodeType::HashNode)
                top_node = h_n(node);
        }
        if (child->type == NodeType::LeafNode){
            AEX_DEBUG_BLOCK({if (l_n(child)->next != nullptr)if (l_n(child)->next->key[0] < key){
                AEX_ERROR("key=" << key << ", " << l_n(child)->key[0] << ", " << l_n(child)->key[child->size - 1] << ", " << l_n(child)->next->key[0]);
                AEX_ERROR("node->type=" << to_string(node->type));
                AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
                if (node->type == NodeType::HashNode){
                    bool is_occ = h_n(node)->is_occupied(h_n(node)->predict(key));
                    AEX_PRINT("pos1=" << h_n(node)->predict(key) << ", pos2=" << h_n(node)->predict(l_n(child)->key[0]) << ", pos3=" << h_n(node)->predict(l_n(child)->next->key[0]) << ", is_occupied=" << is_occ);
                    if (is_occ){
                        key_type find_key; node_ptr _;
                        std::tie(find_key, _) = hash_table.find(h_n(node), h_n(node)->predict(key));
                        AEX_PRINT(find_key << ", " << _ << ", " << l_n(child)->next);
                    }
                }
                AEX_ASSERT(0 == 1);
            }});
            
            if constexpr (!traits::AllowMultiKey){
                slot_type pos = l_n(child)->find(key);
                if (pos < child->size && l_n(child)->key[pos] == key) 
                    return false;
            }

            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                if (top_node != nullptr && top_node != node){
                    split_pos = top_node->predict(split_key);
                    if (split_pos < top_node->slot_size && !top_node->is_occupied(split_pos)){
                        top_flag = true;
                        node = top_node;
                    }
                }
                data_node_ptr new_node = new data_node();
                if (node->type == NodeType::HashNode){
                    if (!top_flag)
                        split_pos = h_n(node)->predict(split_key);
                    if (top_flag || (!h_n(node)->is_occupied(split_pos) && split_pos < h_n(node)->slot_size)){
                        insert_data_node(l_n(child), new_node, key, value);
                        AEX_ASSERT(new_node->key[0] == split_key);
                        insert_no_collision(h_n(node), split_pos, split_key, new_node);
                    }
                    else{
                        insert_data_node(l_n(child), new_node, key, value);
                        AEX_ASSERT(new_node->key[0] == split_key);
                        pos = h_n(node)->prev_item(pos);
                        insert_collision(h_n(node), pos, split_key, new_node);
                    }
                    if (h_n(node)->tail_node == child)
                        h_n(node)->tail_node = tail_node(h_n(node));

                }
                else{
                    insert_data_node(l_n(child), new_node, key, value);
                    insert(d_n(node), split_key, new_node);
                }
            }
            else
                l_n(child)->insert(key, value);
            ++this->_size;
            return true;
        }
        
        if (isfull(i_n(child))){
            if (child->type == NodeType::HashNode){
                expand(h_n(child));
            }
            else{
                AEX_ASSERT(child->type == NodeType::DenseNode);
                flag = false;
                //if (d_n(child)->is_parent){
                //    if (!expand(d_n(child)))
                //        d_n(child)->is_parent = false;
                //    else
                //        flag = true;
                //}
                if (!flag){
                    split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                    top_flag = false;
                    if (top_node != nullptr && top_node != node){
                        split_pos = top_node->predict(split_key);
                        if (split_pos < top_node->slot_size && !top_node->is_occupied(split_pos)){
                            top_flag = true;
                            node = top_node;
                        }
                        else if (!flag){
                            top_node = nullptr;
                        }
                    }
                    dense_node_ptr new_node;

                    if (d_n(child)->is_parent){
                        flag = true;
                        if (top_flag) flag = false;
                        else if (node->type == NodeType::HashNode){
                            split_pos = h_n(node)->predict(split_key);  
                            if (split_pos < h_n(node)->slot_size && !h_n(node)->is_occupied(split_pos)) flag = false;
                        }
                        if (flag){
                            if (!expand(d_n(child))){
                                d_n(child)->is_parent = false;
                                flag = false;
                            }
                        }
                    }
                    
                    if (!flag){
                        flag = (key >= split_key);
                        if (node->type == NodeType::HashNode){
                            if (!top_flag) split_pos = h_n(node)->predict(split_key);  
                            new_node = allocator.allocate_dense_node();
                            split(d_n(child), new_node);
                            if (top_flag || (split_pos < h_n(node)->slot_size && !h_n(node)->is_occupied(split_pos))){
                                insert_no_collision(h_n(node), split_pos, split_key, new_node);
                            }
                            else{
                                pos = h_n(node)->prev_item(pos);
                                insert_collision(h_n(node), pos, split_key, new_node);
                                if (!flag) top_node = nullptr;
                            }
                            if (h_n(node)->tail_node == child)
                                h_n(node)->tail_node = tail_node(h_n(node));
                            AEX_ASSERT(new_node->key_ptr[0] == split_key);
                            if (flag) child = new_node; 
                        }
                        else{
                            AEX_ASSERT(node->type == NodeType::DenseNode);
                            new_node = allocator.allocate_dense_node();
                            split(d_n(child), new_node);
                            insert(d_n(node), split_key, new_node);
                            AEX_ASSERT(new_node->key_ptr[0] == split_key);
                            if (flag) child = new_node; 
                            else top_node = nullptr;
                        }
                        AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
                    }
                }
            }
        }
        node = i_n(child);
    }
    AEX_ASSERT(0 == 1);
    return true;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_tmp_node(dense_node_ptr node, const key_type old_key, const node_ptr old_node, const key_type new_key, const node_ptr new_node){
    node->key_ptr[0]   = old_key;
    node->key_ptr[1]   = new_key;
    node->child_ptr[0] = old_node;
    node->child_ptr[1] = new_node;
    if (old_key > new_key){
        std::swap(node->key_ptr[0],   node->key_ptr[1]);
        std::swap(node->child_ptr[0], node->child_ptr[1]);
    }
    node->is_parent = ((old_node->type == NodeType::DenseNode) & (new_node->type == NodeType::DenseNode) );
    node->size = 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    //AEX_ASSERT(hash_table.find(node, pos).second == nullptr);
    hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.insert(node, i, key, child);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    ++node->size;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert_con(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    AEX_ASSERT(hash_table.find(node, pos).second == nullptr);
    hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.insert(node, i, key, child);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::insert_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(node->is_occupied(pos));
    #ifdef AEX_DEBUG
    opt_stats.allocate_dense_node_cnt++;
    #endif
    dense_node_ptr new_node = allocator.allocate_dense_node();
    key_type prev_key;
    node_ptr prev_child;
    std::tie(prev_key, prev_child) = hash_table.find(node, pos);
    AEX_ASSERT(prev_child != nullptr);
    construct_tmp_node(new_node, prev_key, prev_child, key, child);
    AEX_ASSERT(prev_child->type == child->type);
    hash_table.update(node, pos, new_node->key_ptr[0], new_node);
    slot_type next_pos = node->next_item(pos + 1);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, new_node->key_ptr[0], new_node);
    return new_node;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_no_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child){
    
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(pos < node->slot_size);
    if ((pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        hash_table.update(node, pos, key, child);
    else
        hash_table.insert(node, pos, key, child);
    slot_type next_pos = node->next_item(pos + 1);
    AEX_ASSERT(pos < next_pos);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, key, child);
    node->set_one(pos);
    if constexpr (!traits::AllowConcurrency)
        ++node->size;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(dense_node_ptr node, const key_type key, const node_ptr child){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(node->type == NodeType::DenseNode);
    const slot_type pos = std::upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr;
    std::move_backward(node->key_ptr   + pos, node->key_ptr   + node->size, node->key_ptr   + node->size + 1);
    std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + 1);
    node->key_ptr[pos]   = key;
    node->child_ptr[pos] = child;
    ++node->size;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_data_node(data_node_ptr node, data_node_ptr new_node, const key_type key, const value_type &value){
    #ifdef AEX_DEBUG
    opt_stats.allocate_data_node_cnt++;
    #endif
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(check_lock(new_node));
    split(node, new_node);
    if (key < new_node->key[0]){
        node->insert(key, value);
        AEX_ASSERT(key <= new_node->key[0]);
    }
    else {
        new_node->insert(key, value);
        AEX_ASSERT(key >= node->key[node->size - 1]);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_unlock(inner_node_ptr top_node, inner_node_ptr node) const {
    if constexpr (!traits::AllowConcurrency)
        return;                
    SU(node);
    if (top_node != node && top_node != nullptr){                       
        SU(top_node);
    } 
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(data_node_ptr old_node, data_node_ptr new_node){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    AEX_DEBUG_BLOCK({ if (old_node->size != traits::DATA_NODE_SLOT_SIZE) AEX_PRINT("old_node->size=" << old_node->size);});
    AEX_ASSERT(old_node->size == traits::DATA_NODE_SLOT_SIZE);
    AEX_ASSERT(check_lock(old_node));
    #ifdef AEX_DEBUG
    ++opt_stats.data_node_split_cnt;
    #endif

    //std::move(old_node->key  + traits::DATA_NODE_SLOT_SIZE / 2, old_node->key  + traits::DATA_NODE_SLOT_SIZE, new_node->key );
    //std::move(old_node->data + traits::DATA_NODE_SLOT_SIZE / 2, old_node->data + traits::DATA_NODE_SLOT_SIZE, new_node->data);
    move_item_avx<traits::DATA_NODE_SLOT_SIZE / 2>(old_node->key + traits::DATA_NODE_SLOT_SIZE / 2, new_node->key);
    move_item_avx<traits::DATA_NODE_SLOT_SIZE / 2>(old_node->data + traits::DATA_NODE_SLOT_SIZE / 2, new_node->data);
    new_node->next = old_node->next;
    old_node->next = new_node;        
    old_node->size = new_node->size = traits::DATA_NODE_SLOT_SIZE / 2;
    if constexpr (traits::AllowConcurrency){
        new_node->next_min_key = old_node->next_min_key;
        //new_node->min_key = new_node->key[0];
        old_node->next_min_key = new_node->key[0];
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(dense_node_ptr old_node, dense_node_ptr new_node){
    //AEX_PRINT("old_node->size=" << old_node->size);
    AEX_ASSERT(old_node->size == traits::DENSE_NODE_SLOT_SIZE);
    AEX_ASSERT(check_lock(old_node));
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_split_cnt;
    #endif
    //std::move(old_node->key_ptr   + traits::DENSE_NODE_SLOT_SIZE / 2, old_node->key_ptr   + traits::DENSE_NODE_SLOT_SIZE, new_node->key_ptr  );
    //std::move(old_node->child_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, old_node->child_ptr + traits::DENSE_NODE_SLOT_SIZE, new_node->child_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(old_node->key_ptr   + traits::DENSE_NODE_SLOT_SIZE / 2, new_node->key_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(old_node->child_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, new_node->child_ptr);
    old_node->size = new_node->size = traits::DENSE_NODE_SLOT_SIZE / 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split_root(dense_node_ptr node){
    dense_node_ptr new_node_0 = allocator.allocate_dense_node();
    dense_node_ptr new_node_1 = allocator.allocate_dense_node();
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->key_ptr, d_n(new_node_0)->key_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->child_ptr, d_n(new_node_0)->child_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->key_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, d_n(new_node_1)->key_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->child_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, d_n(new_node_1)->child_ptr);
    new_node_0->size = new_node_1->size = traits::DENSE_NODE_SLOT_SIZE / 2;
    node->size = 2;
    d_n(node)->key_ptr[0] = new_node_0->key_ptr[0]; d_n(node)->child_ptr[0] = new_node_0;
    d_n(node)->key_ptr[1] = new_node_1->key_ptr[0]; d_n(node)->child_ptr[1] = new_node_1;
    d_n(node)->is_parent = true;
}

}
