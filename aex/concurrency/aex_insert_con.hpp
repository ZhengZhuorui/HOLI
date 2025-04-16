#pragma once

namespace aex
{
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con(const key_type key, const value_type &value){
    bool tail, flag, top_flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child, new_tail_node;
    slot_type pos, split_pos;
    version_type node_version, child_version, top_node_version;
    //hash_node node_copy, top_node_copy;
    hash_node_ptr node_copy, top_node_copy;
    int restart_count = 0;
insert_start:
    AEX_SGL_ASSERT(restart_count == 0);
    AEX_ASSERT(restart_count < 100000000);
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
        node_version = node->node_lock.downgradeLock(); // DL(node) | node S
    }

    while (true){
        if (node->type == NodeType::HashNode){
            //node_copy = *h_n(node);
            node_copy = h_n(node)->copy;
            node->node_lock.checkOrRestart(node_version, need_restart); // node S
            if (need_restart) goto insert_start;
            //child = find_insert(&node_copy, key, pos); // node S
            //child = find_insert(&node_copy, key, pos, need_restart);
            //AEX_SGL_ASSERT(node_copy->predict(key) != node->predicy(key));
            child = find_insert(node_copy, key, pos, need_restart);
            //child = find_insert(&node_copy, key, pos, need_restart);
            if (need_restart) goto insert_start;
        }
        else{
            child = find_insert(d_n(node), key, pos); 
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto insert_start;
        }
        child_version = child->node_lock.readLockOrRestart(need_restart);//SL(child); | node S, child S
        if (need_restart) goto insert_start;
        AEX_DEBUG_BLOCK({if (restart_count > 10000) AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size << ", child->lock=" << child->node_lock.get_version_number() << ", child_version=" << child_version);});
        //AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        tail = (node->type == NodeType::HashNode) ? (h_n(node)->tail_node == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild){
            size_type child_size = (node->type == NodeType::HashNode) ? child->size : child->size;
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                TUL(h_n(node), node_version, need_restart);  // UL(node) | node X
                if (need_restart) goto insert_start;
                rebuild(node);
                if (node->type == NodeType::HashNode)
                    //node_copy = node->copy;
                    node_copy = *h_n(node);
                node_version = node->downgradeLock(); // node S
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
            if (l_n(child)->next_min_key < key){
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
                    top_node_copy = top_node->copy;
                    //top_node_copy = *h_n(top_node);

                    top_node->node_lock.checkOrRestart(top_node_version, need_restart); //check_lock_shared(top_node)
                    if (need_restart) goto insert_start;
                    split_pos = top_node_copy->predict(split_key);
                    if (split_pos < top_node_copy->slot_size && !top_node_copy->is_occupied(split_pos)){
                        top_flag = true;
                        node = top_node;
                        node_version = top_node_version;
                        node_copy = top_node_copy;
                    }
                    else
                        top_node = nullptr;
                }
                AEX_ASSERT(need_restart == false);
                data_node_ptr new_node;
                if (node->type == NodeType::HashNode){
                    split_pos = node_copy->predict(split_key);
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                    if (need_restart) goto insert_start;
                    AEX_SGL_ASSERT(node_copy->lock_array[pos2slot(split_pos)].is_lock() == false);
                    if (!node_copy->lock_array[pos2slot(split_pos)].try_lock()) { XU(child); goto insert_start; } // XL(S, split_pos) | node S, node[split_pos] X, child X,
                    if (split_pos < node_copy->slot_size && !node_copy->is_occupied(split_pos)){
                        new_node = new_and_split(l_n(child)); // node S, node[split_pos] X, child X, new_node X
                        insert_no_collision(node_copy, split_pos, split_key, new_node);
                        //insert_no_collision(&node_copy, split_pos, split_key, new_node);
                        if (h_n(node)->tail_node == child) 
                            h_n(node)->tail_node = new_node;
                        h_n(node)->add_size();
                    }
                    else{
                        AEX_ASSERT(top_flag == false); // node S, node[split_pos] X, child X
                        if (top_flag){ XU(child); node_copy->lock_array[pos2slot(split_pos)].unlock(); goto insert_start; } // node S
                        pos = node_copy->prev_item(pos);
                        new_node = new_and_split(l_n(child)); // node S, node[split_pos] X, child X, new_node X
                        new_tail_node = insert_collision(node_copy, pos, split_key, new_node); 
                        //new_tail_node = insert_collision(&node_copy, pos, split_key, new_node); 
                        AEX_SGL_ASSERT(!new_tail_node->node_lock.isLocked());
                        if (h_n(node)->tail_node == child)
                            h_n(node)->tail_node = new_tail_node;
                    }
                    node_copy->lock_array[pos2slot(split_pos)].unlock(); // node S, child X, new_node X

                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XU(child); XU(new_node); // node S
                }
                else{
                    AEX_ASSERT(node->type == NodeType::DenseNode);
                    node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node) | node X
                    if (need_restart) goto insert_start;
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node X, child X
                    if (need_restart) { XU(node); goto insert_start; }
                    new_node = new_and_split(l_n(child)); // node X, child X, new_node X
                    insert(d_n(node), split_key, new_node);
                    if (key < new_node->key[0])
                        l_n(child)->insert(key, value);
                    else
                        new_node->insert(key, value);
                    XU(child); XU(new_node); XU(node); // #
                }
                AEX_SGL_ASSERT(check_unlock(child));
                AEX_SGL_ASSERT(check_unlock(new_node));
                AEX_SGL_ASSERT(check_unlock(node));
            }
            else{
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                AEX_DEBUG_BLOCK({if (restart_count > 10000)AEX_PRINT("need_restart=" << need_restart);});
                if (need_restart) goto insert_start; 
                l_n(child)->insert(key, value);
                XU(child); // node S
            }
            return true;
        }

        if (isfull(i_n(child))){
            if (child->type == NodeType::HashNode){
                TUL(h_n(child), child_version, need_restart);  // UL(child) | node S, child X
                if (need_restart) goto insert_start; 
                expand(h_n(child));
                child_version = child->node_lock.downgradeLock(); // node S, child S
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
                    child_version = child->node_lock.downgradeLock(); // node S, child S
                }
                if (!flag){ // node S, child S
                    split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                    child->node_lock.checkOrRestart(child_version, need_restart); // check_lock_shared(child)
                    if (need_restart) goto insert_start; 
                    flag = (key >= split_key);
                    top_flag = false;
                    if (top_node != nullptr && top_node != node){
                        //top_node_copy = *h_n(top_node);
                        top_node_copy = top_node->copy;
                        top_node->node_lock.checkOrRestart(top_node_version, need_restart); // check_lock_shared(top_node)
                        if (need_restart) goto insert_start;
                        split_pos = top_node_copy->predict(split_key);
                        if (split_pos < top_node_copy->slot_size && !top_node_copy->is_occupied(split_pos)){
                            top_flag = true;
                            node = top_node;
                            node_version = top_node_version;
                            node_copy = top_node_copy;
                        }
                        else if (!flag)
                            top_node = nullptr;
                    }
                    AEX_ASSERT(need_restart == false);
                    dense_node_ptr new_node;
                    if (node->type == NodeType::HashNode){ // node S, child S
                        if (!top_flag){
                            node->node_lock.checkOrRestart(node_version, need_restart); // check_lock_shared(node)
                            if (need_restart) goto insert_start;
                            split_pos = node_copy->predict(split_key);
                        }
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
                        if (need_restart) goto insert_start;
                        if (!node_copy->lock_array[pos2slot(split_pos)].try_lock()){ XU(child); goto insert_start; } // XL(node[split_pos]) | node S, node[split_pos] X, child X
                        if (split_pos < node_copy->slot_size && !node_copy->is_occupied(split_pos)){ // node S, node[split_pos] X, child X
                            new_node = new_and_split(d_n(child)); // node S, node[split_pos] X, child X, new_node X
                            insert_no_collision(node_copy, split_pos, split_key, new_node);
                            //insert_no_collision(&node_copy, split_pos, split_key, new_node);
                            if (h_n(node)->tail_node == child)
                                h_n(node)->tail_node = new_node;
                            h_n(node)->add_size();
                        }// node S, node[split_pos] X, child X, new_node X
                        else { // node S, node[split_pos] X, child X
                            //AEX_ASSERT(top_flag == false);
                            if (top_flag){ XU(child); node_copy->lock_array[pos2slot(split_pos)].unlock(); goto insert_start; } // node S
                            pos = node_copy->prev_item(pos);
                            new_node = new_and_split(d_n(child)); // node S, child X, new_node X
                            new_tail_node = insert_collision(node_copy, pos, split_key, new_node);
                            //new_tail_node = insert_collision(&node_copy, pos, split_key, new_node);
                            AEX_SGL_ASSERT(!new_tail_node->node_lock.isLocked());
                            if (h_n(node)->tail_node == child)
                                h_n(node)->tail_node = new_tail_node;
                            if (!flag) top_node = nullptr;
                        } // node S, node[split_pos] X, child X, new_node X
                        node_copy->lock_array[pos2slot(split_pos)].unlock(); // node S, child X, new_node X
                        
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        if (flag){
                            XU(child); // node S, new_node X
                            child_version = new_node->node_lock.downgradeLock(); // DL(new_node) | node S, new_node S
                            child = new_node; // child S
                        }
                        else{
                            XU(new_node);  // node S, child X
                            child_version = child->node_lock.downgradeLock(); // DL(child) | node S, child S
                        }                        
                    }
                    else{
                        AEX_ASSERT(node->type == NodeType::DenseNode);
                        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);  // UL(node)
                        if (need_restart) goto insert_start; // node X
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                        if (need_restart) { XU(node); goto insert_start;} // node X, child X
                        new_node = new_and_split(d_n(child)); // node X, child X, new_node X
                        insert(d_n(node), split_key, new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        XU(node); // child X, new_node X
                        if (flag){
                            XU(child); // new_node X
                            child_version = new_node->node_lock.downgradeLock(); // DL(new_node) | new_node S
                            child = new_node; // child S
                        }
                        else{
                            XU(new_node); // child X
                            top_node = nullptr;
                            child_version = child->node_lock.downgradeLock(); // DL(child) | child S
                        }
                    }
                    AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
                    AEX_SGL_ASSERT(check_unlock(child));
                    AEX_SGL_ASSERT(check_unlock(new_node));
                    AEX_SGL_ASSERT(check_unlock(node));
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

}
