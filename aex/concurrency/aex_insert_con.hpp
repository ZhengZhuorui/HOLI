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
    version_type node_version, child_version, top_node_version, tail_version;
    //unsigned char node_copy_data[128], top_node_copy_data[128];
    //static_assert(sizeof(hash_node) < 128, "hash node mush lower than 128");
    //hash_node &node_copy = ()
    hash_node node_copy, top_node_copy;
    int restart_count = 0;

insert_start:
    if (restart_count > 0){
        yield(restart_count);
    }
    ++restart_count;
    bool need_restart = false;
    top_node = nullptr;
    node = root;
    node_version = node->node_lock.readLockOrRestart(need_restart); //SL(node)
    if (need_restart) goto insert_start;
    if (isfull(node)){
        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node)
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
        node_version = node->node_lock.downgradeLock(); // DL(node)
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
        
        //AEX_PRINT("node=" << node << ", node->type=" << to_string(node->type) << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        tail = (node->type == NodeType::HashNode) ? (last_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild)
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                //AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", slot_size=" << node->slot_size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
                node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node)
                if (need_restart) goto insert_start;
                rebuild(node);
                node_version = node->node_lock.downgradeLock(); // DL(node)
            }
        if (!tail){
            if (top_node != nullptr){
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode){
                top_node = h_n(node);
                top_node_version = node_version;
                top_node_copy = *top_node;
            }
        }
        if (child->type == NodeType::LeafNode){
            if (!traits::AllowMultiKey && l_n(child)->find(key) > child->size){
                child->node_lock.readUnlockOrRestart(child_version, need_restart); // SU(child)
                if (need_restart) goto insert_start;
                return false;
            }
            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                child->node_lock.checkOrRestart(child_version, need_restart);
                if (need_restart) goto insert_start;
                if (top_node != nullptr && top_node != node){
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
                    if (!node_copy.is_occupied(split_pos) && split_pos < node_copy.slot_size){
                        insert_data_node(l_n(child), new_node, key, value);
                        insert_no_collision(&node_copy, split_pos, split_key, new_node);
                    }
                    else{
                        if (top_flag){ XU(child); goto insert_start; }
                        insert_data_node(l_n(child), new_node, key, value);
                        insert_collision(&node_copy, pos, split_key, new_node);
                    }
                    
                    node->node_lock.readUnlockOrRestart(node_version, need_restart); // SU(node)
                    if (need_restart){
                        XU(child);
                        free_node_helper(new_node);
                        goto insert_start;
                    }
                    complete(l_n(child), new_node);
                    XU(child); XU(new_node);
                }
                else{
                    node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); // UL(node)
                    if (need_restart) goto insert_start;
                    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                    if (need_restart) goto insert_start;
                    insert_data_node(l_n(child), new_node, key, value);
                    insert(d_n(node), split_key, new_node);
                    complete(l_n(child), new_node);
                    XU(node); XU(child); XU(new_node);
                }
            }
            else{
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                if (need_restart) goto insert_start; 
                l_n(child)->insert(key, value);
                XU(child);
            }
            return true;
        }
        
        if (isfull(child)){
            if (child->type == NodeType::HashNode){
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child)
                if (need_restart) goto insert_start; 
                expand(h_n(child));
                child_version = child->node_lock.downgradeLock(); // DL(child)
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
                    child_version = child->node_lock.downgradeLock(); // DL(child)
                }
                if (!flag){
                    split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                    child->node_lock.checkOrRestart(child_version, need_restart); // check_lock_shared(child)
                    if (need_restart) goto insert_start; 
                    flag = (key >= split_key);
                    top_flag = false;
                    if (top_node != nullptr && top_node != node){
                        top_node->node_lock.checkOrRestart(top_node_version, need_restart); // check_lock_shared(top_node)
                        if (need_restart) goto insert_start;
                        split_pos = top_node_copy.predict(split_key);
                        if (!top_node_copy.is_occupied(split_pos) && split_pos < top_node_copy.slot_size){
                            top_flag = true;
                            node = top_node;
                            node_version = top_node_version;
                            node_copy = top_node_copy;
                        }
                        else if (!flag)
                            top_node = nullptr;
                    }
                    dense_node_ptr new_node = Allocator::allocate_dense_node();
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
                        if (!node_copy.is_occupied(split_pos) && split_pos < node_copy.slot_size){
                            tail_leaf = find_tail_leaf(d_n(child)->child_ptr[traits::DENSE_NODE_SLOT_SIZE - 1], tail_version);
                            if (tail_leaf == nullptr){ XU(child); goto insert_start; }
                            tail_leaf->node_lock.upgradeToWriteLockOrRestart(tail_version, need_restart); // UL(tail_leaf)
                            if (need_restart) goto insert_start;
                            split(d_n(child), new_node);
                            insert_no_collision(&node_copy, split_pos, split_key, new_node);
                        }
                        else {
                            split(d_n(child), new_node);
                            insert_collision(&node_copy, pos, split_key, new_node);
                        }
                        node->node_lock.readUnlockOrRestart(node_version, need_restart);
                        if (need_restart){
                            child->size = traits::DATA_NODE_SLOT_SIZE;
                            free_node_helper(new_node);
                            XU(child);
                            goto insert_start;
                        }
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        if (flag){
                            XU(child);
                            child_version = new_node->node_lock.downgradeLock();
                            child = new_node;
                        }
                        else{
                            XU(new_node);
                            child_version = child->node_lock.downgradeLock();
                        }                        
                    }
                    else{
                        AEX_ASSERT(node->type == NodeType::DenseNode);
                        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
                        if (need_restart) goto insert_start;
                        child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart);
                        if (need_restart) { XU(node); goto insert_start;}
                        new_node = Allocator::allocate_dense_node();
                        new_node->node_lock.writeLockOrRestart(need_restart);
                        AEX_ASSERT(need_restart == false);
                        split(d_n(child), new_node);
                        insert(d_n(node), split_key, new_node);
                        complete(d_n(child), new_node);
                        AEX_ASSERT(new_node->key_ptr[0] == split_key);
                        XU(node); XU(child); XU(new_node); 
                        if (flag){
                            XU(child);
                            child_version = new_node->node_lock.downgradeLock();
                            child = new_node;
                        }
                        else{
                            XU(new_node);
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

}
