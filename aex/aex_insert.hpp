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
    
    data_node_ptr node = find_leaf(key);
    slot_type pos = node->find_lower_pos(key);

    /* find the insert position */
    if (pos < node->size && node->key[pos] == key){
        return std::pair<iterator, bool>(iterator(node, pos), false);
    }

    /* if data node is full, split the node */
    if (isfull(node)){
        //if constexpr (!traits::AllowDynamicDataNode::value){
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
            key_type new_key = new_node->key[new_node->size - 1];
            node_ptr _ = static_cast<node_ptr>(new_node);
            insert_nodes(node->parent, &new_key, &_, 1);
            ret = std::pair<iterator, bool>(iter, true);
            //AEX_PRINT("node=" << iter._M_node << ", pos=" << iter.offset << ", iter key=" << iter.key() << ", key=" << key);
        //}
        //else {
        //    dynamic_data_node_ptr _node = (dynamic_data_node_ptr)(node);
        //    if (_node->slot_size * 2 > traits::MAX_DATA_NODE_SLOT_SIZE || (check_split(_node, true))){
        //        insert_split(_node, key, value);
        //        ret = std::pair<iterator, bool>(find_iterator(key), true);
        //    }
        //    else{
        //        [[maybe_unused]] bool flag = rescale(_node, _node->slot_size << 1);
        //        AEX_ASSERT(flag == true);
        //        pos = _node->insert(key, value);
        //        ret = std::pair<iterator, bool>(iterator(node, pos), true);
        //    }
        //}
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
void aex_tree<_Key, _Val, traits>::insert_split_bulk_load(inner_node_ptr node, const slot_type start, const key_type key, node_ptr child, bool half_flag){
    //AEX_WARNING("insert_split_bulk_load");
    std::vector<key_type> new_key, new_key_2;
    std::vector<node_ptr> new_child, new_child_2;
    new_key.push_back(key);
    new_child.push_back(child);
    slot_type n = node->size;
    slot_type left_size = n / 2;
    if (half_flag && start < left_size){
        AEX_WARNING("?");
        split(node->key_ptr + start, node->child_ptr + start, left_size - start, node->level, new_key_2, new_child_2);
        for (unsigned int i = 0; i < new_key_2.size(); ++i){
            new_key.push_back(new_key_2[i]);
            new_child.push_back(new_child_2[new_key_2.size()]);
        }
        new_key.push_back(node->key_ptr[left_size - 1]);
        new_child.push_back(new_child_2[new_key_2.size()]);

        split(node->key_ptr + left_size, node->child_ptr + left_size, n - left_size, node->level, new_key_2, new_child_2);
        for (unsigned int i = 0; i < new_key_2.size(); ++i){
            new_key.push_back(new_key_2[i]);
            new_child.push_back(new_child_2[i]);
        }
        new_child.push_back(new_child_2[new_key_2.size()]);
    }
    else{
        split(node->key_ptr + start, node->child_ptr + start, n - start, node->level, new_key_2, new_child_2);
        for (unsigned int i = 0; i < new_key_2.size(); ++i){
            new_key.push_back(new_key_2[i]);
            new_child.push_back(new_child_2[i]);
        }
        new_child.push_back(new_child_2[new_key_2.size()]);
    }
    
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child.data(), new_child.size());
    new_child.pop_back();
    if (new_child.size() > 0)
        insert_nodes(node->parent, new_key.data(), new_child.data(), new_child.size());
}

// Split an node when the node insert item and (the size is larger than full ratio or no empty slot to insert)
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_split_pipeline(inner_node_ptr node, const key_type* key, const node_ptr* child, const slot_type n){    
    //AEX_PRINT("insert_split pipeline");
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_split_cnt;
    #endif
    inner_node_ptr parent = node->parent;
    bool append_flag = (parent != nullptr);
    AEX_ASSERT(IS_ML_NODE(node) == true);
    AEX_ASSERT(node->size + n <= node->slot_size);
    copy_to_buffer(node, node->key_ptr, node->child_ptr);
    //for (slot_type i = 0; i < node->slot_size; ++i)
    //    AEX_PRINT(node->child_ptr[i]);
    if (n > 0){
        slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, key[0]) - node->key_ptr;
        std::move_backward(node->key_ptr + pos, node->key_ptr + node->size - 1, node->key_ptr + node->size + n - 1);
        std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + n);
        std::copy(key, key + n, node->key_ptr + pos);
        std::copy(child, child + n, node->child_ptr + pos);
        node->size += n;
    }
    slot_type start = 0, ans_size, ans_slot_size, split_node_cnt = 0, size = node->size;
    bool ml_flag;

    unsigned long long recent_udpate_timestamp = node->balance_stats.get_recent_update_timestamp();
    double train_times = node->balance_stats.get_train_times(), write_times = node->balance_stats.get_write_times();
    
    if (check_split(node)){
        #ifdef AEX_EXPERIMENT
        ++opt_stats.inner_node_balance_split_cnt;
        #endif
        slot_type left_size = size / 2;
        while (start < left_size){
            std::tie(ans_size, ans_slot_size, ml_flag) = split_with_exponential_probe(node->key_ptr + start, left_size - start, node->level);
            split_node_cnt++;
            inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
            if (ml_flag)
                new_node->model.train(node->key_ptr + start, ans_size - 1, ans_slot_size);
            new_node->construct(node->key_ptr + start, node->child_ptr + start, ans_size);
            new_node->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                        train_times, 
                                        write_times * (1.0 * new_node->size / node->size));

            append_flag &= (!isfull(parent));
            if (append_flag)
                append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], node);
            if (!append_flag){
                insert_split_bulk_load(node, start, node->key_ptr[start + ans_size - 1], new_node, 1);
                return;
            }
            link_to_next_node(new_node, node);
            start += ans_size;
        }
    }

    while (start < size){
        std::tie(ans_size, ans_slot_size, ml_flag) = split_with_exponential_probe(node->key_ptr + start, size - start, node->level);
        //AEX_PRINT("ans_size=" << ans_size << ", ans_slot_size=" << ans_slot_size << ", ml_flag=" << ml_flag << ", start=" << start << ", size=" << size);
        split_node_cnt++;
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, ml_flag);
        new_node->level = node->level;
        if (ml_flag)
            new_node->model.train(node->key_ptr + start, ans_size - 1, ans_slot_size);
        new_node->construct(node->key_ptr + start, node->child_ptr + start, ans_size);
        if (ans_size != size){
            new_node->balance_stats = node_balance_stats(recent_udpate_timestamp,
                                            train_times, 
                                            write_times * (1.0 * new_node->size / node->size));
        }
        else{
            new_node->balance_stats = node->balance_stats;
            node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
        }
        if (start + ans_size == size){
            node_ptr prev = node->prev, next = node->next;
            *node = std::move(*new_node);
            node->parent = parent;
            node->prev = prev;
            node->next = next;
            split_node_cnt--;
        }
        else{
            append_flag &= (!isfull(parent));
            if (append_flag)
                append_flag &= parent->insert(node->key_ptr[start + ans_size - 1], new_node);
            
            if (!append_flag) {
                insert_split_bulk_load(node, start + ans_size, node->key_ptr[start + ans_size - 1], new_node, false);
                return;
            }
            link_to_next_node(new_node, node);
        }
        start += ans_size;
    }
}

// Split an data node to many nodes with linear_probe
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_split(dynamic_data_node_ptr node, const key_type key, const value_type data){

    #ifdef AEX_EXPERIMENT
    ++opt_stats.data_node_split_cnt;
    #endif
    inner_node_ptr parent = node->parent;
    std::vector<key_type> key_buf(node->size + 1), new_key, new_key_2;
    std::vector<value_type> data_buf(node->size + 1);
    std::vector<node_ptr> new_child, new_child_2;

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

    insert_nodes(parent, new_key.data(), new_child.data(), new_child.size());
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::build_tree(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    AEX_ASSERT(key_buf.size() + 1 == child_buf.size());
    AEX_PRINT("build tree");
    this->m_stats.height = 1;
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    while (child_buf.size() > 1){    
        //AEX_PRINT("height++");
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
//template<typename _Key, typename _Val, typename traits>
//bool aex_tree<_Key, _Val, traits>::insert_ascend(inner_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
//    //AEX_PRINT("insert_ascend");
//    
//    //std::chrono::system_clock::time_point t1, t2;
//    //t1 = std::chrono::high_resolution_clock::now();
//    //static double insert_ascend_time = 0;
//    //static int cnt = 0;
//    //++cnt;
//
//    if (child_buf.size() == 0) 
//        return false;
//    AEX_ASSERT(key_buf.size() == child_buf.size());
//    std::vector<key_type> new_key_buf;
//    std::vector<node_ptr> new_child_buf;
//    inner_node_ptr now_node = node;
//    bool ret_flag = false;
//    while (now_node != nullptr && child_buf.size() > 0){
//        size_t num_buf = child_buf.size();
//        bool split_flag = false;
//        now_node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
//        if (isfull(now_node, num_buf - 1)) {
//            if (rescale(now_node, now_node->real_slot_size << 1) == false){
//                insert_split_pipeline(now_node, key_buf.data(), child_buf.data(), num_buf);
//                split_flag = true;
//                ret_flag = true;
//            }
//        }
//
//        if (split_flag == false){
//            for (size_t i = 0; i < num_buf; ++i){
//                if (!now_node->insert(key_buf[i], child_buf[i])){
//                    //static int inner_node_insert_failed = 0;
//                    //++inner_node_insert_failed;
//                    insert_split(now_node, key_buf.data() + i, child_buf.data() + i, num_buf - i);
//                    ret_flag = true;
//                    break;
//                }
//            }
//        }
//        
//        std::swap(key_buf, new_key_buf);
//        std::swap(child_buf, new_child_buf);
//        new_key_buf.clear();
//        new_child_buf.clear();
//        now_node = now_node->parent;
//    }
//
//    /* if new child, create a new root */
//    if (child_buf.size() > 0){
//        add_root(key_buf.data(), child_buf.data(), child_buf.size());
//    }
//    return ret_flag;
//}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split_dense_inner_node(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
    #ifdef AEX_EXPERIMENT
    ++this->opt_stats.inner_node_split_dense_node_cnt;
    #endif
    AEX_PRINT("insert_split_dense_inner_node");
    AEX_ASSERT(IS_ML_NODE(node) == false);
    inner_node_ptr new_node = node_allocator.allocate_inner_node(node->real_slot_size(), false);
    key_type split_key = split_dense_inner_node(new_node, node);
    if (new_key[0] < split_key){
        slot_type pos = std::lower_bound(new_node->key_ptr, new_node->key_ptr + new_node->size - 1, new_key[0]) - new_node->key_ptr;
        std::move_backward(new_node->key_ptr + pos, new_node->key_ptr + new_node->size - 1, new_node->key_ptr + new_node->size - 1 + n);
        std::move_backward(new_node->child_ptr + pos, new_node->child_ptr + new_node->size, new_node->child_ptr + new_node->size + n);
        std::copy(new_key, new_key + n, new_node->key_ptr + pos);
        std::copy(new_child, new_child + n, new_node->child_ptr + pos);
        for (slot_type i = 0; i < n; ++i)
            new_child[i]->parent = new_node;
        new_node->size += n;
    }
    else{
        slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, new_key[0]) - node->key_ptr;
        std::move_backward(node->key_ptr + pos, node->key_ptr + node->size - 1, node->key_ptr + node->size - 1 + n);
        std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + n);
        std::copy(new_key, new_key + n, node->key_ptr + pos);
        std::copy(new_child, new_child + n, node->child_ptr + pos);
        for (slot_type i = 0; i < n; ++i)
            new_child[i]->parent = node;
        node->size += n;
    }
    node_ptr _ = static_cast<node_ptr>(new_node);
    insert_nodes(node->parent, &split_key, &_, 1);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split_by_buffer(inner_node_ptr node, const key_type* key, node_ptr* child, const slot_type n, bool no_split){
    #ifdef AEX_EXPERIMENT
    ++this->opt_stats.inner_node_split_by_buffer_cnt;
    #endif
    //AEX_PRINT("insert_split_by_buffer");
    std::vector<key_type> key_buf(node->size + n), new_key;
    std::vector<node_ptr> child_buf(node->size + n), new_child;

    copy_to_buffer(node, key_buf.data(), child_buf.data());
    slot_type pos = std::lower_bound(key_buf.data(), key_buf.data() + node->size - 1, key[0]) - key_buf.data();
    std::move_backward(key_buf.data() + pos, key_buf.data() + node->size - 1, key_buf.data() + node->size - 1 + n);
    std::move_backward(child_buf.data() + pos, child_buf.data() + node->size, child_buf.data() + node->size + n);
    std::copy(key, key + n, key_buf.data() + pos);
    std::copy(child, child + n, child_buf.data() + pos);
    slot_type m = node->size + n;
    if (!no_split && check_split(node)){
        std::vector<key_type> new_key_2;
        std::vector<node_ptr> new_child_2;
        int left_size = m / 2;
        split(key_buf.data(), child_buf.data(), left_size, node->level, new_key, new_child);
        new_key.push_back(key_buf[left_size - 1]);
        split(key_buf.data() + left_size, child_buf.data() + left_size, m - left_size, node->level, new_key_2, new_child_2);
        for (unsigned int i = 0; i < new_key_2.size(); ++i){
            new_key.push_back(new_key_2[i]);
            new_child.push_back(new_child_2[i]);
        }
        new_child.push_back(new_child_2[new_key_2.size()]);
    }
    else
        split(key_buf.data(), child_buf.data(), m, node->level, new_key, new_child);
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child.data(), new_child.size());
    new_child.pop_back();
    if (new_child.size() > 0)
        insert_nodes(node->parent, new_key.data(), new_child.data(), new_child.size());
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split_helper(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
    //AEX_PRINT("insert_split_helper");
    if (node == root){
        insert_split_by_buffer(node, new_key, new_child, n);
        return;
    }
    if (!IS_ML_NODE(node)){
        if (node->slot_size * traits::EXPAND_RATIO < traits::MIN_ML_INNER_NODE_SIZE && n < node->slot_size / 2)
            insert_split_dense_inner_node(node, new_key, new_child, n);
        else
            insert_split_by_buffer(node, new_key, new_child, n);
    }
    else if (node->size + n < node->slot_size){
        insert_split_pipeline(node, new_key, new_child, n);
    }
    else{
        insert_split_by_buffer(node, new_key, new_child, n);
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_nodes(inner_node_ptr node, const key_type* key_buf, node_ptr* child_buf, const slot_type n){
    if (node == nullptr){
        add_root(key_buf, child_buf, n);
        return true;
    }
    if (isfull(node, n - 1)) {
        if (rescale(node, node->real_slot_size() << 1) == false){
            insert_split_helper(node, key_buf, child_buf, n);
            return true;
        }
    }
    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    for (slot_type i = 0; i < n; ++i){
        if (!node->insert(key_buf[i], child_buf[i])){
            insert_split_helper(node, key_buf + i, child_buf + i, n - i);
            return true;
        }
    }
    return false;
}

}