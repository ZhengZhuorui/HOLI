#pragma once

namespace aex{


template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const key_type &key){
    AEX_ASSERT(parent != node);
    if (parent == nullptr) return false;
    AEX_PRINT("[SMO] upate_childnode_key begin");
    if (IS_ML_NODE(parent)){
        node_ptr* child = parent->child_ptr;
        key_type* node_key = parent->key_ptr;
        bitmap bm = parent->bitmap_ptr;
        slot_type old_pos = parent->at(node), pred_pos = parent->predict(key), new_pos = -1, max_slot = std::min(parent->slot_size, pred_pos + traits::ERROR_BOUND);
        for (slot_type i = pred_pos; i < max_slot; ++i){
            if (i == old_pos){
                new_pos = old_pos; 
                break;
            }
            if (key < parent->key_ptr[i] && bitmap_impl::at(bm, i) == 0){
                new_pos = i;
                break;
            }
        }
        if (new_pos == -1)
            return false;
        if (old_pos == new_pos){
            slot_type prev_pos = parent->prev_item(old_pos);
            std::fill(node_key + prev_pos + 1, node_key + old_pos + 1, key);
            return true;
        }
        else if (old_pos < new_pos){
            if (bitmap_impl::at(bm, new_pos) == 0){
                slot_type prev_pos = parent->prev_item(old_pos);
                std::fill(node_key + prev_pos + 1, node_key + new_pos + 1, key);
                std::fill(child + old_pos + 1, child + new_pos + 1, node);
                bitmap_impl::set_zero(bm, old_pos);
                bitmap_impl::set_one(bm, new_pos);
                return true;
            }
            else 
                return false;
        }
        // else if (old_pos > new_pos)
        else{
            if (bitmap_impl::at(bm, new_pos) == 0){
                if (old_pos < node->slot_size - 1){
                    std::fill(node_key + new_pos + 1, node_key + old_pos + 1, node_key[old_pos + 1]);
                    std::fill(child + new_pos + 1, child + old_pos + 1, child[old_pos + 1]);
                }
                slot_type prev_pos = parent->prev_item(new_pos);
                std::fill(node_key + prev_pos + 1, node_key + new_pos + 1, key);
                
                bitmap_impl::set_zero(bm, old_pos);
                bitmap_impl::set_one(bm, new_pos);
                return true;
            }
            return false;
        }
    }
    else{
        slot_type pos = parent->at(node);
        parent->key_ptr[pos] = key;
        return true;
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(inner_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    #ifdef AEX_EXPERIMENT
    ++this->opt_stats.inner_node_split_cnt;
    #endif
    std::vector<key_type> key_buf(node->size);
    std::vector<node_ptr> child_buf(node->size);
    copy_to_buffer(node, key_buf.data(), child_buf.data());
    split(key_buf.data(), child_buf.data(), node->size, node->level, new_key, new_child);
    node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child);
    new_key.pop_back();
    new_child.pop_back();
}


// Split an data node to many nodes
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(dynamic_data_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    std::vector<key_type> new_key_2;
    std::vector<node_ptr> new_child_2;

    //split_with_linear_probe(node->key, node->data, node->size / 2, new_key, new_child);
    //split_with_linear_probe(node->key + node->size / 2, node->data + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);

    split_with_exponential_probe(node->key, node->data, node->size / 2, new_key, new_child);
    split_with_exponential_probe(node->key + node->size / 2, node->data + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);

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

// split a ordered key array with child pointers array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, bool can_retrain){
    new_key.clear();
    new_child.clear();
    inner_node_model model;
    AEX_ASSERT(level > 0);
    if (can_retrain && n <= this->max_inner_node_slot_size[level] * this->inner_node_few_ratio[level]){
        if (n < traits::MIN_ML_INNER_NODE_SIZE){
            inner_node_ptr new_node = node_allocator.allocate_inner_node(min_slot_size(n, traits::MIN_INNER_NODE_SLOT_SIZE), false);
            ++this->m_stats.level_node[level];
            new_node->level = level;
            new_node->construct(key, child, n);
            new_key.push_back(key[n - 1]);
            new_child.push_back(new_node);
            return;
        }
        else{
            size_type slot_size = min_slot_size(n, this->inner_node_few_ratio[level], traits::MIN_ML_INNER_NODE_SLOT_SIZE);
            bool flag = model.train(key, n, slot_size);
            if (flag == true && self::check_collision(key, n, slot_size, model)){
                inner_node_ptr new_node = node_allocator.allocate_inner_node(slot_size, true);
                ++this->m_stats.level_node[level];
                new_node->level = level;
                new_node->construct(key, child, n, model);
                new_key.push_back(key[n - 1]);
                new_child.push_back(new_node);
                return;
            }
        }
    }

    size_type start = 0, ans_slot_size, ans_size;
    for (; start < n; start += ans_size){
        size_type slot_size = min_slot_size(traits::MIN_ML_INNER_NODE_SIZE, this->inner_node_few_ratio[level], traits::MIN_ML_INNER_NODE_SLOT_SIZE);
        ans_slot_size = ans_size = 0;
        bool flag = false;
        for (; slot_size * this->inner_node_few_ratio[level] <= (n - start) && slot_size <= this->max_inner_node_slot_size[level]; slot_size <<= 1){
            size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), n - start);
            bool train_flag = model.train(key + start, size, slot_size);
            if (train_flag && self::check_collision(key + start, size, slot_size, model)){
                flag = true;
                ans_slot_size = slot_size;
                ans_size = size;
            }
            else 
                break;
        }
        
        if (ans_slot_size == 0) {
            ans_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
            ans_size = std::min(n - start, static_cast<size_type>(traits::MIN_INNER_NODE_SLOT_SIZE));
        }        
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size, flag);
        ++this->m_stats.level_node[level];
        new_node->level = level;
        if (IS_ML_NODE(new_node)){
            model.train(key + start, ans_size, ans_slot_size);
            new_node->construct(key + start, child + start, ans_size, model);
        }
        else
            new_node->construct(key + start, child + start, ans_size);

        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
    }
    AEX_ASSERT(start == n);
}

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    AEX_ASSERT((std::is_same<data_node, typename self::dynamic_data_node>::value == true));
    size_type start = 0;
    data_node_model model;
    new_key.clear();
    new_child.clear();

    while (start < n){
        bool ml_flag = false;
        size_type slot_size = traits::MIN_ML_DATA_NODE_SLOT_SIZE, ans_slot_size = 0, ans_size = 0;
        for (; slot_size * traits::DATA_NODE_FEW_RATIO <= n - start && slot_size <= traits::MAX_INNER_NODE_SLOT_SIZE; slot_size <<= 1){
            size_type size = std::min(slot_size, n - start);
            model.train(key + start, size);
            if (model.RMSE(key + start, size) < traits::MAX_ALLOW_ERROR * log(size)){
                ans_slot_size = slot_size;
                ans_size = size;
                ml_flag = true;
            }
            else
                break;
        }
        if (ans_slot_size == 0){
            ans_slot_size = traits::MIN_DATA_NODE_SLOT_SIZE;
            ans_size = std::min(n - start, static_cast<size_type>(traits::MIN_DATA_NODE_SLOT_SIZE));
        }
        
        dynamic_data_node_ptr new_node = node_allocator.allocate_dynamic_data_node(ans_slot_size, ml_flag);
        ++this->m_stats.level_node[0];
        
        if (IS_ML_NODE(new_node)){
            model.train(key + start, ans_size);
            new_node->construct(key + start, data + start, ans_size, model);
        }
        else
            new_node->construct(key + start, data + start, ans_size);

        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
        start += ans_size;
    }
}

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_to_static_data_node(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    AEX_ASSERT((std::is_same<data_node, typename self::static_data_node>::value == true));
    new_key.clear();
    new_child.clear();
    for (size_type i = 0; i < n; i += traits::MIN_DATA_NODE_SLOT_SIZE){
        //data_node_ptr new_node = node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
        static_data_node_ptr new_node = node_allocator.allocate_static_data_node();
        ++this->m_stats.level_node[0];
        size_type size = std::min(static_cast<size_type>(traits::MIN_DATA_NODE_SLOT_SIZE), n - i);
        new_node->construct(key + i, data + i, size);
        new_key.push_back(key[i + size - 1]);
        new_child.push_back(new_node);
    }
}

template<typename _Key, typename _Val, typename traits>
typename traits::slot_type aex_tree<_Key, _Val, traits>::linear_probe(const key_type* const key, const size_type n, data_node_model &m){
    //static_assert(std::is_same<data_node_model, linear_model<key_type, traits> >::value,);
    if (n <= traits::MIN_DATA_NODE_SLOT_SIZE)
        return n;
    std::vector<size_type> sta1(2), sta2(2);
    int top1 = 2, top2 = 2, head1 = 1, head2 = 1;
    size_type ret = 0;
    sta1[0] = sta2[0] = 0;
    sta1[1] = sta2[1] = 1;

    slot_type max_n = static_cast<slot_type>(std::min(n, static_cast<size_type>(traits::MAX_DATA_NODE_SLOT_SIZE)));

    //double ERROR = traits::MAX_ALLOW_ERROR * log(std::max(traits::MIN_ML_DATA_NODE_SLOT_SIZE, i));
    double ERROR_BOUND = traits::DATA_NODE_ERROR_BOUND;
    for (slot_type i = 2; i < max_n; ++i){       
        if (cross_product(key[0], -ERROR_BOUND, key[sta1[head1]], sta1[head1], key[i], i - ERROR_BOUND) > 0 || cross_product(key[0], ERROR_BOUND, key[sta2[head2]], sta2[head2], key[i], i + ERROR_BOUND) < 0){
            ret = i;
            break;
        }
        while (top1 > 1 && cross_product(key[sta1[top1 - 2]], sta1[top1 - 2], key[sta1[top1 - 1]], sta1[top1 - 1], key[i], i) <= 0){
            --top1;
            sta1.pop_back();
        }
        sta1.push_back(i);
        ++top1;

        while (top2 > 1 && cross_product(key[sta2[top2 - 2]], sta2[top2 - 2], key[sta2[top2 - 1]], sta2[top2 - 1], key[i], i) >= 0){
            --top2;
            sta2.pop_back();
        }
        sta2.push_back(i);
        ++top2;

        head1 = std::min(head1, top1 - 1);
        head2 = std::min(head2, top2 - 1);
        
        while (head1 < top1 - 1 && cross_product(key[0], -ERROR_BOUND, key[sta1[head1]], sta1[head1], key[sta1[head1 + 1]], sta1[head1 + 1]) < 0) ++head1;
        while (head2 < top2 - 1 && cross_product(key[0], ERROR_BOUND, key[sta2[head2]], sta2[head2], key[sta2[head2 + 1]], sta2[head2 + 1]) > 0) ++head2;

    }
    if (ret == 0)
        ret = max_n;

    m.args.end = key[ret - 1];
    m.args.slope = 1.0 / (key[ret - 1] - key[0]);
    double sum = 0;
    for (size_type i = 0; i < ret; ++i){
        sum += 1.0 * i / (ret - 1) - m.args.slope * (key[i] - key[ret - 1]);
    }
    //m.args.inter = sum / ret;
    m.args.inter = 1.0;
    return ret;
}

// split a ordered key array with data array to inner node array. Using linear probe
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_linear_probe(const key_type* const __restrict__ key, const value_type* const __restrict__ data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    AEX_ASSERT((std::is_same<data_node, typename self::dynamic_data_node>::value == true));
    size_type start = 0;
    new_key.clear();
    new_child.clear();
    while (start < n){
        data_node_model model;
        bool ml_flag = false;
        slot_type size;
        if (n - start >= traits::MIN_ML_DATA_NODE_SLOT_SIZE){
            size = linear_probe(key + start, n - start, model);
            if (size >= traits::MIN_DATA_NODE_SLOT_SIZE)
                ml_flag = true;
        }

        if (!ml_flag)
            size = static_cast<slot_type>(std::min(n - start, static_cast<size_type>(traits::MIN_DATA_NODE_SLOT_SIZE)));
        slot_type slot_size = min_slot_size(size, traits::MIN_DATA_NODE_SLOT_SIZE);

        dynamic_data_node_ptr new_node = node_allocator.allocate_dynamic_data_node(slot_size, ml_flag);
        ++this->m_stats.level_node[0];
        if (IS_ML_NODE(new_node))
            new_node->construct(key + start, data + start, size, model);
        else 
            new_node->construct(key + start, data + start, size);

        new_key.push_back(key[start + size - 1]);
        new_child.push_back(new_node);
        start += size;
    }
}

// check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, inner_node_model &m){
    if (size < traits::MIN_ML_INNER_NODE_SIZE || slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
        return true;
    if (slot_size > traits::MAX_INNER_NODE_SLOT_SIZE)
        return false;
    //if (std::is_same<inner_node_model, piecewise_linear_model<key_type, traits> >::value)
    //    return true;
    slot_type start = 0;
    for (slot_type i = 0; i < size; ++i){            
        slot_type pos = std::max(0, std::min(static_cast<slot_type>(m.predict(key[i]) * slot_size), static_cast<slot_type>(slot_size + traits::ERROR_BOUND - 1)));
        start = std::max(start, pos);
        if (start - pos >= traits::ERROR_BOUND || start >= slot_size + traits::ERROR_BOUND){
            return false;
        }
        ++start;
    }
    return true;
}

// Rescale a inner node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(inner_node_ptr node, const slot_type new_slot_size){
    AEX_PRINT("RESCALE, node=" << node << ", node size=" << node->size << ", old slot size=" << node->slot_size << ", new_slot_size=" << new_slot_size << ", node->level=" << node->level);
    #ifdef AEX_EXPERIMENT
    ++opt_stats.inner_node_rescale_cnt;
    #endif
    if (new_slot_size < traits::MIN_INNER_NODE_SLOT_SIZE || new_slot_size > traits::MAX_INNER_NODE_SLOT_SIZE)
        return false;

    node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    std::vector<key_type> key_buf(node->size);
    std::vector<node_ptr> child_buf(node->size);
    copy_to_buffer(node, key_buf.data(), child_buf.data());
    if (!IS_ML_NODE(node) && node->size >= traits::MIN_ML_INNER_NODE_SIZE)
        return false;

    bool flag = self::check_collision(key_buf.data(), node->size, new_slot_size, node->model);
    if (flag == false){
        AEX_PRINT("WARNING!");
        return false;
    }

    for (slot_type i = 0; i < node->slot_size; ++i)
        AEX_PRINT(node->key_ptr[i]<< ", " << node->child_ptr[i]);
    node_allocator.reallocate(node, new_slot_size);
    if (IS_ML_NODE(node))
        node->construct(key_buf.data(), child_buf.data(), key_buf.size(), node->model);
    AEX_PRINT("==");
    for (slot_type i = 0; i < node->slot_size; ++i)
        AEX_PRINT(node->key_ptr[i]<< ", " << node->child_ptr[i]);
    return true;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(dynamic_data_node_ptr node, const slot_type new_slot_size){
    #ifdef AEX_EXPERIMENT
    ++opt_stats.data_node_rescale_cnt;
    #endif
    AEX_ASSERT((std::is_same<data_node, static_data_node>::value));
    
    if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE || new_slot_size > traits::MAX_DATA_NODE_SLOT_SIZE)
        return false;
    node_allocator.reallocate(node, new_slot_size);
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(node_ptr node, const slot_type new_slot_size){
    if (IS_LEAF_NODE(node)) {
        if (IS_STATIC_NODE(node))
            return rescale(static_cast<dynamic_data_node_ptr>(node), new_slot_size);
    }
    else
        return rescale(static_cast<inner_node_ptr>(node), new_slot_size);
}

// copy keys and pointers of a node to key buffer and pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf){
    key_type* key = node->key_ptr;
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    slot_type n_slot = 0;
    if (IS_ML_NODE(node)){
        for (slot_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot] = key[i];
            child_buf[n_slot] = child[i];
            n_slot++;
        }
    }
    else{
        if (node->key_ptr != key_buf)
            std::copy(key, key + node->size, key_buf);
        if (node->child_ptr != child_buf)
            std::copy(child, child + node->size, child_buf);
    }
}

// copy keys of a node to key buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, key_type* const key_buf){
    key_type* key = node->key_ptr;
    bitmap bm = node->bitmap_ptr;
    slot_type n_slot = 0;
    if (IS_ML_NODE(node)){
        for (slot_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot++] = key[i];
        }
    }
    else{
        if (node->key_ptr != key_buf)
            std::copy(key, key + node->size, key_buf);
    }
}

// copy pointers of a node to pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr  node, node_ptr* child_buf){
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    slot_type n_slot = 0;
    if (IS_ML_NODE(node)){
        for (slot_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            child_buf[n_slot++] = child[i];
        }
    }
    else{
        std::copy(child, child + node->size, child_buf);
    }
}

// update 
// parent           --->        parent 
//  ...\                         ...\.
//  ....old_node                 .[node_list..., old_node]
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::link_node_list_and_replace_last_node(node_ptr node, std::vector<node_ptr> &new_child){
    //AEX_ASSERT(node->level == new_child[0]->level);
    int m = new_child.size();
    node_ptr prev_node = node->prev, next_node = node->next;
    inner_node_ptr parent = node->parent;

    if (IS_LEAF_NODE(node))
        *static_cast<data_node_ptr>(node) = std::move(*static_cast<data_node_ptr>(new_child[m - 1]));
    else
        *static_cast<inner_node_ptr>(node) = std::move(*static_cast<inner_node_ptr>(new_child[m - 1]));

    node_allocator.free_node(new_child[m - 1]);
    if (IS_LEAF_NODE(node))
        --this->m_stats.level_node[0];
    else
        --this->m_stats.level_node[static_cast<inner_node_ptr>(node)->level];
    new_child[m - 1] = node;
    for(slot_type i = 0; i < m - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }
    for (slot_type i = 0; i < m; ++i)
        new_child[i]->parent = parent;

    if (prev_node != nullptr)
        prev_node->next = new_child[0];
    if (next_node != nullptr)
        next_node->prev = new_child[m - 1];
    new_child[0]->prev = prev_node;
    new_child[m - 1]->next = next_node;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::fix_data_node(dynamic_data_node_ptr node){
    std::vector<key_type> new_key;
    std::vector<node_ptr> new_child;
    split_with_linear_probe(node->key, node->data, node->size, new_key, new_child);
    if (new_key.size() > 1){
        node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
        update_node_list_frequency(node, new_child.data(), new_child.size());
        link_node_list_and_replace_last_node(node, new_child);
        new_key.pop_back();
        new_child.pop_back();
        insert_ascend(node->parent, new_key, new_child);
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(data_node_ptr new_node, data_node_ptr old_node){
    AEX_ASSERT(traits::AllowDynamicDataNode::value == false);
    #ifdef AEX_EXPERIMENT
    ++opt_stats.data_node_split_cnt;
    #endif
    new_node->next = old_node;
    new_node->prev = old_node->prev;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    old_node->prev = new_node;
    if (head_leaf == old_node) head_leaf = new_node;
    new_node->parent = old_node->parent;
    
    size_type mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    std::move(old_node->key, old_node->key + mid, new_node->key);
    std::move(old_node->data, old_node->data + mid, new_node->data);
    std::move(old_node->key + mid, old_node->key + old_node->size ,old_node->key);
    std::move(old_node->data + mid, old_node->data + old_node->size, old_node->data);

    old_node->size = old_node->size - mid;
    new_node->size = mid;
}

}
