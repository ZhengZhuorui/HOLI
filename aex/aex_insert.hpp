#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    #ifdef AEX_EXPERIMENT
    //std::chrono::system_clock::time_point t1, t2;
    //t1 = std::chrono::high_resolution_clock::now();
    //static double insert_ascend_time = 0;
    //static int cnt = 0;
    //++cnt;
    #endif
    //AEX_PRINT("insert: key=" << key);
    this->balance_stats.update_timestamp();
    std::pair<iterator, bool> ret;

    if (root == nullptr){
        //root = head_leaf = tail_leaf = node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
        root = head_leaf = tail_leaf = node_allocator.allocate_static_data_node();
        static_cast<data_node_ptr>(root)->insert(key, value, 0);
        root->next = empty_leaf;
        empty_leaf->prev = root;
        root->prev = nullptr;
        ++m_stats.level_node[0];
        m_stats.height = 1;
        ++this->m_stats.size;
        return std::pair<iterator, bool>(iterator(head_leaf, 0), true);
    }
    //AEX_FORMAT("level=%u, size=%lld, root_size=%lld", root->level, this->m_stats.size);
    data_node_ptr node = find_leaf(key);
    slot_type pos = node->find_lower_pos(key);
    //AEX_PRINT("node=" << node << ", pos=" << pos << ", size=" << node->size );

    /* find the insert position */
    if (pos < node->size && node->key[pos] == key){
        return std::pair<iterator, bool>(iterator(node, pos), false);
    }

    /* if data node is full, split the node */
    if (isfull(node)){
        if constexpr (!traits::AllowDynamicDataNode::value){
            //data_node_ptr new_node = node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
            data_node_ptr new_node = node_allocator.allocate_static_data_node();
            ++this->m_stats.level_node[0];
            split(new_node, node);
            iterator iter;
            if (pos < new_node->size){
                iter = iterator(new_node, pos);
                new_node->insert(key, value, pos);
            }
            else{
                iter = iterator(node, pos - new_node->size);
                node->insert(key, value, pos - new_node->size);
            }
            
            insert_one(node->parent, new_node->key[new_node->size - 1], static_cast<node_ptr>(new_node));
            ret = std::pair<iterator, bool>(iter, true);
            //AEX_PRINT("node=" << iter._M_node << ", pos=" << iter.offset << ", iter key=" << iter.key() << ", key=" << key);
        }
        else {
            dynamic_data_node_ptr _node = (dynamic_data_node_ptr)(node);
            if (_node->slot_size * 2 > traits::MAX_DATA_NODE_SLOT_SIZE || (check_split(_node, true))){
                std::vector<key_type> insert_key;
                std::vector<node_ptr> insert_node;
                insert_split(_node, key, value, insert_key, insert_node);
                if (insert_key.size() == 1)
                    insert_one(_node->parent, insert_key[0], insert_node[0]);
                if (insert_key.size() > 1)
                    insert_ascend(_node->parent, insert_key, insert_node);
                ret = std::pair<iterator, bool>(find_iterator(key), true);
            }
            else{
                [[maybe_unused]] bool flag = rescale(_node, _node->slot_size << 1);
                AEX_ASSERT(flag == true);
                pos = _node->insert(key, value);
                ret = std::pair<iterator, bool>(iterator(node, pos), true);
            }
        }
    }
    /* else insert the position of the data node*/ 
    else{
        node->insert(key, value, pos);
        ret = std::pair<iterator, bool>(iterator(node, pos), true);
    }
    ++m_stats.size;

    #ifdef AEX_EXPERIMENT
    //t2 = std::chrono::high_resolution_clock::now();
    //insert_ascend_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    //if (cnt % 10000 == 0)
    //    AEX_PRINT("insert_ascend time=" << insert_ascend_time);
    #endif
    return ret;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::false_type allow_insert_balance){
    AEX_ASSERT(node != nullptr);
    return node->insert(key, child);
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr node, const key_type &key, const node_ptr child, std::true_type allow_insert_balance){
    if (!node->insert(key, child)){
        bitmap bm = node->bitmap_ptr;
        slot_type pred_pos = node->predict(key);
        slot_type inserted_pos = pred_pos;
        for (; inserted_pos < node->slot_size - 1 && inserted_pos - pred_pos < traits::ERROR_BOUND; ++inserted_pos)
            if (key <= node->key_ptr[inserted_pos]){
                break;
            }
        
        // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
        // if inserted_pos == pred_pos, it insert failed because !node->insert(key, child)
        if (inserted_pos == node->slot_size - 1 || inserted_pos - pred_pos >= traits::ERROR_BOUND || inserted_pos == pred_pos)
            return false;

        // insert_pos slot must used. Otherwise node->insert(key, child) is true
        AEX_ASSERT(bitmap_impl::at(bm, inserted_pos) == 0);
        // no multi key
        AEX_ASSERT(node->key_ptr[inserted_pos] == key);

        key_type key_buffer[traits::ERROR_BOUND + 1];
        node_ptr node_buffer[traits::ERROR_BOUND + 1];
        slot_type buffer_size = 0;
        for (slot_type i = pred_pos; i < inserted_pos; ++i)
            if (bitmap_impl::at(bm, i)){
                key_buffer[buffer_size] = node->key_ptr[i];
                node_buffer[buffer_size++] = node->child_ptr[i];
            }
        key_buffer[buffer_size] = key;
        node_buffer[buffer_size++] = child;

        if (isfew(node, -(buffer_size - 1)) == false && check_insert_merge(node_buffer, buffer_size)){
            merge_nodes(key_buffer, node_buffer, buffer_size);
            slot_type prev_pos = node->prev_item(pred_pos);
            for (slot_type i = pred_pos; i <= inserted_pos; ++i)
                bitmap_impl::set_zero(node->bitmap_ptr, inserted_pos);
            std::fill(node->key_ptr + prev_pos + 1, node->key_ptr + pred_pos + 1, key);
            std::fill(node->child_ptr + prev_pos + 1, node->child_ptr + pred_pos + 1, child);
            bitmap_impl::set_one(node->bitmap_ptr, pred_pos);
            if (inserted_pos < node->slot_size - 1){
                std::fill(node->key_ptr + pred_pos + 1, node->key_ptr + inserted_pos + 1, node->key_ptr[inserted_pos + 1]);
                std::fill(node->child_ptr + pred_pos + 1, node->child_ptr + inserted_pos + 1, node->child_ptr[inserted_pos + 1]);
            }

            for (slot_type i = 0; i < buffer_size; ++i)
                node_allocator.free_node(node_buffer[i]);
            return true;
        }
        else return false;
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::ml_node_insert_split_bulk_load(inner_node_ptr node, const slot_type start, const key_type key, node_ptr child, bool half_flag){
    std::vector<key_type> new_key;
    std::vector<node_ptr> new_child;
    new_key.push_back(key);
    new_child.push_back(child);

    std::vector<key_type> new_key_2;
    std::vector<node_ptr> new_child_2;
    slot_type n = node->size;

    split(node->key + start, node->child + start, n / 2, node->level, new_key_2, new_child_2);
    for (slot_type i = 0; i < new_key_2.size(); ++i){
        new_key.push_back(new_key_2[i]);
        new_child.push_back(new_child_2[i]);
    }
    new_key.push_back(node->key[n / 2 - 1]);
    new_child.push_back(new_child_2[new_key_2.size()]);

    split(node->key + n / 2, node->child + n / 2, n / 2, node->level, new_key_2, new_child_2);
    for (slot_type i = 0; i < new_key_2.size(); ++i){
        new_key.push_back(new_key_2[i]);
        new_child.push_back(new_child_2[i]);
    }
    new_child.push_back(new_child_2[new_key_2.size()]);
    update_node_list_frequency(new_child, node);
    link_node_list_and_replace_last_node(new_child, node);
    new_child.pop_back();
    insert_ascend(node->parent, new_key, new_child);
}

// Split an node when the node insert item and (the size is larger than full ratio or no empty slot to insert)
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::ml_node_insert_split_pipeline(inner_node_ptr node, const key_type* const key, const node_ptr* const child, const slot_type n){
    
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_split_cnt;
    #endif
    inner_node_ptr parent = node->parent;
    bool append_flag = (parent != nullptr);
    slot_type j = 0, n_slot = 0;
    AEX_ASSERT(IS_ML_NODE(node) == true);
    copy_to_buffer(node, node->key_ptr, node->child_ptr);
    slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, key[0]) - node->key_ptr;
    std::move_backward(node->key_ptr + pos, node->key_ptr + node->size, node->key_ptr + node->size + n);
    std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + n);
    std::copy(key, key + n, node->key_ptr + pos);
    std::copy(child, child + n, node->child_ptr + pos);
    node->size += n;
    slot_type start = 0, ans_size, ans_slot_size, ml_flag, split_node_cnt = 0, size = node->size;
    inner_node_ptr first_node = nullptr;
    if (check_split(node)){
        #ifdef AEX_EXPERIMENT
        ++opt_stats.inner_node_balance_split_cnt;
        #endif
        
        slot_type left_size = size / 2;
        while (start < left_size){
            std::tie(ans_size, ans_slot_size, ml_flag) = split(node->key_ptr + start, left_size - start, node->level);
            split_node_cnt++;
            inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
            if (first_node == nullptr)
                first_node = new_node;
            new_node->model.train(node->key_ptr + start, ans_size, ans_slot_size);
            new_node->prev = node->prev;
            if (node->prev != nullptr)
                node->prev->next = new_node;
            node->prev = new_node;
            new_node->next = node;
            append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], node);
            append_flag &= isfull(parent);
            if (!append_flag) ml_node_insert_split_bulk_load(node, start, node->key_ptr[start + ans_size - 1], new_node, 1);
            start += ans_size;
        }
    }
    while (start < size){
        std::tie(ans_size, ans_slot_size, ml_flag) = split(node->key_ptr + start, left_size - start, node->level);
        split_node_cnt++;
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
        if (first_node == nullptr)
            first_node = new_node;
        new_node->model.train(node->key_ptr + start, ans_size, ans_slot_size);
        if (start + ans_size == node->size){
            node_ptr prev = node->prev, next = node->next;
            *node = *new_node;
            node->parent = parent;
            node->prev = prev;
            node->next = next;
            split_node_cnt--;
        }
        else{
            new_node->prev = node->prev;
            if (node->prev != nullptr)
                node->prev->next = new_node;
            node->prev = new_node;
            new_node->next = node;
            append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], node);
            append_flag &= isfull(parent);
            if (!append_flag) ml_node_insert_split_bulk_load(node, start, node->key_ptr[start + ans_size - 1], new_node, 0);
        }
        start += ans_size;
    }

    if (split_node_cnt == 0)
        node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    else if (traits::AllowBalance){
        unsigned long long recent_udpate_timestamp = node->balance_stats.get_recent_update_timestamp();
        double train_times = node->balance_stats.get_train_times(), write_times = node->balance_stats.get_write_times();
        while (first_node != node->next){
            first_node->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                        train_times, 
                                        write_times * (1.0 * node_list[i]->size / size));
            first_node++;
        }
    }
}

// Split an data node to many nodes with linear_probe
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_split(dynamic_data_node_ptr node, const key_type key, const value_type data, 
                                                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){

    #ifdef AEX_EXPERIMENT
    ++opt_stats.data_node_split_cnt;
    #endif
    
    std::vector<key_type> key_buf(node->size + 1), new_key_2;
    std::vector<value_type> data_buf(node->size + 1);
    std::vector<node_ptr> new_child_2;

    //insertion
    std::move(node->key, node->key + node->size, key_buf.data());
    std::move(node->data, node->data + node->size, data_buf.data());

    slot_type pos = std::lower_bound(key_buf.data(), key_buf.data() + node->size, key) - key_buf.data();
    std::move_backward(node->key + pos, node->key + node->size, node->key + node->size + 1);
    std::move_backward(node->data + pos, node->data + node->size, node->data + node->size + 1);
    node->key[pos] = key;
    node->data[pos] = data;

    // split_with_linear_probe(key_buf.data(), data_buf.data(), node->size / 2, new_key, new_child);
    // split_with_linear_probe(key_buf.data() + node->size / 2, data_buf.data() + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);
    split_with_exponential_probe(key_buf.data(), data_buf.data(), node->size / 2, new_key, new_child);
    split_with_exponential_probe(key_buf.data() + node->size / 2, data_buf.data() + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);
    
    size_t new_m = new_key_2.size();
    for (size_t i = 0; i < new_m; ++i){
        new_key.push_back(new_key_2[i]);
        new_child.push_back(new_child_2[i]);
    }

    node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child);
    new_key.pop_back();
    new_child.pop_back();
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::build_tree(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    AEX_ASSERT(key_buf.size() + 1 == child_buf.size());
    this->m_stats.height = 1;
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    while (child_buf.size() > 1){    
        ++this->m_stats.height;
        split(key_buf.data(), child_buf.data(), child_buf.size(), this->m_stats.height - 1, new_key_buf, new_child_buf);
        size_type m = new_child_buf.size();
        
        new_child_buf[0]->prev = nullptr;
        new_child_buf[m - 1]->next = nullptr;
        for(size_type i = 0; i < m - 1; ++i){
            new_child_buf[i + 1]->prev = new_child_buf[i];
            new_child_buf[i]->next = new_child_buf[i + 1];
        }
        std::swap(key_buf, new_key_buf);
        std::swap(child_buf, new_child_buf);
        new_key_buf.clear();
        new_child_buf.clear();
    }
    root = child_buf[0];
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
    this->deconstruct(this->root);
    if (nums == 0)
        return;
    this->m_stats.height = 1;
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    for (size_type i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
        #ifdef AEX_DEBUG
        if (i > 0)
            AEX_ASSERT(key_buf[i - 1] <= key_buf[i]);
        #endif
    }
    //this->m_stats.min_key = key_buf[0];
    //this->m_stats.max_key = key_buf[nums - 1];

    if constexpr (traits::AllowDynamicDataNode::value){
        //split_with_linear_probe(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
        split_with_exponential_probe(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    }
    else{
        split_to_static_data_node(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    }
    
    size_type m = new_child_buf.size();
    new_child_buf[0]->prev = nullptr;
    new_child_buf[m - 1]->next = this->empty_leaf;
    this->empty_leaf->prev = new_child_buf[m - 1];
    for(size_type i = 0; i < m - 1; ++i){
        new_child_buf[i + 1]->prev = new_child_buf[i];
        new_child_buf[i]->next = new_child_buf[i + 1];
    }

    this->m_stats.size = nums;
    this->head_leaf = static_cast<data_node_ptr>(new_child_buf[0]);
    this->tail_leaf = static_cast<data_node_ptr>(new_child_buf[m - 1]);
    
    new_key_buf.pop_back();
    this->build_tree(new_key_buf, new_child_buf);
}


// if node split, return true. Otherwise return false
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_ascend(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    //AEX_PRINT("insert_ascend");
    
    std::chrono::system_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    static double insert_ascend_time = 0;
    static int cnt = 0;
    ++cnt;

    if (child_buf.size() == 0) 
        return false;
    AEX_ASSERT(key_buf.size() == child_buf.size());
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    inner_node_ptr now_node = node;
    bool ret_flag = false;
    while (now_node != nullptr && child_buf.size() > 0){
        size_t num_buf = child_buf.size();
        bool split_flag = false;
        now_node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
        if (isfull(now_node, num_buf - 1)) {
            if (rescale(now_node, now_node->real_slot_size() << 1) == false){
                insert_split(now_node, key_buf.data(), child_buf.data(), num_buf, new_key_buf, new_child_buf);
                split_flag = true;
                ret_flag = true;
            }
        }

        if (split_flag == false){
            for (size_t i = 0; i < num_buf; ++i){
                /* if can insert, then insert it, else split it */
                //typename traits::AllowInsertBalance _;
                //if (this->insert_node(now_node, key_buf[i], child_buf[i], _)){
                //}
                //if (isfull(now_node)){
                //    AEX_PRINT(isfull(now_node, num_buf - 1) << ", " << num_buf);
                //}
                if (!now_node->insert(key_buf[i], child_buf[i])){
                    //static int inner_node_insert_failed = 0;
                    //++inner_node_insert_failed;
                    insert_split(now_node, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                    ret_flag = true;
                    break;
                }
            }
        }
        
        std::swap(key_buf, new_key_buf);
        std::swap(child_buf, new_child_buf);
        new_key_buf.clear();
        new_child_buf.clear();
        now_node = now_node->parent;
    }

    /* if new child, create a new root */
    if (child_buf.size() > 0){
        add_root(key_buf.data(), child_buf.data(), child_buf.size());
    }
    t2 = std::chrono::high_resolution_clock::now();
    insert_ascend_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    AEX_PRINT("cnt=" << cnt << "insert_ascend time=" << insert_ascend_time);
    return ret_flag;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::dense_node_insert_split(inner_node_ptr node, key_type new_key, node_ptr new_child){
    AEX_ASSERT(IS_ML_NODE(node) == false);
    AEX_ASSERT(node->slot_size == traits::MIN_ML_INNER_NODE_SIZE);
    slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, new_key) - node;
    inner_node_ptr new_node = node_allocator.allocate_inner_node(node->slot_size, false);
    split_dense_node(new_node, node);
    if (pos < new_node->size){
        node->insert(new_key, new_child, pos);
    }
    else
        node->insert(new_key, new_child, pos - new_node->size);
    insert_one(new_node, node->parent);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::ml_node_insert_split(inner_node_ptr node, key_type new_key, node_ptr new_child){
    
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_split(inner_node_ptr node, key_type new_key, node_ptr new_child){
    if (!IS_ML_NODE(node)){
        dense_node_insert_split(node, new_key, new_child);
    }
    else if (check_split(node)){
        ml_node_insert_split(node, new_key, new_child);
    }
    else{
        copy_to_buffer(node, node->key_ptr, node->child_ptr);
        slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, new_key) - node->key_ptr;
        std::move_backward(node->key_ptr + pos, node->key_ptr + node->size, node->key_ptr + node->size + 1);
        std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + 1);
        ++node->size;
        if (!retrain(node))
            split(node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_one(inner_node_ptr node, key_type new_key, node_ptr new_child){
    if (node == nullptr)
        add_root(new_key, new_child);
    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    new_child->parent = node;
    if (isfull(node)){
        if (rescale(node, node->real_slot_size() << 1) == false){
            insert_split(node, &new_key, &new_child, 1);
            return true;
        }
    }
    if (!node->insert(new_key, new_child))
        insert_split(node, &new_key, &new_child);
    return false;
}

}