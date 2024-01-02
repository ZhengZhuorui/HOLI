#pragma once
namespace aex{
template<typename _Key, typename _Val, typename traits>
aex_tree_con<_Key, _Val, traits>::data_node_ptr aex_tree_con<_Key, _Val, traits>::find_leaf_con(const key_type &key){
    node_ptr node, child_node;
    bool flag = false;
    while (!flag){
        flag = true;
        node = root;
        if (!child_node->node_mutex->try_lock_shared())
            continue;
        while (!IS_LEAF_NODE(node)){
            slot_type pos = static_cast<inner_node_ptr>(node)->find(key);
            child_node = static_cast<inner_node_ptr>(node)->child_ptr[pos];
            if (!child_node->node_mutex->try_lock_shared()){
                node->node_mutex->unlock_shared();
                flag = false;
                break;
            }
            node->node_mutex->unlock_shared();
            node = child_node;
        }
    }
    return static_cast<data_node_ptr>(node);
}

template<typename _Key, typename _Val, typename traits>
aex_tree_con<_Key, _Val, traits>::data_node_ptr aex_tree_con<_Key, _Val, traits>::find_leaf_lock_con(const key_type &key){
node_ptr node, child_node;
    bool flag = false;
    while (!flag){
        flag = true;
        node = root;
        if (!child_node->node_mutex->try_lock_shared())
            continue;
        while (!IS_LEAF_NODE(node)){
            slot_type pos = static_cast<inner_node_ptr>(node)->find(key);
            child_node = static_cast<inner_node_ptr>(node)->child_ptr[pos];
            if (IS_LEAF_NODE(child_node)){
                if (!child_node->node_mutex->try_lock()){
                    node->node_mutex->unlock_shared();
                    flag = false;
                    break;
                }
            }
            else{
                if (!child_node->node_mutex->try_lock_shared()){
                    node->node_mutex->unlock_shared();
                    flag = false;
                    break;
                }
            }
            node->node_mutex->unlock_shared();
            node = child_node;
        }
    }
    return static_cast<data_node_ptr>(node);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree_con<_Key, _Val, traits>::range_scan(const key_type &L, const key_type &R, std::vector<std::pair<key_type, value_type>>& answer){
    std::shared_lock<std::shared_mutex> lock(this->tree_mutex);
    data_node_ptr last_node = find_leaf_con(R);
    data_node_ptr first_node = last_node;
    while(true){
        if (first_node->prev == nullptr) break;
        first_node->prev->node_mutex->lock_shared();
        if (first_node->prev->key[first_node->size - 1] < L){
            first_node->prev->unlock_shared();
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
        iter_node->node_mutex->unlock_shared();
}

}