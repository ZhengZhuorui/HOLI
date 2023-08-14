//#include "aex/aex.h"
#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::update_node_list_frequency(node_ptr node, node_ptr* node_list, slot_type n){
    unsigned long long recent_udpate_timestamp = node->balance_stats.get_recent_update_timestamp();
    double train_times = node->balance_stats.get_train_times(), read_times = node->balance_stats.get_read_times(), write_times = node->balance_stats.get_write_times();
    for (slot_type i = 0; i < n; ++i){
        node_list[i]->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                                        train_times, 
                                                        read_times * (1.0 * node_list[i]->size / node->size),
                                                        write_times * (1.0 * node_list[i]->size / node->size));
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_insert_merge(data_node_ptr* node_buffer, slot_type size){
    // delta cost
    // 1. write cost 
    // 2. read cost
    // 3. merge cost
    // 4. delta insertion cost
    double read_times = 0, write_times = 0, train_times = 0;
    size_type tot_size = 0, data_node_size = 0, slot_size = 0;
    inner_node_ptr parent = node_buffer[0]->parent;

    for (size_type i = 0; i < size; ++i){
        node_buffer[i]->balance_stats.update_frequency(this->balance_stats.get_timestamp());
        read_times += node_buffer[i]->balance_stats.get_read_times();
        write_times += node_buffer[i]->balance_stats.get_write_times();
        train_times += node_buffer[i]->balance_stats.get_train_times();
        tot_size += node_buffer[i]->size;
    }

    if (tot_size > traits::MAX_DATA_NODE_SLOT_SIZE)
        return false;
    
    double read_pro = 1.0 * read_times / this->balance_stats.get_lambda_timestamp();
    double write_pro = 1.0 * write_times / this->balance_stats.get_lambda_timestamp();
    double train_pro = 1.0 * train_times / this->balance_stats.get_lambda_timestamp();
    double parent_train_pro = 1.0 * parent->balance_stats.get_train_times() / this->balance_stats.get_lambda_timestamp();

    double write_cost = write_pro * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR * tot_size; // +
    double read_cost = -1.0 * (size / this->m_stats.level_node[node_buffer[0]->level]) * traits::MODEL_ARGS::MODEL_SEARCH_FACTOR
                        + (read_pro + write_pro) * traits::MODEL_ARGS::BINEARY_SEARCH_FACTOR * log(tot_size); // - +
    double SMO_cost = train_pro * tot_size * traits::MODEL_ARGS::DATA_NODE_TRAIN_FACTOR; // +
    double delta_insert_cost = 0;
    if (parent != nullptr){
        double parent_train_pro = 1.0 * parent->balance_stats.get_train_times() / this->balance_stats.get_lambda_timestamp();
        delta_insert_cost = -parent_train_pro * parent->size * traits::INNER_NODE_TRAIN_FACTOR; // -
    }
    double delta_cost = write_cost + read_cost + SMO_cost + delta_insert_cost;

    AEX_PRINT("balance cost: " << delta_cost);
    if (delta_cost < 0)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_insert_merge(inner_node_ptr* node_buffer, slot_type size){
    // delta cost
    // 1. write cost 
    // 2. merge cost
    // 3. read cost
    double read_times = 0, write_times = 0, delta_cost = 0, train_times = 0;
    size_type tot_size = 0, data_node_size = 0, slot_size = 0;
    inner_node_ptr parent = node_buffer[0]->parent;
    
    for (size_type i = 0; i < size; ++i){
        double SMO_cost = node_buffer[i]->balance_stats.train_times * traits::LEARNING_COST / this->m_stats.timestamp * node_buffer[i]->slot_size;
        delta_cost += SMO_cost;
        node_buffer[i]->balance_stats.update_frequency(this->balance_stats.get_timestamp());
        read_times += node_buffer[i]->balance_stats.get_read_times();
        write_times += node_buffer[i]->balance_stats.get_write_times();
        train_times += node_buffer[i]->balance_stats.get_train_times();
        tot_size += node_buffer[i]->size;
    }

    unsigned int level = node_buffer[0]->level - 1;
    if (1.0 * tot_size > traits::MAX_INNER_NODE_SLOT_SIZE * this->inner_node_few_ratio[level])
        return false;
    
    double read_pro = 1.0 * read_times / this->balance_stats.get_lambda_timestamp();
    double write_pro = 1.0 * write_times / this->balance_stats.get_lambda_timestamp();
    double train_pro = 1.0 * train_times / this->balance_stats.get_lambda_timestamp();

    double read_cost = -1.0 * (size / this->balance_stats.inner_node) + (read_pro + write_pro) * traits::BINEARY_SEARCH_FACTOR * log(tot_size); // - +
    double write_cost =  write_pro * tot_size * traits::MODEL_ARGS::DENSE_ARRAY_INSERTION_FACTOR; // +
    double SMO_cost = train_pro * tot_size * traits::MODEL_ARGS::INNER_NODE_TRAIN_FACTOR; // +
    double delta_insert_cost = 0; // -
    if (parent != nullptr){
        double parent_train_pro = 1.0 * parent->balance_stats.get_train_times() / this->balance_stats.get_lambda_timestamp();
        delta_insert_cost = -parent_train_pro * parent->size * traits::INNER_NODE_TRAIN_FACTOR; // -
    }
    
    delta_cost = write_cost + read_cost + SMO_cost + delta_insert_cost;

    AEX_PRINT("balance cost: " << delta_cost);
    if (delta_cost < 0)
        return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_nodes(data_node_ptr* node_buffer, slot_type size){
    AEX_ASSERT(size > 1);
    size_type data_size = 0;
    unsigned int level = node_buffer[0]->level;
    for (size_type i = 0; i < size; ++i)
        data_size += node_buffer[i]->size;
    std::vector<key_type> key_buf(data_size);
    std::vector<value_type> data_buf(data_size);

    for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
        std::copy(static_cast<data_node_ptr>(node_buffer[i])->key, static_cast<data_node_ptr>(node_buffer[i])->key + node_buffer[i]->size, key_buf + cnt);
        std::copy(static_cast<data_node_ptr>(node_buffer[i])->data, static_cast<data_node_ptr>(node_buffer[i])->data + node_buffer[i]->size, data_buf + cnt);
    }

    data_node_model m;
    data_node_ptr new_node = node_buffer[size - 1];
    node_allocator.reallocate(new_node, data_size);
    if (linear_probe(key_buf.data(), data_size, m) == size){
        new_node->construct(key_buf.data(), data_buf.data(), m);
    }
    else{
        new_node = node_allocator.allocate_data_node(data_size, false);
        new_node->prop &= ~(node_property::ML_NODE);
        new_node->construct(key_buf.data(), data_buf.data());
    }

    data_node_ptr prev_node = node_buffer[0]->prev, next_node = node_buffer[size - 1]->next;
    new_node->prev = prev_node;
    new_node->next = next_node;
    if (prev_node != nullptr)
        prev_node->next = new_node;
    if (next_node != nullptr)
        next_node->prev = new_node;
    
    for (slot_type i = 0; i < size - 1; ++i){
        node_allocator.free_node(node_buffer[i]);
        --this->m_stats.data_node;
        --this->m_stats.level_node[0];
    }

}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_nodes(inner_node_ptr* node_buffer, slot_type size){
    size_type child_size = 0;

    for (size_type i = 0; i < size; ++i)
        child_size += node_buffer[i]->size;

    std::vector<key_type> key_buf(child_size);
    std::vector<node_ptr> child_buf(child_size);
    
    for (size_type i = 0, cnt = 0; i < size; cnt += node_buffer[i]->size, ++i){
        copy_to_buffer(node_buffer[i], key_buf.data() + cnt, child_buf.data() + cnt);
    }

    inner_node_model model;
    inner_node_ptr new_node;
    size_type slot_size = 0;
    int level = node_buffer[0]->level;

    while (slot_size * this->inner_node_few_ratio[level] <= child_size && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
    slot_size >>= 1;
    bool ml_flag = check_collision(key_buf.data(), child_size, slot_size, model);

    new_node = node_allocator.allocate_inner_node(child_size, ml_flag);
    ++this->m_stats.level_node[node_buffer[0]->level];
    ++this->m_stats.inner_node;
    if (ml_flag)
        new_node->construct(key_buf.data(), child_buf.data(), model);
    else 
        new_node->construct(key_buf.data(), child_buf.data());
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_split(data_node_ptr node, bool is_forced){
    // delta cost
    // 1. delta write cost
    // 2. delta all read cost
    // 3. average SMO cost
    if (traits::AllowBalance::value == false)
        return false;
    node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    double write_pro = node->balance_stats.get_write_times() / this->balance_stats.get_lambda_timestamp();
    double train_pro = node->balance_stats.get_train_times() / this->balance_stats.get_lambda_timestamp();
    double write_cost = -write_pro * (node->slot_size) / 2 * traits::MODEL_ARGS::DENSE_ARRAY_INSERT_FACTOR; // -
    double read_cost = 1.0 / this->m_stats.data_node * traits::MODEL_ARGS::MODEL_SEARCH_FACTOR; // +
    double SMO_cost = train_pro * node->size * traits::MODEL_ARGS::DATA_NODE_TRAIN_FACTOR;
    double delta_cost = write_cost + read_cost + (1 - is_forced) * SMO_cost;
        
    AEX_PRINT("balance cost:" <<  delta_cost);
    if (delta_cost < 0) 
        return true;
    return false;
}

}
