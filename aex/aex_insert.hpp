#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    #define INSERT_UNLOCK() insert_unlock(top_node, top_pos, top_next_pos, node, pos, next_pos, child)

    bool restart = false, inserted, tail;
    key_type split_key;
    hash_node_ptr top_node = nullptr;
    slot_type pos, next_pos, top_pos = 0, top_next_pos = 0, split_pos;
    data_node_ptr new_node;
    node_ptr node, child;
    version_type now_version;
    std::pair<iterator, bool> ret;

insert_start:
    now_version = this->version;
    if (insert_init(key, value, ret))
        return ret;

    node = root;
    SL(node);    
    if (check_insert_SMO(i_n(node))){
        SU(node);
        goto insert_start;
    }

    while (true){

insert_loop_start:
        child = find_update(i_n(node), key, pos, next_pos);
        tail = (node->type == NodeType::HashNode) ? (next_pos >= h_n(node)->slot_size) : (next_pos >= d_n(node)->size);
        if (tail){
            if (top_node != nullptr){
                top_node->array_unlock_shared(top_pos - 1, top_next_pos);
                SU(top_node);
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode && next_pos - pos > 1){
                top_node = h_n(node);
                top_pos  = pos;
                top_next_pos = next_pos;
            }
        }

        if (child->type == NodeType::LeafNode){
            if (now_version < l_n(child)->version){
                INSERT_UNLOCK();
                goto insert_start;
            }
            
            pos = l_n(child)->find_lower_pos(key);
            if (!traits::AllowMultiKey && pos < l_n(child)->size && l_n(child)->key[pos] == key){
                ret = std::make_pair(iterator(l_n(child), pos), false);
                INSERT_UNLOCK();
                return ret;
            }

            if (!TUL(child)){
                INSERT_UNLOCK();
                goto insert_start;
            }
            if (isfull(l_n(child))){
                inserted = false;
                split_key = l_n(child)->key[traits::MIN_DATA_NODE_SLOT_SIZE >> 1];
                if (top_node != nullptr){
                    split_pos = top_node->predict(split_key);
                    inserted = check_upgrade_and_lock(top_node, split_pos, top_pos, top_next_pos, restart);
                    if (restart){
                        INSERT_UNLOCK();
                        goto insert_start;
                    }
                    if (inserted){
                        ret = insert_data_node(l_n(child), new_node, key, value);
                        insert(top_node, split_pos, top_next_pos, split_key, new_node);
                    }
                }
                if (!inserted){
                    if (node->type == NodeType::HashNode){
                        split_pos = h_n(node)->predict(split_key);
                        if (!h_n(node)->try_array_upgrade_lock(split_pos, next_pos)){
                            INSERT_UNLOCK();
                            goto insert_start;
                        }
                        ret = insert_data_node(l_n(child), new_node, key, value);
                        insert(h_n(node), split_pos, next_pos, split_key, new_node);
                    }
                    else{
                        if (!TUL(node)){
                            INSERT_UNLOCK();
                            goto insert_start;
                        }
                        ret = insert_data_node(l_n(child), new_node, key, value);
                        insert(d_n(node), split_key, new_node);
                    }
                }
            }
            else{
                ret = std::make_pair(iterator(l_n(child), pos), true);
                l_n(child)->insert(key, value);
            }
            INSERT_UNLOCK();
            return ret;
        }

        if (!check_insert_SMO(i_n(child))){
            if (node->type == NodeType::HashNode)
                h_n(node)->array_unlock_shared(pos - 1, next_pos);
            SU(child);
            goto insert_loop_start;
        }

        if (top_node != node){
            AEX_ASSERT(tail == true);
            if (node->type == NodeType::HashNode)
                h_n(node)->array_unlock_shared(pos - 1, next_pos);
            SU(node);
        }
        node = child;
    }
    AEX_ASSERT(0 == 1);
    return ret;
    #undef INSERT_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_tmp_node(dense_node_ptr node, const key_type &old_key, const node_ptr old_node, const key_type &new_key, const node_ptr new_node){
    node->key_ptr[0]   = old_key;
    node->key_ptr[1]   = new_key;
    node->child_ptr[0] = old_node;
    node->child_ptr[1] = new_node;
    if (old_key > new_key){
        std::swap(node->key_ptr[0],   node->key_ptr[1]);
        std::swap(node->child_ptr[0], node->child_ptr[1]);
    }
    node->size = 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    //AEX_PRINT("pos=" << pos << ", next_pos=" << next_pos);
    //printf("bitmap[pos_slot]=%llx\n", node->bitmap_ptr[pos >> 6]);
    AEX_ASSERT(node->is_occupied(pos) == false);
    AEX_ASSERT(hash_table.find(node, pos).second == nullptr);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    AEX_ASSERT(node->is_occupied(pos) == true);
    hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit_64(pos + 1); i < next_pos; i += traits::SLOT_PER_LOCK)
        hash_table.insert(node, i, key, child);
    ++node->size;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    AEX_ASSERT(node->is_occupied(pos) == false);
    AEX_PRINT("pos=" << pos << ", pos & 63=" << (pos & 63) << ", next_pos=" << next_pos << ", key=" << key);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    if ((pos & 63) == 0)
        hash_table.update(node, pos, key, child);
    else
        hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit_64(pos + 1); i < next_pos; i += traits::SLOT_PER_LOCK){
        AEX_PRINT("i=" << i);
        hash_table.update(node, i, key, child);
    }
    node->meta_lock.lock();
    ++node->size;
    node->meta_lock.unlock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    key_type old_key;
    node_ptr old_node;

    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(check_unlock(node));
    AEX_ASSERT(check_lock(child) || check_lock_shared(child));
    
    if (node->is_occupied(pos)){
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        #endif
        dense_node_ptr new_node = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
        std::tie(old_key, old_node) = hash_table.find(node, pos);
        construct_tmp_node(new_node, old_key, old_node, key, child);
        hash_table.update(node, pos, new_node->key_ptr[0], new_node);
        for (slot_type i = highbit_64(pos + 1); i < next_pos; i += traits::SLOT_PER_LOCK)
            hash_table.update(node, i, new_node->key_ptr[0], new_node);
    }
    else 
        __construct_insert(node, pos, next_pos, key, child);
    node->array_downgrade_lock(pos, next_pos);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(dense_node_ptr node, const key_type &key, const node_ptr child){
    AEX_ASSERT(check_lock(node));
    slot_type pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    std::move_backward(node->key_ptr   + pos, node->key_ptr   + node->size, node->key_ptr   + node->size + 1);
    std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + 1);
    node->key_ptr[pos]   = key;
    node->child_ptr[pos] = child;
    ++node->size;
    DL(node);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_insert_SMO(inner_node_ptr node){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (is_rebuild(node)){
        if (!TUL(node))
            return false;
        rebuild(node);
        DL(node);
    }
    else if (isfull(node)){
        if (!TUL(node))
            return false;
        while (isfull(node))
            expand(node);
        DL(node);
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_upgrade_and_lock(hash_node_ptr node, const slot_type split_pos, const slot_type top_pos, const slot_type top_next_pos, bool &restart){
    AEX_ASSERT(node->type == NodeType::HashNode);
    restart = false;
    if constexpr (traits::AllowConcurrency)
        return true;
    if (top_pos == split_pos || top_next_pos == split_pos)
        return false;
    AEX_ASSERT(!node->is_occupied(split_pos));
    //if (node->is_occupied(split_pos))    
    //    return false;
    restart = node->try_array_upgrade_lock(split_pos, top_next_pos);
    if (restart)
        return false;
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_init(const key_type &key, const value_type &value, std::pair<iterator, bool> &res){
    data_node_ptr new_node;
insert_init_start:
    SL();
    if (root == nullptr){
        if (!TUL()){
            SU();
            goto insert_init_start;
        }
        #ifdef AEX_DEBUG
        opt_stats.allocate_data_node_cnt++;
        #endif
        new_node = new data_node(this->version);
        head_leaf = new_node;
        new_node->insert(key, value, 0);
        new_node->next = nullptr;
        this->m_stats.size = 1;
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        #endif
        root = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
        d_n(root)->key_ptr[0]   = std::numeric_limits<key_type>::lowest();
        d_n(root)->child_ptr[0] = new_node;
        XU();
        res = std::make_pair(iterator(new_node, 0), true);
        return true;
    }
    this->meta_lock.lock();
    ++this->m_stats.size;
    this->meta_lock.unlock();
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert_data_node(data_node_ptr node, data_node_ptr &new_node, const key_type &key, const value_type &value){
    slot_type pos = node->find_lower_pos(key);
    std::pair<iterator, bool> ret;
    #ifdef AEX_DEBUG
    opt_stats.allocate_data_node_cnt++;
    #endif
    new_node = new data_node(this->version);
    split(node, new_node);
    if (pos <= node->size){
        l_n(node)->insert(key, value, pos);
        ret = std::make_pair(iterator(node, pos), true);
    }
    else {
        pos -= node->size;
        l_n(new_node)->insert(key, value, pos);
        ret = std::make_pair(iterator(new_node, pos), true);
    }
    DL(node);
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_unlock(hash_node_ptr top_node, const slot_type top_pos, const slot_type top_next_pos, node_ptr node, const slot_type pos, const slot_type next_pos, node_ptr child){
    AEX_ASSERT(check_lock_shared(top_node));
    if constexpr (!traits::AllowConcurrency)
        return;
    top_node->array_unlock_shared(top_pos - 1, top_next_pos);           
    SU(top_node);                                                   
    if (top_node != node){                                          
        if (node->type == NodeType::HashNode)
            h_n(node)->array_unlock_shared(pos - 1, next_pos);                    
        SU(node);
    } 
    XU(child);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(data_node_ptr old_node, data_node_ptr new_node){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    #ifdef AEX_DEBUG
    ++opt_stats.data_node_split_cnt;
    #endif
    
    if constexpr (traits::AllowConcurrency){
        ++this->version;
        old_node->version = new_node->version = this->version;
    }

    new_node->next = old_node->next;
    old_node->next = new_node;
    
    ULL mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    std::move(old_node->key  + mid, old_node->key  + old_node->size, new_node->key );
    std::move(old_node->data + mid, old_node->data + old_node->size, new_node->data);
    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

}
