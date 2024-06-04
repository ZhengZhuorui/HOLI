#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    //AEX_PRINT("insert: key=" << key);
    this->balance_stats.update_timestamp();
    //inner_node_ptr stack[traits::MAX_DEPTH];
    inner_node_ptr *stack;
    std::pair<iterator, bool> ret;

    if (root == nullptr){
        //root = head_leaf = tail_leaf = allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
        root = head_leaf = tail_leaf = allocator.allocate_data_node();
        static_cast<data_node_ptr>(root)->insert(key, value, 0);
        root->next = empty_leaf;
        empty_leaf->prev = root;
        root->prev = nullptr;
        ++m_stats.level_node[0];
        m_stats.height = 1;
        ++this->m_stats.size;
        return std::pair<iterator, bool>(iterator(head_leaf, 0), true);
    }
    
    data_node_ptr node = find_leaf_with_stack(key, stack);
    slot_type pos = node->find_lower_pos(key);

    /* find the insert position */
    if (!traits::AllowMultiKey && pos < node->size && node->key[pos] == key){
        return std::pair<iterator, bool>(iterator(node, pos), false);
    }

    /* if data node is full, split the node */
    if (isfull(node)){
        if constexpr (!traits::AllowDynamicDataNode){
            data_node_ptr new_node = allocator.allocate_data_node();
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
            key_type new_key = MID_KEY(new_node->key[new_node->size - 1], node->key[0]);
            node_ptr _ = static_cast<node_ptr>(new_node);
            insert_recursive(stack, &new_key, &_, 1);
            ret = std::pair<iterator, bool>(iter, true);
            //AEX_PRINT("node=" << iter._M_node << ", pos=" << iter.offset << ", iter key=" << iter.key() << ", key=" << key);
        }
        else {
            if (node->slot_size * 2 > traits::MAX_DATA_NODE_SLOT_SIZE || (check_split(node, true))){
                insert_split(node, key, value);
                ret = std::pair<iterator, bool>(find_iterator(key), true);
            }
            else{
                [[maybe_unused]] bool flag = rescale(node, node->slot_size << 1);
                AEX_ASSERT(flag == true);
                pos = node->insert(key, value);
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
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::false_type allow_insert_balance){
    AEX_ASSERT(node != nullptr);
    return node->insert(key, child);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr node, const key_type &key, const node_ptr child, std::true_type allow_insert_balance){
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
                allocator.free_node(node_buffer[i]);
            return true;
        }
        else return false;
    }
    return true;
}

//template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::__insert_split_bulk_load(inner_node_ptr node, const slot_type start, const key_type key, inner_node_ptr child, bool half_flag, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child){
//    #ifdef AEX_DEBUG
//    ++opt_stats.inner_node_split_bulk_load_cnt;
//    #endif
//    std::vector<key_type> new_key_2;
//    std::vector<inner_node_ptr> new_child_2;
//    new_key.push_back(key);
//    new_child.push_back(child);
//    slot_type n = node->size;
//    slot_type left_size = n / 2;
//    if (half_flag && start < left_size){
//        split(node->key_ptr + start, node->child_ptr + start, left_size - start, node->level, new_key_2, new_child_2);
//        for (unsigned int i = 0; i < new_key_2.size(); ++i){
//            new_key.push_back(new_key_2[i]);
//            new_child.push_back(new_child_2[i]);
//        }
//        new_key.push_back(node->key_ptr[left_size - 1]);
//        new_child.push_back(new_child_2[new_key_2.size()]);
//
//        split(node->key_ptr + left_size, node->child_ptr + left_size, n - left_size, node->level, new_key_2, new_child_2);
//        for (unsigned int i = 0; i < new_key_2.size(); ++i){
//            new_key.push_back(new_key_2[i]);
//            new_child.push_back(new_child_2[i]);
//        }
//        new_child.push_back(new_child_2[new_key_2.size()]);
//    }
//    else{
//        split(node->key_ptr + start, node->child_ptr + start, n - start, node->level, new_key_2, new_child_2);
//        for (unsigned int i = 0; i < new_key_2.size(); ++i){
//            new_key.push_back(new_key_2[i]);
//            new_child.push_back(new_child_2[i]);
//        }
//        new_child.push_back(new_child_2[new_key_2.size()]);
//    }
//    
//    update_node_list_frequency(node, reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
//    link_node_list_and_replace_last_node(node, reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
//    new_child.pop_back();
//    //if (new_child.size() > 0)
//    //    insert_nodes(node->parent, new_key.data(), reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
//}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__insert_split_bulk_load(const key_type* key_buf, node_ptr* child_buf, const slot_type size, const slot_type tail, const int split_size, key_type last_key, inner_node_ptr last_node, node_balance_stats &stats, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child){
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_bulk_load_cnt;
    #endif
    AEX_HINT("insert_split_bulk_load, tail=" << tail << ", size=" << size << ", split_size=" << split_size);
    slot_type level = last_node->level, his_size = 0;
    slot_type block_nums = size / split_size + (size % split_size != 0);
    //AEX_PRINT(new_key.size() << ", " << new_child.size());
    for(int i = 0; i < split_size; ++i){
    //while (start < size){
        slot_type start = i * block_nums;
        slot_type block_point = std::min(tail, (i == split_size - 1) ? size : (i + 1) * block_nums);
        if (start >= tail)
            continue;
        unsigned int his_size = new_child.size();
        split(key_buf + start, child_buf + start, block_point - start, level, new_key, new_child);

        if constexpr (traits::AllowInsertBalance)
            if (split_size > 0 && new_child.size() - his_size == 1)
                for (unsigned int i = his_size; i < new_child.size(); ++i)
                    SET_FLAG(new_child[i], CAN_MERGED);
        new_key.push_back(key_buf[block_point - 1]);
        his_size = new_child.size();
        start = block_point;
    }
    new_key.push_back(last_key);
    new_child.push_back(last_node);
    //AEX_PRINT(new_key.size() << ", " << new_child.size());
    
    //AEX_ASSERT(new_key.size() + 1 == new_child.size());
    update_node_list_frequency(stats, size, reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    his_size = new_child.size();
    node_ptr prev_node = last_node->prev;
    for (slot_type i = 0; i < his_size - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }
    if (prev_node != nullptr)
        prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;

}

// Split an node when the node insert item and (the size is larger than full ratio or no empty slot to insert)
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split_pipeline(inner_node_ptr* stack, const key_type* key, const node_ptr* child, const slot_type n){
    //AEX_PRINT("pipeline");
    AEX_ASSERT(static_cast<node_ptr>(*stack) != nullptr);
    AEX_ASSERT(static_cast<node_ptr>(*stack) != this->root);
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_pipeline_cnt;
    #endif
    inner_node_ptr node = *stack, parent = *(stack - 1);
    node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
    bool append_flag = true;
    slot_type start = 0, tail, ans_size, ans_slot_size, size;
    bool ml_flag;
    AEX_ASSERT(IS_ML_NODE(node) == true);
    AEX_ASSERT(node->size + n <= node->slot_size);
    AEX_ASSERT(child[0]->level == node->child_ptr[0]->level);
    size = node->size + n;
    key_type* key_buffer = allocator.allocate_key_buffer(size);
    node_ptr* child_buffer = allocator.allocate_nodeptr_buffer(size);
    copy_to_buffer(node, key_buffer, child_buffer);
    if (n > 0){
        slot_type pos = std::lower_bound(key_buffer, key_buffer + node->size - 1, key[0]) - key_buffer;
        std::move_backward(key_buffer + pos, key_buffer + node->size - 1, key_buffer + node->size + n - 1);
        std::move_backward(child_buffer + pos, child_buffer + node->size, child_buffer + node->size + n);
        std::copy(key, key + n, key_buffer + pos);
        std::copy(child, child + n, child_buffer + pos);
        node->size += n;
    }
    
    //bool split_flag = check_split(node);
    int split_size = check_split(node);
    slot_type block_nums = size / split_size + (size % split_size != 0);
    unsigned long long recent_update_timestamp = this->balance_stats.get_timestamp();
    node->balance_stats.update_frequency(recent_update_timestamp);
    double SMO_times = node->balance_stats.get_SMO_times(), write_times = node->balance_stats.get_write_times();
    node_balance_stats stats = node->balance_stats;
    
    //for (int i = 0; i < split_size; ++i){
    tail = size;
    for (int i = split_size - 1; i >= 0; --i){
        start = i * block_nums;
        while (tail > start){
            std::tie(ans_size, ans_slot_size, ml_flag) = split_with_exponential_probe_reverse(key_buffer + start, tail - start, node->level);
            inner_node_ptr new_node = allocator.allocate_inner_node(ans_slot_size, ml_flag);
            new_node->level = node->level;
            ++this->m_stats.level_node[new_node->level];
            if (split_size > 1)
                SET_FLAG(new_node, CAN_MERGED);
            if (ml_flag)
                new_node->model.train(key_buffer + tail - ans_size, ans_size - 1, ans_slot_size);
            //AEX_PRINT("ans_size=" << ans_size << ", ans_slot_size=" << ans_slot_size << ", flag=" << ml_flag);
            new_node->construct(key_buffer + tail - ans_size, child_buffer + tail - ans_size, ans_size);
            new_node->balance_stats = node_balance_stats(recent_update_timestamp,
                                        SMO_times * (1.0 * new_node->size / size), 
                                        write_times * (1.0 * new_node->size / size));
            if (tail == size){
                new_node->prev = node->prev;
                new_node->next = node->next;
                *node = std::move(*new_node);
                --this->m_stats.level_node[new_node->level];
                allocator.free_node(new_node);
            }
            else{
                append_flag &= (!isfull(parent));
                if (append_flag)
                    append_flag &= parent->insert(key_buffer[tail - 1], new_node);
                if (!append_flag){
                    insert_split_bulk_load(stack, key_buffer, child_buffer, size, tail - ans_size, key_buffer[tail - 1], new_node, split_size, stats);
                    return;
                }
                link_to_next_node(new_node, node);
            }
            tail -= ans_size;
        }
    }
    this->allocator.deallocate_key_buffer(key_buffer);
    this->allocator.deallocate_nodeptr_buffer(child_buffer);
}

// Split an data node to many nodes with linear_probe
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split(data_node_ptr node, const key_type key, const value_type data){
    if constexpr (traits::AllowDynamicDataNode){
        #ifdef AEX_DEBUG
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

        node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
        update_node_list_frequency(node, new_child.data(), new_child.size());
        link_node_list_and_replace_last_node(node, new_child);
        new_key.pop_back();
        new_child.pop_back();

        insert_recursive(parent, new_key.data(), new_child.data(), new_child.size());
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::build_tree(key_type* key, data_node_ptr* child, size_type n){
    AEX_PRINT("n=" << n);
    this->m_stats.height = 1;
    std::vector<key_type> key_buf, new_key_buf;
    std::vector<inner_node_ptr> child_buf, new_child_buf;
    ++this->m_stats.height;
    split(key, reinterpret_cast<node_ptr*>(child), n, this->m_stats.height - 1, key_buf, child_buf);
    size_type m = child_buf.size();
    AEX_PRINT("size=" << m);
    child_buf[0]->prev = nullptr;
    child_buf[m - 1]->next = nullptr;
    for(size_type i = 0; i < m - 1; ++i){
        child_buf[i + 1]->prev = child_buf[i];
        child_buf[i]->next = child_buf[i + 1];
    }

    while (child_buf.size() > 1){
        ++this->m_stats.height;
        split(key_buf.data(), reinterpret_cast<node_ptr*>(child_buf.data()), child_buf.size(), this->m_stats.height - 1, new_key_buf, new_child_buf);
        m = new_child_buf.size();
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
    AEX_PRINT("build tree end");
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
    this->deconstruct(this->root);
    if (nums == 0)
        return;
    this->m_stats.height = 1;
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<data_node_ptr> new_child_buf;
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

    if constexpr (traits::AllowDynamicDataNode){
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
    
    //new_key_buf.pop_back();
    if (new_child_buf.size() > 1)
        this->build_tree(new_key_buf.data(), new_child_buf.data(), new_child_buf.size());
    else{
        this->root = new_child_buf[0];
    }
}

template<typename _Key, typename _Val, typename traits>
inline std::pair<_Key, typename aex_tree<_Key, _Val, traits>::node_ptr> aex_tree<_Key, _Val, traits>::__insert_split_dense_inner_node(inner_node_ptr node, const key_type* new_key, node_ptr* new_child, const slot_type n){
    #ifdef AEX_DEBUG
    ++this->opt_stats.inner_node_split_dense_node_cnt;
    #endif
    //AEX_PRINT("insert_split_dense_inner_node, slot_size=" << node->size << ", key=" << new_key[0] << ", n=" << n);
    AEX_ASSERT(IS_ML_NODE(node) == false);
    inner_node_ptr new_node = allocator.allocate_inner_node(node->real_slot_size(), false);
    new_node->level = node->level;
    ++this->m_stats.level_node[new_node->level];
    key_type split_key = split_dense_inner_node(new_node, node);
    if (new_key[0] < split_key){
        slot_type pos = std::lower_bound(new_node->key_ptr, new_node->key_ptr + new_node->size - 1, new_key[0]) - new_node->key_ptr;
        std::move_backward(new_node->key_ptr + pos, new_node->key_ptr + new_node->size - 1, new_node->key_ptr + new_node->size - 1 + n);
        std::move_backward(new_node->child_ptr + pos, new_node->child_ptr + new_node->size, new_node->child_ptr + new_node->size + n);
        std::copy(new_key, new_key + n, new_node->key_ptr + pos);
        std::copy(new_child, new_child + n, new_node->child_ptr + pos);
        new_node->size += n;
    }
    else{
        slot_type pos = std::lower_bound(node->key_ptr, node->key_ptr + node->size - 1, new_key[0]) - node->key_ptr;
        std::move_backward(node->key_ptr + pos, node->key_ptr + node->size - 1, node->key_ptr + node->size - 1 + n);
        std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + n);
        std::copy(new_key, new_key + n, node->key_ptr + pos);
        std::copy(new_child, new_child + n, node->child_ptr + pos);
        node->size += n;
    }
    //node_ptr split_node = static_cast<node_ptr>(new_node);
    return std::make_pair(split_key, static_cast<node_ptr>(new_node));
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__insert_split_by_buffer(inner_node_ptr node, const key_type* key, node_ptr* child, const slot_type n, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child){
    #ifdef AEX_DEBUG
    ++this->opt_stats.inner_node_split_by_buffer_cnt;
    #endif
    //AEX_PRINT("insert_node_by_buffer, size=" << node->size << ", n=" << n << ", slot_size=" << node->slot_size);

    //std::vector<key_type> key_buf(node->size + n);
    //std::vector<node_ptr> child_buf(node->size + n);
    node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
    slot_type size = node->size + n;
    key_type* key_buf = allocator.allocate_key_buffer(size);
    node_ptr* child_buf = allocator.allocate_nodeptr_buffer(size);

    copy_to_buffer(node, key_buf, child_buf);
    slot_type pos = std::lower_bound(key_buf, key_buf + node->size - 1, key[0]) - key_buf;
    std::move_backward(key_buf + pos, key_buf + node->size - 1, key_buf + node->size - 1 + n);
    std::move_backward(child_buf + pos, child_buf + node->size, child_buf + node->size + n);
    std::copy(key, key + n, key_buf + pos);
    std::copy(child, child + n, child_buf + pos);    
    slot_type split_size = check_split(node);
    slot_type block_nums = size / split_size + (size % split_size != 0);
    slot_type block_point = 0, start = 0;
    for (slot_type i = 0; i < split_size; ++i){
        start = i * block_nums;
        block_point = (i == split_size - 1) ? size : (i + 1) * block_nums;
        split(key_buf + start, child_buf + start, block_point - start, node->level, new_key, new_child);
        if (block_point != size)
            new_key.push_back(key_buf[block_point - 1]);
    }
    //AEX_PRINT("size=" << size << ", new_child.size=" << new_child.size());
    AEX_ASSERT(new_key.size() + 1 == new_child.size());
    update_node_list_frequency(node->balance_stats, size, reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    link_node_list_and_replace_last_node(node, reinterpret_cast<node_ptr*>(new_child.data()), new_child.size());
    new_child.pop_back();
    //AEX_PRINT("new_child.size=" << new_child.size() << ", level=" << new_child[0]->level);
    allocator.deallocate_key_buffer(key_buf);
    allocator.deallocate_nodeptr_buffer(child_buf);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_split_helper(inner_node_ptr* stack, const key_type* new_key, node_ptr* new_child, const slot_type n){
    //AEX_PRINT("insert_split_helper");
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_cnt;
    #endif 
    inner_node_ptr node = *stack;

    if (node == root){ 
        // equal: node == root
        if (!IS_ML_NODE(node) && n < node->slot_size / 2){
            //AEX_HINT("[helper]: insert_split_dense_inner_node 0");
            insert_split_dense_inner_node(stack, new_key, new_child, n);
        }
        else{
            //AEX_HINT("[helper]: insert_split_by_buffer 1");
            insert_split_by_buffer(stack, new_key, new_child, n);
        }
        return;
    }
    
    // node != root
    if (!IS_ML_NODE(node)){
        //AEX_PRINT("size=" << node->size);
        if (n < node->slot_size / 2){
            //AEX_HINT("[helper]: insert_split_dense_inner_node 1");
            insert_split_dense_inner_node(stack, new_key, new_child, n);
        }
        else{
            //AEX_HINT("[helper]: insert_split_by_buffer 2");
            insert_split_by_buffer(stack, new_key, new_child, n);
        }
        
    }
    else{
        if (node->size + n < node->slot_size){
            //AEX_HINT("[helper]: insert_split_pipeline 1");
            insert_split_pipeline(stack, new_key, new_child, n);
        }
        else{
            //AEX_HINT("[helper]: insert_split_by_buffer 3");
            insert_split_by_buffer(stack, new_key, new_child, n);
        }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_recursive(inner_node_ptr* stack, const key_type* key_buf, node_ptr* child_buf, const slot_type n){
    inner_node_ptr node = *stack;
    if (node == nullptr){
        add_root(key_buf, child_buf, n);
        return;
    }
    node->balance_stats.update_write_frequency(this->balance_stats.get_timestamp());
    if (isfull(node, n - 1)) {
        //AEX_PRINT("node->size=" << node->size << ", IS_ML_NODE" << IS_ML_NODE(node) << ", node->slot_size=" << node->slot_size << ", n=" << n);
        if (check_split(node) > 1){
            insert_split_helper(stack, key_buf, child_buf, n);
            return;
        }
        if (expand(node) == false){
            insert_split_helper(stack, key_buf, child_buf, n);
            return;
        }
    }
    
    for (slot_type i = n - 1; i >= 0; --i){
        if (!node->insert(key_buf[i], child_buf[i])){
            if constexpr (traits::AllowInsertBalance)
                if (n == 1 && node->level >= 2 && IS_ML_NODE(node) && CAN_MERGED_NODE(child_buf[0]))
                    if (this->insert_merge(node, key_buf[0], child_buf[0]))
                        return;

            insert_split_helper(stack, key_buf, child_buf, i + 1);
            return;
        }
    }
}

}