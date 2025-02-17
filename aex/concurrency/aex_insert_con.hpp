#pragma once

namespace aex
{
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    bool tail, flag, top_flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    data_node_ptr tail_leaf;
    node_ptr child;
    slot_type pos, split_pos;
    version_type node_version, child_version, top_node_version;
    hash_node node_copy, top_node_copy;
    int restart_count = 0;
insert_start:
    AEX_SGL_ASSERT(restart_count == 0);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    top_node = nullptr;
    node = root;
    node_version = node->node_lock.readLockOrRestart(need_restart); //SL(node)
    if (need_restart) goto insert_start;
    if (isfull(node)){
        TUL(node, node_version, need_restart); // UL(node)
        if (need_restart) goto insert_start;
        if (!isfull(node)){
            XU(node); goto insert_start;
        }
        if (node->type == NodeType::HashNode)
            expand(h_n(node));
        else{
            if (!d_n(node)->is_parent || !expand(d_n(node)))
                split_root(d_n(node));
        }
        DL(node, node_version); // DL(node)
    }

    while (true){
        if (node->type == NodeType::HashNode){
            node_copy = *h_n(node);
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto insert_start;
            child = find_insert_con(&node_copy, key, pos, child_version); // SL(child)
        }
        else{
            child = find_insert_con(d_n(node), key, pos); 
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto insert_start;
            child_version = child->node_lock.readLockOrRestart(need_restart);//SL(child);
            if (need_restart) goto insert_start;
        }
        node->node_lock.checkOrRestart(node_version, need_restart);
        if (need_restart) goto insert_start;
        
        AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        //tail = (node->type == NodeType::HashNode) ? (tail_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        tail = (node->type == NodeType::HashNode) ? (node_copy.tail_node == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild){
            size_type child_size = (node->type == NodeType::HashNode) ? child->size : child->size;
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                TUL(h_n(node), node_version, need_restart);  // UL(node)
                if (need_restart) goto insert_start;
                rebuild(node);
                node_version = DL(node, node_version);
            }
        }
        if (!tail){
            if (top_node != nullptr){
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode){
                top_node = h_n(node);
                top_node_version = node_version;
            }
        }
        if (child->type == NodeType::LeafNode){
            if (l_n(child)->key[(int)std::min(0, (int)child->size - 1)] < key){
                data_node_ptr next_node = l_n(child)->next;
                if (next_node != nullptr && next_node->min_key <= key)
                    goto insert_start;
            }
            if (!traits::AllowMultiKey && l_n(child)->find(key) > child->size){
                child->node_lock.readUnlockOrRestart(child_version, need_restart); // SU(child)
                if (need_restart) goto insert_start;
                return false;
            }
            //AEX_PRINT("child->size=" << child->size << ", isfull=" << isfull(l_n(child)));
            AEX_PRINT("child->size=" << child->size);
            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                child->node_lock.checkOrRestart(child_version, need_restart);
                if (need_restart) goto insert_start;
                if (top_node != nullptr && top_node != node){
                    top_node_copy = *h_n(top_node);
                    top_node->node_lock.checkOrRestart(top_node_version, need_restart); //check_lock_shared(top_node)
                    if (need_restart) goto insert_start;
                    split_pos = top_node_copy.predict(split_key);
                    if (split_pos < top_node_copy.slot_size && !top_node_copy.is_occupied(split_pos)){
                        top_flag = true;
                        node = top_node;
                        node_version = top_node_version;
                        node_copy = top_node_copy;
                    }
                    else
                        top_node = nullptr;
                }
                data_node_ptr new_node = new data_node();
                new_node->node_lock.writeLockOrRestart(need_restart); // XL(new_node)
                AEX_ASSERT(need_restart == false);
                if (node->type == NodeType::HashNode){
                    if (!top_flag){
                        node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                        if (need_restart) goto insert_start;
                        split_pos = node_copy.predict(split_key);
                    }
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                    if (need_restart) goto insert_start;
                    node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                    if (need_restart) goto insert_start;
                    split(l_n(child), new_node);
                    size_type add_cnt = 0;
                    if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){
                        insert_no_collision(&node_copy, split_pos, split_key, new_node);
                        add_cnt = node_copy.add_size_rand();
                    }
                    else{
                        AEX_ASSERT(top_flag == false);
                        if (top_flag){ XUNH(child); goto insert_start; }
                        pos = node_copy.prev_item(pos);
                        insert_collision(&node_copy, pos, split_key, new_node);
                    }
                    if ((node_copy.tail_node == child) || add_cnt > 0)
                        update_meta(h_n(node), child, add_cnt, node_version, need_restart);
                    else
                        node->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node)
                    AEX_SGL_ASSERT(need_restart == false);
                    if (need_restart){
                        XUNH(child);
                        free_node_helper(new_node);
                        goto insert_start;
                    }
                    complete(l_n(child), new_node);
                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XUNH(child); XUNH(new_node);
                }
                else{
                    node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node)
                    if (need_restart) goto insert_start;
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                    if (need_restart) goto insert_start;
                    split(l_n(child), new_node);
                    insert(d_n(node), split_key, new_node);
                    complete(l_n(child), new_node);
                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XUNH(node); XUNH(child); XUNH(new_node);
                }
            }
            else{
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                if (need_restart) goto insert_start; 
                l_n(child)->insert(key, value);
                AEX_PRINT("child->size=" << child->size);
                XUNH(child);
            }
            return true;
        }
        
        if (isfull(child)){
            if (child->type == NodeType::HashNode){
                TUL(h_n(node), node_version, need_restart);  // UL(child)
                if (need_restart) goto insert_start; 
                expand(h_n(child));
                DL(h_n(child), child_version);
            }
            else{
                flag = false;
                if (d_n(child)->is_parent){
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); //UL(child)
                    if (need_restart) goto insert_start; 
                    if (!expand(d_n(child)))
                        d_n(child)->is_parent = false;
                    else
                        flag = true;
                    DL(d_n(child), child_version);
                }
                if (!flag){
                    split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                    child->node_lock.checkOrRestart(child_version, need_restart); // check_lock_shared(child)
                    if (need_restart) goto insert_start; 
                    flag = (key >= split_key);
                    top_flag = false;
                    if (top_node != nullptr && top_node != node){
                        top_node_copy = *h_n(top_node);
                        top_node->node_lock.checkOrRestart(top_node_version, need_restart); // check_lock_shared(top_node)
                        if (need_restart) goto insert_start;
                        split_pos = top_node_copy.predict(split_key);
                        if (split_pos < top_node_copy.slot_size && !top_node_copy.is_occupied(split_pos)){
                            top_flag = true;
                            node = top_node;
                            node_version = top_node_version;
                            node_copy = top_node_copy;
                        }
                        else if (!flag)
                            top_node = nullptr;
                    }
                    dense_node_ptr new_node = allocator.allocate_dense_node();
                    new_node->node_lock.writeLockOrRestart(need_restart); // XL(new_node)
                    AEX_ASSERT(need_restart == false);
                    if (node->type == NodeType::HashNode){
                        if (!top_flag){
                            node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                            if (need_restart) goto insert_start;
                            split_pos = node_copy.predict(split_key);
                        }
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                        if (need_restart) goto insert_start;
                        split(d_n(child), new_node);
                        size_type add_cnt = 0;
                        if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){
                            tail_leaf = find_tail_leaf(child);
                            AEX_SGL_ASSERT(tail_leaf != nullptr);
                            AEX_ASSERT(check_lock(tail_leaf));
                            insert_no_collision(&node_copy, split_pos, split_key, new_node);
                            add_cnt = node_copy.add_size_rand();
                            XUNH(tail_leaf);
                        }
                        else {
                            AEX_ASSERT(top_flag == false);
                            pos = node_copy.prev_item(pos);
                            insert_collision(&node_copy, pos, split_key, new_node);
                            if (!flag) top_node = nullptr;
                        }
                        
                        if (node_copy.tail_node == child || add_cnt > 0)
                            update_meta(h_n(node), child, add_cnt, node_version, need_restart);
                        else
                            node->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node)

                        if (need_restart){
                            child->size = traits::DATA_NODE_SLOT_SIZE;
                            free_node_helper(new_node);
                            XUNH(child);
                            goto insert_start;
                        }
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        if (flag){
                            XUNH(child);
                            child_version = new_node->node_lock.downgradeLock();
                            child = new_node;
                        }
                        else{
                            XUNH(new_node);
                            child_version = child->node_lock.downgradeLock();
                        }                        
                    }
                    else{
                        AEX_ASSERT(node->type == NodeType::DenseNode);
                        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
                        if (need_restart) goto insert_start;
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart);
                        if (need_restart) { XUNH(node); goto insert_start;}
                        new_node = allocator.allocate_dense_node();
                        new_node->node_lock.writeLockOrRestart(need_restart);
                        AEX_ASSERT(need_restart == false);
                        split(d_n(child), new_node);
                        insert(d_n(node), split_key, new_node);
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        XUNH(node);
                        if (flag){
                            XUNH(child);
                            child_version = new_node->node_lock.downgradeLock();
                            child = new_node;
                        }
                        else{
                            XUNH(new_node);
                            top_node = nullptr;
                            child_version = child->node_lock.downgradeLock();
                        }
                    }
                    AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
                }
            }
        }
        if (top_node != nullptr && top_node != node){
            top_node->node_lock.checkOrRestart(top_node_version, need_restart);
            if (need_restart) goto insert_start;
        }
        node = i_n(child);
        node_version = child_version;
    }
    AEX_ASSERT(0 == 1);
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update_meta(hash_node_ptr node, const node_ptr child, const size_type add_cnt, const version_type &node_version, bool &need_restart){
    AEX_ASSERT(node->type == NodeType::HashNode);
    node->meta_lock.writeLockOrRestart(need_restart);
    if (need_restart) return;
    node->node_lock.readUnlockOrRestart(node_version, need_restart);
    if (!need_restart){
        if (node->tail_node == child)
            node->tail_node = tail_node(node);
        if (add_cnt > 0)
            node->size += add_cnt;
    }
    node->meta_lock.writeUnlock();
}

}
