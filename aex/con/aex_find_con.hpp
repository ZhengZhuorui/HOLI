#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
inline typename aex_tree_con<_Key, _Val, traits>::data_node_ptr aex_tree_con<_Key, _Val, traits>::find_leaf_con(const key_type &key){
    node_ptr node, child_node;
    bool flag = false;
    slot_type pos;
    while (!flag){
        flag = true;
        {
            std::shared_lock(this->tree_mutex);
            node = this->base_tree::root;
            if (!TSL(node))
                continue;
        }
            
        while (!IS_LEAF_NODE(node)){
            child_node = static_cast<inner_node_ptr>(node)->find(key);
            SU(node);
            if (child_node == nullptr || !TSL(child_node)) break;
            node = child_node;
        }
    }
    return static_cast<data_node_ptr>(node);
}

//template<typename _Key, typename _Val, typename traits>
//inline typename aex_tree_con<_Key, _Val, traits>::data_node_ptr aex_tree_con<_Key, _Val, traits>::find_leaf_con(const key_type &key){
//    node_ptr node, child_node;
//    bool flag = false;
//    slot_type pos;
//    
//    {
//        std::shared_lock(this->tree_mutex);
//        node = this->base_tree::root;
//        node->lock_shared();
//    }
//    while (!IS_LEAF_NODE(node)){
//        child_node = static_cast<inner_node_ptr>(node)->find(key, pos);
//        child_node->lock_shared();
//        node->unlock_shared();
//        node = child_node;
//    }
//
//    return static_cast<data_node_ptr>(node);
//}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree_con<_Key, _Val, traits>::node_ptr aex_tree_con<_Key, _Val, traits>::find_node_lock_con(const key_type &key, const int level, inner_node_ptr* stack, int &top){
    node_ptr node, child_node;
    slot_type pos;
    while (true){
        {
            std::shared_lock(this->tree_mutex);
            node = this->base_tree::root;
            if (!TSL(node))
                continue;
        }
        top = 0;
        while (node->level != level){
            stack[top++] = node;
            child_node  = static_cast<inner_node_ptr>(node)->find(key);
            SU(node);
            if (child_node == nullptr){
                break;
            }
            if (node->level == level){
                if (!TXL(child_node))
                    break;
                return child_node;
            }
            else{
                if (!TSL(child_node))
                    break;
                node = child_node;
            }
        }
    }
    AEX_ASSERT(0 == 1);
    return nullptr;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree_con<_Key, _Val, traits>::data_node_ptr aex_tree_con<_Key, _Val, traits>::find_leaf_lock_con(const key_type &key, inner_node_ptr* stack, int &top){
    node_ptr node, child_node;
    slot_type pos;
    while (true){
        {
            std::shared_lock(this->tree_mutex);
            node = this->base_tree::root;
            if (!TSL(node))
                continue;
        }
        top = 0;
        while (!IS_LEAF_NODE(node)){
            stack[top++] = node;
            child_node  = static_cast<inner_node_ptr>(node)->find(key);
            SU(node);
            if (child_node == nullptr){
                break;
            }
            if (!IS_LEAF_NODE(node)){
                if (!TSL(child_node))
                    break;
                return child_node;
            }
            else{
                if (!TXL(child_node))
                    break;
                node = child_node;
            }
        }
    }
    AEX_ASSERT(0 == 1);
    return nullptr;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree_con<_Key, _Val, traits>::range_scan(const key_type &L, const key_type &R, std::vector<std::pair<key_type, value_type>>& answer){
    std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
    data_node_ptr last_node = find_leaf_con(R);
    data_node_ptr first_node = last_node;
    while(true){
        if (first_node->prev == nullptr) break;
        SL(first_node->prev);
        if (first_node->prev->key[first_node->size - 1] < L){
            SU(first_node->prev);
            break;
        }
    };
    slot_type pos = first_node->find_lower_pos(L);
    for (slot_type i = pos; i < first_node->size; ++i)
        answer.push_back(std::make_pair(first_node->key[i], first_node->data[i]));
    for (data_node_ptr iter_node = first_node->next; iter_node != last_node; iter_node = iter_node->next)
        for (slot_type i = 0; i < iter_node->size; ++i)
            answer.push_back(std::make_pair(iter_node->key[i], iter_node->data[i]));
    
    for (slot_type i = 0; i < last_node->size; ++i)
    if (last_node->key[i] <= R)
        answer.push_back(std::make_pair(last_node->key[i], last_node->data[i]));
    
    for (data_node_ptr iter_node = first_node->next; iter_node != last_node->next; iter_node = iter_node->next)
        SU(iter_node);
}

}