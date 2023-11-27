//#include "aex/aex.h"
#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::update_node_list_frequency(dynamic_node_ptr node, node_ptr* node_list, slot_type n){
    unsigned long long recent_udpate_timestamp = node->balance_stats.get_recent_update_timestamp();
    double train_times = node->balance_stats.get_train_times(), write_times = node->balance_stats.get_write_times();
    for (slot_type i = 0; i < n; ++i){
        ((dynamic_node_ptr)node_list[i])->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                                        train_times, 
                                                        write_times * (1.0 * node_list[i]->size / node->size));
    }
}


template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_insert_merge(node_ptr* node_buffer, slot_type size){
    // delta cost
    // 1. write cost 
    // 2. read cost
    // 3. merge cost
    // 4. train cost

    // read/write frequency:
    // W\R     low high
    // low      T   F
    // high     F   F
    double write_times = 0, train_times = 0;
    size_type tot_size = 0;
    inner_node_ptr parent = node_buffer[0]->parent;
    AEX_ASSERT(parent != nullptr);
    if (node_buffer[0]->level <= 1)
        return false;
    parent->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    for (slot_type i = 0; i < size; ++i){
        node_buffer[i]->balance_stats.update_frequency(this->balance_stats.get_timestamp());
        write_times += node_buffer[i]->balance_stats.get_write_times();
        train_times += node_buffer[i]->balance_stats.get_train_times();
        tot_size += node_buffer[i]->size;
    }

    if (tot_size > traits::MAX_DATA_NODE_SLOT_SIZE)
        return false;

    double lambda_timestamp = this->balance_stats.get_lambda_timestamp();
    double write_pro = write_times / lambda_timestamp;

    double write_cost = write_pro * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR * tot_size; // +
    double read_cost;
    if (IS_LEAF_NODE(node_buffer[0]))
        read_cost = -1.0 * (size / this->m_stats.level_node[node_buffer[0]->level]) * traits::MODEL_ARGS::DATA_NODE_MODEL_SEARCH_FACTOR; // - +
    else
        read_cost = -1.0 * (size / this->m_stats.level_node[node_buffer[0]->level]) * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR; // - +
                    
    double merge_cost;
    // merge cost:
    if (IS_LEAF_NODE(node_buffer[0]))
        merge_cost = write_pro * tot_size * traits::MODEL_ARGS::DATA_NODE_TRAIN_FACTOR; // +
    else 
        merge_cost = write_pro * tot_size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR; // +

    merge_cost = -write_pro * parent->size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR
                -(this->m_stats.height - parent->level) * traits::MODEL_ARGS::GAP_ARRAY_INSERT_FACTOR * (1.0 * this->inner_node_few_ratio[1] / this->inner_node_few_ratio[parent->level]);// -

    double delta_cost = write_cost + read_cost + merge_cost;

    AEX_PRINT("balance cost: " << delta_cost);
    return delta_cost < 0;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_nodes(key_type* key_buffer, dynamic_data_node_ptr* node_buffer, slot_type size){
    //AEX_ASSERT(std::is_same<aex_data_node<key_type, value_type, traits>, data_node>::value == true);
    AEX_ASSERT((std::is_same<data_node, dynamic_data_node>::value));

    #ifdef AEX_EXPERIMENT
    ++this->opt_stats.data_node_merge_cnt;
    #endif

    size_type data_size = 0;
    for (slot_type i = 0; i < size; ++i)
        data_size += node_buffer[i]->size;
    std::vector<key_type> key_buf(data_size);
    std::vector<value_type> data_buf(data_size);

    for (slot_type i = 0; i < size; ++i){
        std::copy(static_cast<data_node_ptr>(node_buffer[i])->key, static_cast<data_node_ptr>(node_buffer[i])->key + node_buffer[i]->size, key_buf.data() + data_size);
        std::copy(static_cast<data_node_ptr>(node_buffer[i])->data, static_cast<data_node_ptr>(node_buffer[i])->data + node_buffer[i]->size, data_buf.data() + data_size);
        data_size += node_buffer[i]->size;
    }

    data_node_model m;
    data_node_ptr new_node = node_buffer[size - 1];
    node_allocator.reallocate(new_node, min_slot_size(data_size, traits::MIN_DATA_NODE_SLOT_SIZE));

    if (linear_probe(key_buf.data(), data_size, m) == size){
        new_node->construct(key_buf.data(), data_buf.data(), data_buf.size(), m);
    }
    else{
        UNSET_FLAG(new_node, node_property::ML_NODE);
        new_node->construct(key_buf.data(), data_buf.data(), data_buf.size());
    }

    node_ptr prev_node = node_buffer[0]->prev, next_node = node_buffer[size - 1]->next;
    new_node->prev = prev_node;
    new_node->next = next_node;
    if (prev_node != nullptr)
        prev_node->next = new_node;
    if (next_node != nullptr)
        next_node->prev = new_node;
    
    for (slot_type i = 0; i < size - 1; ++i){
        node_allocator.free_node(node_buffer[i]);
        --this->m_stats.level_node[0];
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_nodes(key_type* key_buffer, inner_node_ptr* node_buffer, slot_type size){
    #ifdef AEX_EXPERIMENT
    ++this->opt_stats.inner_node_merge_cnt;
    #endif
    size_type child_size = 0;

    for (slot_type i = 0; i < size; ++i)
        child_size += node_buffer[i]->size;

    std::vector<key_type> key_buf(child_size);
    std::vector<node_ptr> child_buf(child_size);
    
    size_type cnt = 0;
    for (slot_type i = 0; i < size; ++i){
        copy_to_buffer(node_buffer[i], key_buf.data() + cnt, child_buf.data() + cnt);
        cnt += node_buffer[i]->size;
        key_buf[cnt - 1] = key_buffer[i];
    }

    inner_node_model model;    
    bool ml_flag = child_size >= traits::MIN_ML_INNER_NODE_SIZE;
    slot_type ml_slot_size = min_slot_size(child_size, this->inner_node_few_ratio[node_buffer[0]->level], traits::MIN_INNER_NODE_SLOT_SIZE);
    if (ml_flag) ml_flag &= check_collision(key_buf.data(), child_size - 1, ml_slot_size, model);
    inner_node_ptr new_node = node_buffer[size - 1];
    if (ml_flag){
        SET_FLAG(new_node, node_property::ML_NODE);
        node_allocator.reallocate(new_node, ml_slot_size);
        new_node->construct(key_buf.data(), child_buf.data(), child_buf.size(), model);
    }
    else{
        UNSET_FLAG(new_node, node_property::ML_NODE);
        node_allocator.reallocate(new_node, min_slot_size(child_buf.size(), traits::MIN_INNER_NODE_SLOT_SIZE));
        new_node->construct(key_buf.data(), child_buf.data(), child_buf.size());
    }

    node_ptr prev_node = node_buffer[0]->prev, next_node = node_buffer[size - 1]->next;
    new_node->prev = prev_node;
    new_node->next = next_node;
    if (prev_node != nullptr)
        prev_node->next = new_node;
    if (next_node != nullptr)
        next_node->prev = new_node;
    
    for (slot_type i = 0; i < size - 1; ++i){
        --this->m_stats.level_node[node_buffer[i]->level];
        node_allocator.free_node(node_buffer[i]);
    }
    
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_split(inner_node_ptr node){
    // delta cost
    // 1. delta all read cost
    // 3. delta SMO cost
    if constexpr (traits::AllowBalance::value == false)
        return false;
    double lambda_timestamp = this->balance_stats.get_timestamp();
    double train_pro = 1.0 * node->balance_stats.get_train_times() / lambda_timestamp;
    double write_pro = 1.0 * node->balance_stats.get_write_times() / lambda_timestamp;

    double read_cost = 1.0 * (1.0 / this->m_stats.level_node[node->level]) * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR;
    double SMO_cost = -(train_pro / 2) * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR * node->size;
    double write_cost = (IS_ML_NODE(node)) ? 0 : -1.0 * write_pro * (node->size / 2) * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR;
    double delta_cost = read_cost + SMO_cost + write_cost;
    //if ()
        AEX_PRINT("read cost=" << read_cost << ", SMO_cost=" << SMO_cost << ", write_cost=" << write_cost << ", delta_cost=" << delta_cost);
    if (delta_cost < 0)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_split(dynamic_data_node_ptr node, bool is_forced){
    // delta cost
    // 1. delta write cost
    // 2. delta all read cost
    // 3. average SMO cost
    if constexpr (traits::AllowBalance::value == false)
        return false;
    AEX_ASSERT((std::is_same<data_node, dynamic_data_node>::value == true));
    double lambda_timestamp = this->balance_stats.get_timestamp();
    node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    double write_pro = 1.0 * node->balance_stats.get_write_times() / lambda_timestamp;
    double train_pro = 1.0 * node->balance_stats.get_train_times() / lambda_timestamp;
    double write_cost = -write_pro * (node->slot_size) / 2 * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR; // -
    double read_cost = 1.0 / this->m_stats.data_node() * traits::MODEL_ARGS::INNER_NODE_MODEL_SEARCH_FACTOR; // +
    double SMO_cost = train_pro * node->size * traits::MODEL_ARGS::DATA_NODE_TRAIN_FACTOR;// +
    double delta_cost = write_cost + read_cost + (1 - is_forced) * SMO_cost;
        
    AEX_PRINT("balance cost:" <<  delta_cost);
    if (delta_cost < 0) 
        return true;
    return false;
}

}
