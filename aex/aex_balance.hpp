#pragma once
//#include "aex/aex.h"
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline double aex_tree<_Key, _Val, traits>::estimate_cost() const {
    return this->root->est_cost;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_merge(inner_node_ptr node){
    double read_pro = 1.0 * node->size /  * this->read_times / (this->read_times + this->write_times);
    double node_pro = node->write_times / this->write_times;
    double write_pro = node->write_times / (this->read_times + this->write_times);
    double delta_cost = write_pro * node->data_size / 2 + node_pro * read_pro - \
                        (1.0 * node->data_size / this->m_traits.size);

    double merge_cost = node->data_size / (this->write_times - this->pre_write_times);

    if (delta_cost + merge_cost < 0) return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_split(data_node_ptr node){
    double read_pro = 1.0 * node->size /  * this->read_times / (this->read_times + this->write_times);
    double node_pro = node->write_times / this->write_times;
    double write_pro = node->write_times / (this->read_times + this->write_times);
    double delta_cost = -(write_pro * node->data_size / 2 + node_pro * read_pro) + \
                        (1.0 * node->data_node_size / this->m_traits.data_node);

    double split_cost = node->data_size / (this->write_times - this->pre_write_times);

    if (delta_cost + split_cost < 0) return true;
    return false;
}

template<typename _Key, typename _Val, typename traits>
double aex_tree<_Key, _Val, traits>::check_split(data_node_ptr node, size_type slot_size){
    double read_pro = 1.0 * node->size /  * this->read_times / (this->read_times + this->write_times);
    double node_pro = node->write_times / this->write_times;
    double write_pro = node->write_times / (this->read_times + this->write_times);
    double delta_cost = -(write_pro * node->data_size / 2 + node_pro * read_pro) + \
                        ((1.0 * node->data_size / slot_size) / this->size);

    return delta_cost;

    return false;
}

template<typename _Key, typename _Val, typename traits>
node_ptr aex_tree<_Key, _Val, traits>::balance_merge(inner_node_ptr node){
    // check
    if (node->prop & CHECK_MERGE) 
        return node;
    //mutex the write lock of the node
    data_node_ptr new_node = node_allocator::allocate_data_node(node->m_stats.size, true);
    data_node_ptr first_leaf = find_head_leaf(node), last_leaf = find_tail_leaf(node);
    size_type cnt = 0;

    for (auto i_leaf = first_leaf; i_leaf != last_leaf->next; i_leaf = i_leaf->next){
        //copy_to_buffer(i_leaf, key_buffer + cnt);
        copy_to_buffer(i_leaf->key, new_node->key + cnt);
        if (traits::used_as_set == true)
            data_memmove(i_leaf->data, new_node->data + cnt);
        cnt += i_leaf->next;
    }
    new_node->size = cnt;

    if (cnt < traits::EASY_MODEL_SLOT_SIZE){
        node->model.easy_model.train(new_node->key, cnt, cnt);
        if (node->check_error() * 2 > log(cnt)){
            node->prop ^= ML_NODE;
        }
    }
    else{
        node->model.complex_model->construct(new_node->key, cnt);
        if (node->check_error() * 2 > log(cnt)){
            node->model.complex_model->free();
            node->prop ^= ML_NODE;
        }
    }

    // merge
    {
        cnt = 0;
        data_node_ptr brother = (first_leaf->prev == nullptr) ? last_leaf->next : first_leaf->prev;
        erase_recursive(node);
        
        if (brother == nullptr){
            root = new_node;
            this->m_traits.level = 1;
        }

        if (parent->last_key() < key){
            for (inner_node_ptr node = parent->parent, son; node != nullptr; son = node, node = node->parent_node){
                size_type pos = node->last();
                update_childnode_key(node, son, key);
            }
        }
        insert_recursive(parent, key, son);
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::balance_split(data_node_ptr node){
    if (node->prop & CHECK_SPLIT) return false;
    std::vector<key_type> key_buffer;
    std::vecotr<node_ptr> node_buffer;

    size_type slot_size = traits::MIN_DATA_NODE_SLOT_SIZE;
    double min_cost = 0;
    for (size_type now_slot_size = traits::MIN_DATA_NODE_SLOT; now_slot_size <= traits::MIN_DATA_NODE_SLOT_SIZE; now_slot_size >>= 1){
        double cost = check_split(node, slot_size);
        if (min_cost < cost){
            min_cost = cost;
            slot_size = now_slot_size;
        }
    }
    size_type leaf_size = slot_size * traits::DATA_NODE_FEW_RATIO;
    for (size_type i = 0; i < node->size; i += leaf_size){
        data_node_ptr new_data_node = node_allocator::allocate_data_node(slot_size, false);
        memcpy(new_data->key, node->key + offset);
        data_memmove(new_data_node->value, node->data + offset);
    }
    if (node->parent == nullptr){
        this->bulk_load_node(key_buffer, node_buffer);
    }
    else{
        inner_node_ptr parent = node->parent;
        insert_recursive(parent, key_buffer, node_buffer);
    }

    {
        if (node->model.complex_model == nullptr){
            node->model.complex_model->free();
        }
        allocator::free(node->model.complex_model);
        node_allocator::free(node);
    }
    return true;
}


}