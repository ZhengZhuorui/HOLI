#pragma once
#include "aex.h"

namespace aex{

#define CHECK_TOP_NODE() \
    { \
        if (top_node != nullptr && top_node != node){ \
            split_pos = top_node->predict(split_key); \
            if (split_pos < top_node->slot_size && !top_node->is_occupied(split_pos)){ \
                top_flag = true; \
                node = top_node; \
                pos = top_node->prev_item(split_pos); \
            } \
            else \
                top_node = nullptr; \
        }\
    }


template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    //AEX_PRINT("_insert_single_thread");
    bool tail, top_flag;
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
            if (d_n(node)->is_train == false && d_n(node)->level > 1){
                if (!expand(d_n(node)))
                    split_root(d_n(node));
            }
            else
                split_root(d_n(node));
        }
    }

    while (true){
        child = find_insert(node, key, pos);
        tail = (node->type == NodeType::HashNode) ? (h_n(node)->tail_node == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild)
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                rebuild(node);
            }
        if (!tail){
            if (top_node != nullptr)
                top_node = nullptr;
            if (node->type == NodeType::HashNode)
                top_node = h_n(node);
        }
        if (child->type == NodeType::LeafNode){
            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                CHECK_TOP_NODE();
                data_node_ptr new_node = new data_node();
                if (node->type == NodeType::HashNode){
                    if (!top_flag)
                        split_pos = h_n(node)->predict(split_key);
                    if (top_flag || (!h_n(node)->is_occupied(split_pos) && split_pos < h_n(node)->slot_size)){
                        insert_data_node(l_n(child), new_node, key, value);
                        insert_no_collision(h_n(node), split_pos, h_n(node)->next_item(split_pos + 1), split_key, new_node);
                    }
                    else{
                        insert_data_node(l_n(child), new_node, key, value);
                        AEX_ASSERT(new_node->key[0] == split_key);
                        pos = h_n(node)->prev_item(pos);
                        insert_collision(h_n(node), pos, h_n(node)->next_item(pos + 1), split_key, new_node);
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
                dense_node_ptr new_node;
                bool rebuild_flag = false;
                AEX_ASSERT(child->type == NodeType::DenseNode);
                split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                top_flag = false;
                CHECK_TOP_NODE();

                if (node->type == NodeType::HashNode){
                    split_pos = h_n(node)->predict(split_key);  
                    if (split_pos < h_n(node)->slot_size && !h_n(node)->is_occupied(split_pos)){
                        new_node = new_and_split(d_n(child));
                        insert_no_collision(h_n(node), split_pos, h_n(node)->next_item(split_pos + 1), split_key, new_node);
                    }
                    else {
                        if (d_n(child)->is_train == false && d_n(child)->level > 1) rebuild_flag = expand(d_n(child));
                        if (!rebuild_flag){
                            pos = h_n(node)->prev_item(pos);
                            new_node = new_and_split(d_n(child));
                            insert_collision(h_n(node), pos, h_n(node)->next_item(pos + 1), split_key, new_node);
                        }
                    }
                }
                else{
                    if (d_n(child)->is_train == false && d_n(child)->level > 1) rebuild_flag = expand(d_n(child));
                    if (!rebuild_flag){
                        new_node = new_and_split(d_n(child));
                        insert(d_n(node), split_key, new_node);
                    }
                }
                if (!rebuild_flag){
                    if (key >= split_key) child = new_node;
                    else top_node = nullptr;
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
    node->level = std::min((old_node->type == NodeType::DenseNode) ? d_n(old_node)->level + 1 : 0, (new_node->type == NodeType::DenseNode) ? d_n(new_node)->level + 1 : 0);
    node->size = 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    node->hash_table.insert(pos, std::make_pair(key, child));
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        node->hash_table.insert(i, std::make_pair(key, child));
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
    AEX_ASSERT(node->hash_table.find(pos).second == nullptr);
    node->hash_table.insert(pos, std::make_pair(key, child));
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        node->hash_table.insert(i, std::make_pair(key, child));
    bitmap_impl::set_one(node->bitmap_ptr, pos);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_collision(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(node->is_occupied(pos));
    #ifdef AEX_DEBUG
    opt_stats.allocate_dense_node_cnt++;
    #endif
    key_type prev_key;
    node_ptr prev_child;
    std::tie(prev_key, prev_child) = node->hash_table.find(pos);
    AEX_ASSERT(prev_child != nullptr);
    dense_node_ptr new_node = allocator.allocate_dense_node(false, 0);
    construct_tmp_node(new_node, prev_key, prev_child, key, child);
    AEX_ASSERT(prev_child->type == child->type);
    node->hash_table.update(pos, std::make_pair(new_node->key_ptr[0], new_node));
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        node->hash_table.update(i, std::make_pair(new_node->key_ptr[0], new_node));
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_no_collision(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(pos < node->slot_size);
    if ((pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        node->hash_table.update(pos, std::make_pair(key, child));
    else
        node->hash_table.insert(pos, std::make_pair(key, child));
    AEX_ASSERT(pos < next_pos);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        node->hash_table.update(i, std::make_pair(key, child));
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
    dense_node_ptr new_node_0 = allocator.allocate_dense_node(false, node->level);
    dense_node_ptr new_node_1 = allocator.allocate_dense_node(false, node->level);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->key_ptr, d_n(new_node_0)->key_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->child_ptr, d_n(new_node_0)->child_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->key_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, d_n(new_node_1)->key_ptr);
    move_item_avx<traits::DENSE_NODE_SLOT_SIZE / 2>(d_n(node)->child_ptr + traits::DENSE_NODE_SLOT_SIZE / 2, d_n(new_node_1)->child_ptr);
    new_node_0->size = new_node_1->size = traits::DENSE_NODE_SLOT_SIZE / 2;
    node->size = 2;
    d_n(node)->key_ptr[0] = new_node_0->key_ptr[0]; d_n(node)->child_ptr[0] = new_node_0;
    d_n(node)->key_ptr[1] = new_node_1->key_ptr[0]; d_n(node)->child_ptr[1] = new_node_1;
    d_n(node)->is_train = true;
    ++node->level;
}

}
