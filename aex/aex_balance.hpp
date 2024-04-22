//#include "aex/aex.h"
#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::update_node_list_frequency(dynamic_node_ptr node, node_ptr* node_list, slot_type n){
    unsigned long long recent_udpate_timestamp = node->balance_stats.get_recent_update_timestamp();
    double train_times = node->balance_stats.get_SMO_times();
    double write_times = node->balance_stats.get_write_times();
    for (slot_type i = 0; i < n; ++i){
        ((dynamic_node_ptr)node_list[i])->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                                        train_times * (1.0 * node_list[i]->size / node->size), 
                                                        write_times * (1.0 * node_list[i]->size / node->size));
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_merge(inner_node_ptr parent, key_type &new_key, node_ptr new_node){
    // check nodes can merge
    // AllowInsertBalance need open
    AEX_ASSERT(parent != nullptr);
    if constexpr (traits::AllowInsertBalance)
        return false;
    // child must not inner node
    if (parent->level <= 1 || !IS_ML_NODE(parent))
        return false;
    if (!CAN_MERGED(new_node))
        return false;
    
    slot_type pos = parent->find_lower_pos(new_key), pred_pos = parent->predict(new_key);
    slot_type last_pos = pos - 1;
    bool ret = true;
    if (pos >= parent->slot_size - 1)
        return false;
    
    AEX_ASSERT(bitmap_impl::at(parent->bitmap_ptr, pos) != 0 || pred_pos - pos >= traits::ERROR_BOUND);
    
    slot_type node_merge_size = 0;
    key_type key_merge_buf[traits::ERROR_BOUND << 1];
    node_ptr* node_merge_buf[traits::ERROR_BOUND << 1];
    auto insert_merge_node = [&](key_type key, node_ptr node){key_merge_buf[node_merge_size] = key; node_merge_buf[node_merge_size] = node; node_merge_size++;};
    for (slot_type i = pred_pos; i < pos; ++i)
    if (bitmap_impl::at(parent->bitmap_ptr, i) != 0){
        if  (!CAN_MERGED(parent->child_ptr[pos]))
            return false;
        else{
            insert_merge_node(parent->key_ptr[i], parent->child_ptr[i]);
        }
    }
    
    insert_merge_node(new_key, new_node);
    slot_type upper_bound = std::min(pred_pos + traits::ERROR_BOUND, parent->slot_size - 1);
    for (slot_type i = pos; i < upper_bound; ++i){
        if (bitmap_impl::at(parent->bitmap_ptr, i) != 0){
            if (parent->predict(parent->key_ptr[i]) != pred_pos)
                break;
            if (!CAN_MERGED(parent->child_ptr[pos]))
                return false;
            else{
                last_pos = i;
                insert_merge_node(parent->key_ptr[i], parent->child_ptr[i]);
            }
        }
    }
    slot_type last_key = key_merge_buf[node_merge_size - 1];
    size_type tot_size = 0;
    double SMO_times = 0, SMO_cost;

    {
        // check delta_cost < 0
        // delta cost
        // 1. - read cost
        // 2. SMO cost
        //   2.1 - child SMO cost
        //   2.2 + merged node SMO cost
        //   2.3 - parent delta SMO probability this times
        //   2.4 + parent delta SMO probability in future

        double lambda_timestamp = this->balance_stats.get_lambda_timestamp();
        unsigned long long timestamp = this->balance_stats.get_timestamp();
        parent->balance_stats.update_frequency(timestamp);

        for_each(node_merge_buf, node_merge_buf + node_merge_size, [&](node_ptr *node){
            node->balance_stats.update_frequency(timestamp);
            SMO_times += node->balance_stats.get_SMO_times();
            tot_size += node->size;
            SMO_cost -= 1.0 * node->balance_stats.get_SMO_times() / lambda_timestamp * node->size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR;// -
        });


        if (tot_size > traits::MAX_DATA_NODE_SLOT_SIZE)
            return false;

        double read_cost = -1.0 * (1 / this->m_stats.level_node[parent->level]) * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR; // -
        SMO_cost += 1.0 * SMO_times / lambda_timestamp * tot_size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR;// + 
        double merge_parent_cost = -1.0 * (1.0 / lambda_timestamp) * parent->size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR;// -   
        double delta_cost = read_cost + SMO_cost;
        AEX_PRINT("delta cost: " << delta_cost);
        if (delta_cost >= 0) return false;
    }
    
    node_ptr merge_node;
    key_type* key_buf = node_allocator.allocate_key_buf(tot_size);
    node_ptr* child_buf = node_allocator.allocate_nodeptr_buf(tot_size);
    for (slot_type i = 0, j = 0; i < node_merge_size; j += node_merge_buf[i]->size, ++i)
            copy_to_buffer(new_node, key_buf + j, child_buf + j);
    if (tot_size >= traits::ML_INNER_NODE_SIZE){
        InnerNodeModel m;
        slot_type slot_size;
        slot_size = min_slot_size(tot_size, this->inner_node_few_ratio[new_node->level], traits::MIN_INNER_NODE_SLOT_SIZE);
        if (slot_size * this->inner_node_few_ratio[new_node->level] > tot_size) slot_size <<= 1;
        if (!m.train(key_buf, tot_size, slot_size)){
            std::for_each(node_merge_buf, node_merge_buf + node_merge_size, [](node_ptr node){UNSET_FLAG(node, CAN_MERGED);});
            ret = false;
        }
        if (!self::check_collision(m, size, slot_size)){
            std::for_each(node_merge_buf, node_merge_buf + node_merge_size, [](node_ptr node){UNSET_FLAG(node, CAN_MERGED);});
            goto insert_merge_end;
        }
        merge_node = node_allocator.allocate_inner_node(slot_size, true);
        merge_node->construct(key_buf, child_buf, tot_size, m);
    }
    else{
        slot_type slot_size = min_slot_size(tot_size, traits::MIN_INNER_NODE_SLOT_SIZE);
        merge_node = node_allocator.allocate_inner_node(slot_size, false);
        merge_node->construct(key_buf, child_buf);
    }
    node_allocator.deallocate(key_buf);
    node_allocator.deallocate(child_buf);
    if (ret){
        merge_node->level = node_merge_buf[0]->level;
        merge_node->balance_stats = node_balance_stats(this->balance_stats.get_timestamp(), SMO_times, 0);
        SET_FLAG(merge_node, CAN_MERGED);
        this->m_stats.level_node[merge_node->level] += 1 - node_merge_size;

        slot_type prev_pos = parent->prev_item(pred_pos);
        std::fill(parent->key_ptr + prev_pos, parent->key_ptr + pred_pos + 1, last_key);
        std::fill(parent->child_ptr + prev_pos, parent->child_ptr + pred_pos + 1, merge_node);
        slot_type next_pos = parent->next_item(last_pos);
        std::fill(parent->key_ptr + pred_pos + 1, parent->key_ptr + next_pos, parent->key_ptr[next_pos]);
        std::fill(parent->child_ptr + pred_pos + 1, parent->child_ptr + next_pos, parent->child_ptr[next_pos]);
        merge_node->prev = node_merge_buf[0]->prev;
        merge_node->next = node_merge_buf[node_merge_size - 1]->prev;
        if (merge_node->prev != nullptr)
            merge_node->prev->next = merge_node;
        if (merge_node->next != nullptr)
            merge_node->next->prev = merge_node;
        for_each(node_merge_buf, node_merge_buf + node_merge_size, [&](node_ptr node){node_allocator.free_node(node);});
    }
    return ret;
}

//template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::merge_nodes(inner_node_ptr parent, key_type* key_buffer, data_node_ptr* node_buffer, slot_type size){
//    //AEX_ASSERT(std::is_same_v<aex_data_node<key_type, value_type, traits>, data_node>);
//    if constexpr (traits::AllowDynamicDataNode){
//        #ifdef AEX_EXPERIMENT
//        ++this->opt_stats.data_node_merge_cnt;
//        #endif
//
//        size_type data_size = 0;
//        for (slot_type i = 0; i < size; ++i)
//            data_size += node_buffer[i]->size;
//        std::vector<key_type> key_buf(data_size);
//        std::vector<value_type> data_buf(data_size);
//
//        for (slot_type i = 0; i < size; ++i){
//            std::copy(static_cast<data_node_ptr>(node_buffer[i])->key, static_cast<data_node_ptr>(node_buffer[i])->key + node_buffer[i]->size, key_buf.data() + data_size);
//            std::copy(static_cast<data_node_ptr>(node_buffer[i])->data, static_cast<data_node_ptr>(node_buffer[i])->data + node_buffer[i]->size, data_buf.data() + data_size);
//            data_size += node_buffer[i]->size;
//        }
//
//        DataNodeModel m;
//        data_node_ptr new_node = node_buffer[size - 1];
//        node_allocator.reallocate(new_node, min_slot_size(data_size, traits::MIN_DATA_NODE_SLOT_SIZE));
//
//        if (linear_probe(key_buf.data(), data_size, m) == size){
//            new_node->construct(key_buf.data(), data_buf.data(), data_buf.size(), m);
//        }
//        else{
//            UNSET_FLAG(new_node, node_property::ML_NODE);
//            new_node->construct(key_buf.data(), data_buf.data(), data_buf.size());
//        }
//
//        node_ptr prev_node = node_buffer[0]->prev, next_node = node_buffer[size - 1]->next;
//        new_node->prev = prev_node;
//        new_node->next = next_node;
//        if (prev_node != nullptr)
//            prev_node->next = new_node;
//        if (next_node != nullptr)
//            next_node->prev = new_node;
//        
//        for (slot_type i = 0; i < size - 1; ++i){
//            node_allocator.free_node(node_buffer[i]);
//            --this->m_stats.level_node[0];
//        }
//    }
//}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_split(inner_node_ptr node){
    // delta cost
    // 1. delta all read cost
    // 2. delta SMO cost
    // 3. delta write cost
    AEX_HINT("CHECK_SPLIT_INNER_NODE");
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_balance_check_split_cnt;
    #endif
    if constexpr (traits::AllowBalance == false)
        return false;
    double lambda_timestamp = this->balance_stats.get_lambda_timestamp();
    double train_pro = 1.0 * node->balance_stats.get_SMO_times() / lambda_timestamp;
    double write_pro = 1.0 * node->balance_stats.get_write_times() / lambda_timestamp;

    double read_cost = 1.0 * (1.0 / this->m_stats.level_node[node->level]) * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR;
    double SMO_cost = -(train_pro / 2) * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR * node->size;
    double write_cost = (IS_ML_NODE(node)) ? 0 : -1.0 * write_pro * (node->size / 2) * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR;
    double delta_cost = read_cost + SMO_cost + write_cost;
    //if ()
    //    AEX_PRINT("read cost=" << read_cost << ", SMO_cost=" << SMO_cost << ", write_cost=" << write_cost << ", delta_cost=" << delta_cost);
    if (delta_cost < 0){
        #ifdef AEX_EXPERIMENT
        ++opt_stats.inner_node_balance_split_cnt;
        #endif
        AEX_PRINT("timestamp=" << this->balance_stats.get_timestamp() << ", lambda_timestamp=" << lambda_timestamp);
        AEX_PRINT("node=" << node << ", train_times=" << node->balance_stats.get_SMO_times() << ", write_times=" << node->balance_stats.get_write_times());
        AEX_PRINT("read_cost=" << read_cost << ", SMO_cost=" << SMO_cost << ", write_cost=" << write_cost << ", delta_cost=" << delta_cost);
        return true;
    }
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::check_split_size(inner_node_ptr node){
    // delta total cost
    // 1. delta all read cost
    // 2. delta SMO cost
    // 3. delta write cost? (X)
    //AEX_HINT("CHECK_SPLIT_INNER_NODE");
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_balance_check_split_cnt;
    #endif
    if constexpr (traits::AllowRWBalance == false)
        return 1;
    //if (!IS_ML_NODE(node))
    //    return 1;
    double lambda_timestamp = this->balance_stats.get_lambda_timestamp();
    node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    double write_pro = 1.0 * node->balance_stats.get_write_times() / lambda_timestamp;
    double SMO_pro = 1.0 * node->balance_stats.get_SMO_times() / lambda_timestamp;
    double read_cost, write_cost, SMO_cost, delta_cost;
    slot_type split_nums = 1;
    do{
        split_nums <<= 1;
        read_cost = 1.0 * (1.0 * (split_nums - 1) / this->m_stats.level_node[node->level]) * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR; // +
        write_cost = (!IS_ML_NODE(node)) * (-write_pro * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR * node->size + write_pro * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR * (node->size / split_nums)); // 0 or -
        SMO_cost = -SMO_pro * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR * node->size + SMO_pro * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR * (node->size / split_nums); // - +
        //delta_cost = read_cost + write_cost + SMO_cost;
        delta_cost = read_cost + write_cost + SMO_cost;
    }while(node->size / split_nums > traits::MIN_INNER_NODE_SLOT_SIZE && node->size / (split_nums * 2) > traits::MIN_INNER_NODE_SLOT_SIZE && delta_cost < 0);
    split_nums >>= 1;
    if (split_nums > 1){
        #ifdef AEX_EXPERIMENT
        ++opt_stats.inner_node_balance_split_cnt;
        #endif
        
        AEX_IMPORTANT("timestamp=" << this->balance_stats.get_timestamp() << ", lambda_timestamp=" << lambda_timestamp);
        AEX_IMPORTANT("node=" << node << ", train_times=" << node->balance_stats.get_SMO_times() << ", write_times=" << node->balance_stats.get_write_times());
        AEX_IMPORTANT("read_cost=" << read_cost << ", SMO_cost=" << SMO_cost << ", delta_cost=" << delta_cost);
        AEX_IMPORTANT("split_nums=" << split_nums);
        return split_nums;
    }
    return 1;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_split(data_node_ptr node, bool is_forced){
    // delta cost
    // 1. delta write cost
    // 2. delta all read cost
    // 3. average SMO cost
    if constexpr (traits::AllowBalance && traits::AllowDynamicDataNode){
        AEX_HINT("CHECK_SPLIT_DATANODE");
        double lambda_timestamp = this->balance_stats.get_timestamp();
        node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
        double write_pro = 1.0 * node->balance_stats.get_write_times() / lambda_timestamp;
        double SMO_pro = 1.0 * node->balance_stats.get_SMO_times() / lambda_timestamp;
        double write_cost = -write_pro * (node->slot_size) / 2 * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR; // -
        double read_cost = 1.0 / this->m_stats.data_node() * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR; // +
        double SMO_cost = SMO_pro * node->size * traits::MODEL_ARGS::DATA_NODE_TRAIN_FACTOR;// +
        double delta_cost = write_cost + read_cost + (1 - is_forced) * SMO_cost;
        
        
        AEX_PRINT("balance cost:" <<  delta_cost);
        if (delta_cost < 0) 
            return true;
        return false;
    }
    else return false;
}

}
