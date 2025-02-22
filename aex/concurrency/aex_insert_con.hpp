#pragma once

namespace aex
{
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con(const key_type key, const value_type &value){
    bool tail, flag, top_flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child;
    slot_type pos, split_pos;
    version_type node_version, child_version, top_node_version;
    hash_node node_copy, top_node_copy;
    int restart_count = 0;
insert_start:
    AEX_SGL_ASSERT(restart_count == 0);
    AEX_ASSERT(restart_count < 1000000000);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    top_node = nullptr;
    node = root;
    node_version = node->node_lock.readLockOrRestart(need_restart); //SL(node) | node S
    if (need_restart) goto insert_start;
    if (isfull(node)){
        TUL(node, node_version, need_restart); // UL(node) | node X
        if (need_restart) goto insert_start;
        if (!isfull(node)){
            XU(node); goto insert_start; // node #
        }
        if (node->type == NodeType::HashNode)
            expand(h_n(node));
        else{
            if (!d_n(node)->is_parent || !expand(d_n(node)))
                split_root(d_n(node));
        }
        DL(node, node_version); // DL(node) | node S
        AEX_DEBUG_BLOCK({if constexpr (traits::AllowConcurrency) if (node->type == NodeType::HashNode) AEX_ASSERT(!node->meta_lock.isLocked());});
    }

    while (true){
        if (node->type == NodeType::HashNode){
            node_copy = *h_n(node);
            node->node_lock.checkOrRestart(node_version, need_restart); // node S
            if (need_restart) goto insert_start;
            child = find_insert_con(&node_copy, key, pos, child_version); // SL(child) | node S, child S
        }
        else{
            child = find_insert_con(d_n(node), key, pos); 
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto insert_start;
            child_version = child->node_lock.readLockOrRestart(need_restart);//SL(child); | node S, child S
            if (need_restart) goto insert_start;
        }
        node->node_lock.checkOrRestart(node_version, need_restart); // node S, child S
        if (need_restart) goto insert_start;
        
        //AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        //tail = (node->type == NodeType::HashNode) ? (tail_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        tail = (node->type == NodeType::HashNode) ? (node_copy.tail_node == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild){
            size_type child_size = (node->type == NodeType::HashNode) ? child->size : child->size;
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                TUL(h_n(node), node_version, need_restart);  // UL(node) | node X
                if (need_restart) goto insert_start;
                rebuild(node);
                node_version = DL(node, node_version); // node S
            }
        }
        if (!tail){
            if (top_node != nullptr)
                top_node = nullptr;
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
            
            
            if constexpr (!traits::AllowMultiKey){
                slot_type pos = l_n(child)->find(key);
                bool ret_flag = true;
                if (pos < child->size && l_n(child)->key[pos] == key) 
                    ret_flag = false;                    
                child->node_lock.readUnlockOrRestart(child_version, need_restart); // SU(child) | node S, child S
                if (need_restart) goto insert_start;
                if (!ret_flag)
                    return false;
            }

            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                child->node_lock.checkOrRestart(child_version, need_restart); // node S, child S
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
                AEX_ASSERT(need_restart == false);
                if (node->type == NodeType::HashNode){
                    if (!top_flag){
                        node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                        if (need_restart) goto insert_start;
                        split_pos = node_copy.predict(split_key);
                    }

                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                    if (need_restart) goto insert_start;

                    size_type add_cnt = 0;
                    data_node_ptr new_node;
                    if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){
                        node_copy.lock_array[pos2slot(split_pos)].writeLockOrRestart(need_restart); // XL(S, split_pos) | node S, node[split_pos] X, child X,
                        if (need_restart){ XUNH(child); goto insert_start; } // node S
                        if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){
                            new_node = new_and_split(l_n(child)); // node S, node[split_pos] X, child X, new_node X
                            insert_no_collision(&node_copy, split_pos, split_key, new_node); 
                            add_cnt = node_copy.add_size_rand();
                        }
                        else{
                            XUNH(child);  // node S, node[split_pos] X
                            node_copy.lock_array[pos2slot(split_pos)].writeUnlock(); // node S
                            goto insert_start;
                        }
                        node_copy.lock_array[pos2slot(split_pos)].writeUnlock(); // node S, child X, new_node X
                    }
                    else{
                        AEX_ASSERT(top_flag == false); // node S, child X
                        if (top_flag){ XUNH(child); goto insert_start; } // node S
                        pos = node_copy.prev_item(pos);
                        new_node = new_and_split(l_n(child)); // node S, child X, new_node X
                        insert_collision(&node_copy, pos, split_key, new_node); 
                    }

                    if ((node_copy.tail_node == child) || add_cnt > 0)
                        update_meta(h_n(node), child, add_cnt, node_version, need_restart);
                    else
                        node->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node)

                    AEX_SGL_ASSERT(need_restart == false);
                    if (need_restart){
                        XUNH(child); // node S, new_node X
                        free_node_helper(new_node); // node S
                        goto insert_start;
                    }
                    complete(l_n(child), new_node);
                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XUNH(child); XUNH(new_node); // node S
                }
                else{
                    AEX_ASSERT(node->type == NodeType::DenseNode);
                    node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node) | node X
                    if (need_restart) goto insert_start;
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node X, child X
                    if (need_restart) { XUNH(node); goto insert_start; }
                    data_node_ptr new_node = new_and_split(l_n(child)); // node X, child X, new_node X
                    insert(d_n(node), split_key, new_node);
                    complete(l_n(child), new_node);
                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XUNH(node); XUNH(child); XUNH(new_node); // #
                }
            }
            else{
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                if (need_restart) goto insert_start; 
                l_n(child)->insert(key, value);
                XUNH(child); // node S
            }
            return true;
        }
        
        if (isfull(i_n(child))){
            if (child->type == NodeType::HashNode){
                TUL(h_n(child), child_version, need_restart);  // UL(child) | node S, child X
                if (need_restart) goto insert_start; 
                expand(h_n(child));
                DL(h_n(child), child_version); // node S, child S
            }
            else{
                flag = false; // node S, child S
                if (d_n(child)->is_parent){
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                    if (need_restart) goto insert_start; 
                    if (!expand(d_n(child)))
                        d_n(child)->is_parent = false;
                    else
                        flag = true;
                    DL(child, child_version); // node S, child S
                }
                if (!flag){ // node S, child S
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
                    AEX_ASSERT(need_restart == false);
                    if (node->type == NodeType::HashNode){
                        if (!top_flag){
                            node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                            if (need_restart) goto insert_start;
                            split_pos = node_copy.predict(split_key);
                        }
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                        if (need_restart) goto insert_start;
                        dense_node_ptr new_node;
                        size_type add_cnt = 0;
                        if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){ // node S, child X
                            node_copy.lock_array[pos2slot(split_pos)].writeLockOrRestart(need_restart); // XL(node[split_pos]) | node S, child X
                            if (need_restart){ XUNH(child); goto insert_start; } // node S
                            if (split_pos < node_copy.slot_size && !node_copy.is_occupied(split_pos)){ // node S, node[split_pos] X, child X
                                new_node = new_and_split(d_n(child)); // node S, node[split_pos] X, child X, new_node X
                                insert_no_collision(&node_copy, split_pos, split_key, new_node);
                                add_cnt = node_copy.add_size_rand();
                            }
                            else{  // node S, node[split_pos] X, child X
                                node_copy.lock_array[pos2slot(split_pos)].writeUnlock(); // node S,  child X
                                XUNH(child); // node S
                                goto insert_start;
                            }
                            node_copy.lock_array[pos2slot(split_pos)].writeUnlock(); // node S, child X, new_node X
                        }// node S, child X, new_node X
                        else { // node S, child X
                            AEX_ASSERT(top_flag == false);
                            if (top_flag){ XUNH(child); goto insert_start; } // node S
                            pos = node_copy.prev_item(pos);
                            new_node = new_and_split(d_n(child)); // node S, child X, new_node X
                            insert_collision(&node_copy, pos, split_key, new_node);
                            if (!flag) top_node = nullptr;
                        } // node S, child X, new_node X
                        
                        if (node_copy.tail_node == child || add_cnt > 0)
                            update_meta(h_n(node), child, add_cnt, node_version, need_restart);
                        else
                            node->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node)

                        if (need_restart){
                            child->size = traits::DATA_NODE_SLOT_SIZE;
                            free_node_helper(new_node); // node S, child X
                            XUNH(child); // node S
                            goto insert_start;
                        }
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        if (flag){
                            XUNH(child); // node S, new_node X
                            child_version = new_node->node_lock.downgradeLock(); // DL(new_node) | node S, new_node S
                            child = new_node; // child S
                        }
                        else{
                            XUNH(new_node);  // node S, child X
                            child_version = child->node_lock.downgradeLock(); // DL(child) | node S, child S
                        }                        
                    }
                    else{
                        AEX_ASSERT(node->type == NodeType::DenseNode);
                        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);  // UL(node)
                        if (need_restart) goto insert_start; // node X
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                        if (need_restart) { XUNH(node); goto insert_start;} // node X, child X
                        dense_node_ptr new_node = new_and_split(d_n(child)); // node X, child X, new_node X
                        insert(d_n(node), split_key, new_node);
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        XUNH(node); // child X, new_node X
                        if (flag){
                            XUNH(child); // new_node X
                            child_version = new_node->node_lock.downgradeLock(); // DL(new_node) | new_node S
                            child = new_node; // child S
                        }
                        else{
                            XUNH(new_node); // child X
                            top_node = nullptr;
                            child_version = child->node_lock.downgradeLock(); // DL(child) | child S
                        }
                    }
                    AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
                }
            }
        }
        // require: child S
        if (top_node != nullptr && top_node != node){
            top_node->node_lock.checkOrRestart(top_node_version, need_restart);
            if (need_restart) goto insert_start;
        }
        node = i_n(child);
        node_version = child_version; // node S
    }
    AEX_ASSERT(0 == 1);
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update_meta(hash_node_ptr node, const node_ptr child, const size_type add_cnt, const version_type &node_version, bool &need_restart){
    AEX_ASSERT(node->type == NodeType::HashNode);
    //AEX_PRINT(node << ", " << node->meta_lock.typeVersionLockObsolete.load());
    node->meta_lock.writeLockOrRestart(need_restart);
    AEX_SGL_ASSERT(need_restart == false);
    if (need_restart) return;
    node->node_lock.checkOrRestart(node_version, need_restart);
    AEX_SGL_ASSERT(need_restart == false);
    if (!need_restart){
        if (node->tail_node == child)
            node->tail_node = tail_node(node);
        if (add_cnt > 0)
            node->size += add_cnt;
    }
    node->meta_lock.writeUnlock();
}

}
