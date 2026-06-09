#pragma once

namespace aex{

#define XLHD \
    pos = node_copy->prev_item(pos); \
    next_pos = node_copy->next_item(pos + 1); \
    if (!node_copy->try_lock(pos, next_pos)) goto insert_start; \
    if (node_copy->next_item(pos + 1) != next_pos) { node_copy->unlock(pos, next_pos); goto insert_start; } \
    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); \
    if (need_restart) { node_copy->unlock(pos, next_pos); goto insert_start;} 

#define XLDD \
    node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart); \
    if (need_restart) goto insert_start; \
    child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); \
    if (need_restart) { XU(node); goto insert_start; } \

#define CHECK_TOP_NODE_CON() \
    { \
        if (top_node != nullptr && top_node != node){ \
            top_node_copy = top_node->copy; \
            top_node->node_lock.checkOrRestart(top_node_version, need_restart); \
            if (need_restart) goto insert_start; \
            split_pos = top_node_copy->predict(split_key); \
            if (split_pos < top_node_copy->slot_size && !top_node_copy->is_occupied(split_pos)){ \
                top_flag = true; \
                node = top_node; \
                node_version = top_node_version; \
                node_copy = top_node_copy; \
                pos = top_node_copy->prev_item(split_pos); \
            } \
            else \
                top_node = nullptr; \
        }\
    }


template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con(const key_type key, const value_type &value){
    bool tail, top_flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child;
    slot_type pos, split_pos, next_pos;
    version_type node_version, child_version, top_node_version;
    //hash_node node_copy, top_node_copy;
    hash_node_ptr node_copy, top_node_copy;
    int restart_count = 0;
insert_start:
    AEX_SGL_ASSERT(restart_count == 0);
    //AEX_ASSERT(restart_count < 100000000);
    if (restart_count > 0)
        _yield(restart_count);
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
            if (d_n(node)->is_train == false && d_n(node)->level > 1){
                if (!expand(d_n(node)))
                    split_root(d_n(node));
            }
            else
                split_root(d_n(node));
        }
        node_version = node->node_lock.downgradeLock(); // DL(node) | node S
    }

    while (true){
        if (node->type == NodeType::HashNode){
            node_copy = h_n(node)->copy;
            node->node_lock.checkOrRestart(node_version, need_restart); // node S
            if (need_restart) goto insert_start;
            child = find_insert(node_copy, key, pos, need_restart);
            if (need_restart) goto insert_start;
        }
        else{
            child = find_insert(d_n(node), key, pos); 
            node->node_lock.checkOrRestart(node_version, need_restart);
            if (need_restart) goto insert_start;
        }
        child_version = child->node_lock.readLockOrRestart(need_restart);//SL(child); | node S, child S
        if (need_restart) goto insert_start;
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
            if (top_node != nullptr) top_node = nullptr;
            if (node->type == NodeType::HashNode){
                top_node = h_n(node);
                top_node_version = node_version;
            }
        }
        if (child->type == NodeType::LeafNode){
            if (l_n(child)->next_min_key < key)
                goto insert_start;
            
            if (isfull(l_n(child))){
                top_flag = false;
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
                child->node_lock.checkOrRestart(child_version, need_restart); // node S, child S
                if (need_restart) goto insert_start;
                CHECK_TOP_NODE_CON();
                AEX_ASSERT(need_restart == false);
                data_node_ptr new_node;
                if (node->type == NodeType::HashNode){
                    split_pos = node_copy->predict(split_key);
                    XLHD
                    if (split_pos < node_copy->slot_size && !node_copy->is_occupied(split_pos)){
                        new_node = new_and_split(l_n(child));
                        insert_no_collision(node_copy, split_pos, next_pos, split_key, new_node);
                        if (h_n(node)->tail_node == child) h_n(node)->tail_node = new_node;
                        h_n(node)->add_size();
                    }
                    else{
                        if (top_flag){ XU(child); node_copy->unlock(pos, next_pos); goto insert_start; } // node S
                        new_node = new_and_split(l_n(child)); // node S, node[split_pos] X, child X, new_node X
                        insert_collision(node_copy, pos, next_pos, split_key, new_node); 
                        if (h_n(node)->tail_node == child) h_n(node)->tail_node = tail_node(h_n(node));
                    }
                    node_copy->unlock(pos, next_pos); // node S, child X, new_node X
                }
                else{
                    XLDD
                    new_node = new_and_split(l_n(child)); // node X, child X, new_node X
                    insert(d_n(node), split_key, new_node);
                    XU(node); // #
                }
                if (key < new_node->key[0])
                    l_n(child)->insert(key, value);
                else
                    new_node->insert(key, value);
                XU(child); XU(new_node); // node S
                AEX_SGL_ASSERT(check_unlock(child));
                AEX_SGL_ASSERT(check_unlock(new_node));
                AEX_SGL_ASSERT(check_unlock(node));
            }
            else{
                child->node_lock.upgradeToWriteLockOrRestart(child_version, need_restart); // UL(child) | node S, child X
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
                dense_node_ptr new_node;
                bool rebuild_flag = false;
                AEX_ASSERT(child->type == NodeType::DenseNode);
                split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                top_flag = false;
                CHECK_TOP_NODE_CON();

                if (node->type == NodeType::HashNode){
                    split_pos = node_copy->predict(split_key);  
                    if (split_pos < node_copy->slot_size && !node_copy->is_occupied(split_pos)){
                        XLHD
                        if (split_pos < node_copy->slot_size && node_copy->is_occupied(split_pos)){
                            node_copy->unlock(pos, next_pos); XU(child); goto insert_start;
                        }
                        new_node = new_and_split(d_n(child));
                        insert_no_collision(node_copy, split_pos, next_pos, split_key, new_node);
                        h_n(node)->add_size();
                        node_copy->unlock(pos, next_pos); // node S, 
                    }
                    else {
                        if (top_flag) goto insert_start;  // node S
                        if (d_n(child)->is_train == false && d_n(child)->level > 1){
                            TUL(child, child_version, need_restart);  // UL(child) | node S, child X
                            if (need_restart) goto insert_start;
                            rebuild_flag = expand(d_n(child));
                            child_version = child->node_lock.downgradeLock(); // node S, child S
                        }
                        if (!rebuild_flag){
                            XLHD
                            new_node = new_and_split(d_n(child)); // node S, child S, new_node X
                            insert_collision(node_copy, pos, next_pos, split_key, new_node); // node S, node[split_pos] X, child X, new_node X
                            node_copy->unlock(pos, next_pos);// node S, child X, new_node X
                        }
                    }
                }
                else{
                    if (d_n(child)->is_train == false && d_n(child)->level > 1){
                        TUL(d_n(child), child_version, need_restart);  // UL(child) | node S, child X
                        if (need_restart) goto insert_start;     
                        rebuild_flag = expand(d_n(child));
                        child_version = child->node_lock.downgradeLock(); // node S, child S
                    }
                    if (!rebuild_flag){
                        XLDD 
                        new_node = new_and_split(d_n(child));
                        insert(d_n(node), split_key, new_node);
                        XU(node); // node S, child X, new_node X
                    }
                }

                if (!rebuild_flag){
                    AEX_ASSERT(new_node->key_ptr[0] == split_key);
                    if (key >= split_key){
                        XU(child); // node S, new_node X
                        child_version = new_node->node_lock.downgradeLock(); // DL(new_node) | node S, new_node S
                        child = new_node; // child S
                    }
                    else{
                        top_node = nullptr;
                        XU(new_node);  // node S, child X
                        child_version = child->node_lock.downgradeLock(); // DL(child) | node S, child S
                    }
                    AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
                    AEX_SGL_ASSERT(check_unlock(new_node));
                }
                AEX_SGL_ASSERT(check_unlock(child));
                AEX_SGL_ASSERT(check_unlock(node));
            }
        }
        // require: child S
        //if (top_node != nullptr && top_node != node){
        //    top_node->node_lock.checkOrRestart(top_node_version, need_restart);
        //    if (need_restart) goto insert_start;
        //}
        node = i_n(child);
        node_version = child_version; // node S
    }
    AEX_ASSERT(0 == 1);
    return true;
}


#undef XLHD
#undef XLDD
#undef CHECK_TOP_NODE
}
