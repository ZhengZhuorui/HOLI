//#include "aex/aex.h"
#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update_node_frequency(node_ptr node) {
    double forget_rate = rapid_pow(this->lambda, this->m_stats.timestamp - node->base_stats.recent_update_timestamp);
    node->base_stats.write_times = node->base_stats.write_times * forget_rate;
    node->base_stats.train_times = node->base_stats.train_times * forget_rate;
    node->base_stats.read_times = node->base_stats.read_times * forget_rate;
    node->base_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update_tree_frequency(){
    double forget_rate = rapid_pow(this->lambda, this->m_stats.timestamp - this->m_stats.recent_update_timestamp);
    this->m_stats.write_times = this->m_stats.write_times * forget_rate;
    this->m_stats.read_times = this->m_stats.read_times * forget_rate;
    this->m_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
}

template<typename _Key, typename _Val, typename traits>
inline double aex_tree<_Key, _Val, traits>::estimate_cost() const {
    /* TODO */
    return 0;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_balance_merge_subtree(inner_node_ptr node){
    // delta cost
    // 1. delta write cost
    // *2. delta node read cost
    // 3. delta all read cost
    if (this->level - node->level > 1)
        return false;
    update_node_frequency(node);
    //double read_pro = 1.0 * read_times / (read_times + this->m_stats.write_times);
    //double read_pro = 1 - (this->m_stats.write_times / this->m_stats.lambda_timestamp);
    
    //double merge_cost = node->m_stats.size * traits::LEARNING_COST / node->write_times;

    double SMO_cost = node_train_pro(node) * traits::LEARNING_COST * node->slot_size;

    double delta_cost = + node_write_pro(node) * node->m_stats.size / 2 \
                        - (1.0 * (node->m_stats.data_node - 1) / this->m_stats.data_node)
                        + SMO_cost;
    
    AEX_FORMAT("balance cost: %.2f", delta_cost);
    if (delta_cost < 0)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_balance_merge_nodes(node_ptr* node_buffer, size_type size){
    // delta cost
    // 1. write cost 
    // 2. merge cost
    // 3. read cost
    double write_times = 0, train_times = 0, delta_cost = 0;
    size_type data_size = 0, data_node_size = 0, slot_size = 0;
    for (size_type i = 0; i < size; ++i){
        if (node_buffer[i]->prop & node_property::LEAF){
            if (!(node_buffer[i]->prop & node_property::ML_NODE)) return false;
        }
        else{
            if (node_buffer[i]->m_stats.rewired_cnt > 0)
                return false;
            double SMO_cost = node_buffer[i]->base_stats.train_times * traits::LEARNING_COST / this->m_stats.timestamp * node_buffer[i]->slot_size;
            delta_cost += SMO_cost;
        }
        update_node_frequency(node_buffer[i]);
        write_times += node_buffer[i]->base_stats.write_times;
        train_times += node_buffer[i]->base_stats.train_times;
        data_size += node_buffer[i]->data_size();
        data_node_size += node_buffer[i]->data_node_size();
    }
    

    double write_pro = 1.0 * write_times / this->m_stats.lambda_timestamp;

    double train_pro = 1.0 * train_times / this->m_stats.lambda_timestamp;

    double read_pro = 1 - (this->m_stats.write_times / this->m_stats.lambda_timestamp);

    delta_cost += train_pro * traits::LEARNING * size;

    AEX_FORMAT("balance cost: %.2f", delta_cost);
    if (delta_cost < 0)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::balance_merge_nodes(node_ptr* node_buffer, size_type size){
    node_ptr ret = nullptr;
    size_type child_size = 0;
    unsigned int level = node_buffer[0]->level;
    for (size_type i = 0; i < size; ++i)
        child_size += node_buffer[i]->size;
    std::vector<key_type> key_buffer(child_size);

    if (node_buffer[0]->prop & LEAF){
        data_node_model m;
        for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
            std::copy(static_cast<data_node_ptr>(node_buffer[i])->key, static_cast<data_node_ptr>(node_buffer[i])->key + node_buffer[i]->size, key_buffer + cnt);
            node_buffer[i]->m_stats.train_times++;
        }
        m.train(key_buffer, child_size);
        if (m.RMSE(key_buffer, child_size) < traits::MAX_ALLOW_ERROR){
            data_node_ptr new_node = node_allocator.allocate_data_node(child_size);
            value_type* data_buffer = node_allocator.allocate_data_buffer(child_size);
            for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
                std::move(static_cast<data_node_ptr>(node_buffer[i])->data, static_cast<data_node_ptr>(node_buffer[i])->data + node_buffer[i]->size, data_buffer + cnt);
                node_buffer[i]->m_stats.train_times++;
            }
            new_node->construct(key_buffer, data_buffer, m);
            ret = new_node;
            node_allocator.deallocate(data_buffer);
        }
    }
    else{
        
        inner_node_model m;
        for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
            copy_to_buffer(node_buffer[i], key_buffer + cnt);
            node_buffer[i]->base.train_times++;
        }
        
        size_type slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (slot_size * this->inner_node_full_ratio[level] < child_size) slot_size <<= 1;
        
        if (check_rewired(key_buffer, child_size, slot_size, m)){
            inner_node_ptr new_node = node_allocator.allocate_inner_node(slot_size);
            ++this->m_stats.inner_node;
            node_ptr* child_buffer = node_allocator.allocate_nodeptr_buffer(child_size);
            for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
                copy_to_buffer(node_buffer[i], child_buffer + cnt);
                node_buffer[i]->m_stats.train_times++;
            }
            new_node->construct(key_buffer, child_buffer, m);
            ret = new_node; 
            node_allocator.deallocate(child_buffer);
        }
    }
    node_allocator.deallocate(key_buffer);
    return ret;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_balance_split(size_type node_size, double node_write_pro, double train_pro, size_type slot_size){
    // delta cost
    // 1. delta write cost
    // 2. delta node read cost
    // 3. delta all read cost
    // 4. average SMO cost
    double node_pro = 1.0 * node_size / this->m_stats.size;
    double read_pro = 1 - (this->m_stats.read_times / this->m_stats.lambda_timestamp);
    double SMO_cost = train_pro * slot_size;
    double delta_cost = - node_write_pro * (node_size - slot_size) / 2 \
                        - read_pro * node_pro \
                        + (1.0 * (node_size / slot_size - 1) / this->m_stats.data_node)
                        + SMO_cost;

    AEX_FORMAT("balance cost: %.2f",  delta_cost);
    if (delta_cost < 0) 
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_balance_split(data_node_ptr node, size_type slot_size){
    // delta cost
    // 1. delta write cost
    // 2. delta node read cost
    // 3. delta all read cost
    // 4. average SMO cost
    AEX_ASSERT(this->m_stats.height - node->level == 0);
    update_node_frequency(node);
    update_tree_frequency();
    return check_balance_split(node->slot_size, node_write_pro(node), node_train_pro(node), slot_size);
    return false;
}

template<typename _Key, typename _Val, typename traits>
typename traits::pos_type aex_tree<_Key, _Val, traits>::check_balance_split_best_slot_size(data_node_ptr node){
    // delta cost
    // 1. delta write cost
    // 2. delta node read cost
    // 3. delta all read cost
    pos_type slot_size = node->slot_size, best_slot_size = node->slot_size;
    double best_delta_cost = 0, delta_cost;
    for (; slot_size >= traits::MIN_DATA_NODE_SLOT_SIZE; slot_size >>= 1){
        delta_cost = - node_write_pro(node) * (node->size - slot_size) / 2 \
                     + (1.0 * (node->size / slot_size) / this->m_stats.data_node);
        if (delta_cost < best_delta_cost){
            best_delta_cost = delta_cost;
            best_slot_size = slot_size;
        }
        else 
            break;
    }

    delta_cost += node_train_pro(node) * traits::LEARNING_COST * node->slot_size;
    AEX_FORMAT("balance cost: %.2f", delta_cost);
    if (delta_cost < 0) 
        return best_slot_size;
    return slot_size;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::balance_merge_subtree(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ parent){
    // check
    update_tree_frequency();
    size_type cnt;

    data_node_ptr new_node = node_allocator.allocate_data_node(node->m_stats.size, true);
    ++this->m_stats.data_node;
    {
        // meta:
        new_node->base_stats = node->base_stats;
        cnt = 0;
        data_node_ptr last_node = static_cast<data_node_ptr>(node->child_ptr[node->last()]);

        for (data_node_ptr i_leaf = static_cast<data_node_ptr>(node->child_ptr[0]); i_leaf != last_node->next; i_leaf = i_leaf->next){
            copy_to_buffer(i_leaf->key, new_node->key + cnt);
            cnt += i_leaf->size;
        }
        new_node->size = cnt;

        bool merge_flag = new_node->train_model();
        if (merge_flag ==false){
            update_node_frequency(node);
            ++node->base_stats.train_times;
            node_allocator.free_node(new_node);
            --this->m_stats.data_node;
            return nullptr;
        }
    }

    // merge
    {
        cnt = 0;
        key_type first_key = node->key_ptr[first_key];
        if (parent != nullptr){
            if (node != parent->child_ptr[0]){
                inner_node_ptr brother = node->next;
                bool flag = true;
                if (isfull(brother)) 
                    flag &= rescale(brother, traits::EXPAND_RATIO);
                if (flag){
                    if (brother->insert(first_key, new_node)){
                        //
                    }
                    else if (rewired(brother, first_key, new_node) && brother->insert_node(first_key, new_node)){
                        //
                    }
                    else flag = false;
                }

                if (flag){
                    data_node_ptr new_node = merge_to_node(node);
                    if (new_node != nullptr){
                        ++this->m_stats.data_node;
                        node_ptr prev_head_leaf = node->child_ptr[0]->prev;
                        node_ptr next_tail_leaf = node->child_ptr[node->last()]->next;
                        if (prev_head_leaf != nullptr)
                            prev_head_leaf->next = next_tail_leaf;
                        if (next_tail_leaf != nullptr)
                            next_tail_leaf->prev = prev_head_leaf;
                        erase_child_node(parent, node);
                    }
                    else{
                        return node;
                    }
                }
            }
        }
        else{
            data_node_ptr new_node = merge_to_node(node);
            if (new_node != nullptr){
                root = new_node;
                this->m_stats.level = 1;
                return new_node;
            }
        }
        this->erase_tree_recursive(node);
    }
}

//template<typename _Key, typename _Val, typename traits>
//inline bool aex_tree<_Key, _Val, traits>::check_insert_balance(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
//    size_type size = left_node->size + right_node->size;
//    update_node_frequency(left_node);
//    update_node_frequency(right_node);
//    update_tree_frequency();
//    size_type node_slot_size = std::max(left_node->slot_size, right_node->slot_size);
//    if (left_node->size + right_node->size > node_slot_size)
//        node_slot_size <<= 1;
//    return check_balance_split(node_slot_size, node_write_pro(left_node) + node_write_pro(right_node), std::min(left_node->slot_size, right_node->slot_size));
//}

//template<typename _Key, typename _Val, typename traits>
//inline bool aex_tree<_Key, _Val, traits>::check_insert_balance(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
//    if (left_node->m_stats.rewired_cnt > 0 || right_node->m_stats.rewired_cnt > 0)
//        return false;
//}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::balance_merge_to_left_node(inner_node_ptr __restrict__ parent, data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    if (check_insert_balance(left_node, right_node)){
        size_type node_slot_size = std::max(left_node->slot_size, right_node->slot_size);
        if (left_node->size + right_node->size > node_slot_size){
            node_slot_size <<= 1;
            data_node_ptr new_node = node_allocator.allocate_data_node(node_slot_size);
            replace_node(left_node, new_node);
            memcpy(new_node->key, left_node->key, left_node->size * sizeof(key_type));
            memcpy(new_node->data, left_node->data, left_node->size * sizeof(value_type));
            memcpy(new_node->key + left_node->size, right_node->key, right_node->size * sizeof(key_type));
            memcpy(new_node->data + left_node->size, right_node->data, right_node->size * sizeof(value_type));
            new_node->size = left_node->size + right_node->size;
            new_node->train_model();
            new_node->next = right_node->next;
            if (right_node->next != nullptr) right_node->next->prev = new_node;
            new_node->base_stats.write_times += right_node->base_stats.write_times;
            new_node->base_stats.train_times += right_node->base_stats.train_times;
            return new_node;
        }
        else{
            if (left_node->slot_size >= right_node->slot_size){
                merge_to_left_leaf(left_node, right_node);
                return left_node;
            }
            else{
                merge_to_right_leaf(left_node, right_node);
                return right_node;
            }
        }
        
    }
    return nullptr;
}

// Unused.
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::inner_node_ptr aex_tree<_Key, _Val, traits>::balance_merge_to_left_node(inner_node_ptr __restrict__ parent, inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    if (left_node->m_stats.rewired_cnt > 0 || right_node->m_stats.rewired_cnt > 0)
        return nullptr;
    inner_node_model m;
    size_type size = left_node->size + right_node->size, slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
    while (slot_size * this->inner_node_full_ratio[left_node->level] < size) slot_size <<= 1;
    key_type* key_buffer = node_allocator.allocate_key_buffer(size);
    copy_to_buffer(left_node, key_buffer);
    copy_to_buffer(right_node, key_buffer + left_node->size);
    if (check_rewired(key_buffer, size, slot_size, m)){
        node_ptr* child_buffer = node_allocator.allocate_nodeptr_buffer(size);
        if (slot_size > left_node->real_slot_size() && slot_size > left_node->real_slot_size()){
            update_node_frequency(left_node);
            update_node_frequency(right_node);
            copy_to_buffer(left_node, child_buffer);
            copy_to_buffer(right_node, child_buffer + left_node->size);
            inner_node_ptr new_node = node_allocator.allocate_key_buffer(slot_size);
            new_node->construct(key_buffer, child_buffer, m);
            replace(left_node, new_node);
            new_node->next = right_node->next;
            if (right_node->next != nullptr) right_node->next->prev = new_node;
            new_node->base_stats.write_times += right_node->base_stats.write_times;
            new_node->base_stats.train_times += right_node->base_stats.train_times;
            node_allocator.deallocate(child_buffer);
            return new_node;
        }
        else if (slot_size <= left_node->real_slot_size()){
            left_node->construct(key_buffer, child_buffer, size);
            left_node->next = right_node->next;
            if (right_node->next != nullptr) right_node->next->prev = left_node;
            left_node->base_stats.write_times += right_node->base_stats.write_times;
            left_node->base_stats.train_times += right_node->base_stats.train_times;
            return left_node;
        }
        else{
            right_node->construct(key_buffer, child_buffer, size);
            right_node->prev = right_node->next;
            if (left_node->prev != nullptr) left_node->prev->next = right_node;
            right_node->base_stats.write_times += left_node->base_stats.write_times;
            left_node->base_stats.train_times += right_node->base_stats.train_times;
            return right_node;
        }
    }
    else{
        node_allocator.deallocate(key_buffer);
        return nullptr;
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::balance_split(const node_ptr* stack, const int top, data_node_ptr node, pos_type slot_size){
    std::vector<key_type> key_buffer;
    std::vector<node_ptr> node_buffer;

    iterator res_iter;
    //double min_cost = 0;
    pos_type leaf_size = slot_size * traits::DATA_NODE_FEW_RATIO;
    for (pos_type i_offset = 0; i_offset < node->size; i_offset += leaf_size){
        data_node_ptr new_data_node = node_allocator.allocate_data_node(slot_size);
        ++this->m_stats.data_node;
        if (node->size - i_offset < slot_size) leaf_size = node->size - i_offset;
        new_data_node->base_stats.write_times = 1.0 * node->base_stats.write_times * leaf_size / node->size;
        new_data_node->base_stats.train_times = 1.0 * node->base_stats.train_times * leaf_size / node->size;
        new_data_node->construct(node->key + i_offset, node->data + i_offset, leaf_size);
    }
    if (top == 2){
        this->build_tree(key_buffer, node_buffer);
    }
    else{
        insert_many(stack, top, key_buffer, node_buffer);
    }

    node_allocator.free_node(node);
    --this->m_stats.data_node;
}

}