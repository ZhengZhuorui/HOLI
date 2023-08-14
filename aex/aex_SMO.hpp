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
        //AEX_ASSERT(old_pos > new_pos);
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
    split(node->key_ptr, node->child_ptr, node->size, node->level, new_key, new_child);
    node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child);
    new_key.pop_back();
    new_child.pop_back();
}


// Split an data node to many nodes
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(data_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    std::vector<key_type> new_key_2;
    std::vector<node_ptr> new_child_2;

    split_with_linear_probe(node->key, node->data, node->size / 2, new_key, new_child);
    split_with_linear_probe(node->key + node->size / 2, node->data + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);

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
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0;
    inner_node_model model;

    while (start < n){
        size_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE, ans_slot_size = 0, ans_size = 0;
        if (start == 0){
            while (slot_size * this->inner_node_few_ratio[level] <= (n - start) && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
            slot_size >>= 1;
            model.train(key, n, slot_size);
            if (self::check_collision(key, n, slot_size, model)){
                ans_slot_size = slot_size;
                ans_size = n;
            }
        }
        if (ans_slot_size == 0){
            slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
            for (; slot_size * this->inner_node_few_ratio[level] <= (n - start) && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
                size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), n - start);
                model.train(key, size, slot_size);
                if (self::check_collision(key + start, size, slot_size, model)){
                    ans_slot_size = slot_size;
                    ans_size = size;
                }
                else 
                    break;
            }
        }
        if (ans_slot_size == 0) {
            ans_slot_size = ans_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        }
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size);
        ++this->m_stats.inner_node;
        ++this->m_stats.level_node[level];
        new_node->level = level;
        new_node->construct(key + start, child + start, ans_size);
        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
        start += ans_size;
    }
}

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0;
    data_node_model model;
    while (start < n){
        size_type slot_size = traits::MIN_ML_DATA_NODE_SLOT_SIZE, ans_slot_size = 0, ans_size = 0;
        if (start == 0){
            while (slot_size * traits::DATA_NODE_FEW_RATIO <= (n - start) && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
            slot_size >>= 1;
            model.train(key, n);
            if (model.RMSE(key + start, n) < traits::MAX_ALLOW_ERROR * log(n)){
                ans_slot_size = slot_size;
                ans_size = n;
            }
            else
                break;
        }
        if (ans_slot_size == 0){
            for (; slot_size * traits::DATA_NODE_FEW_RATIO <= n - start && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
                size_type size = std::min(slot_size, n - start);
                model.train(key + start, size);
                if (model.RMSE(key + start, size) < traits::MAX_ALLOW_ERROR * log(size)){
                    ans_slot_size = slot_size;
                    ans_size = size;
                }
                else
                    break;
            }
        }
        if (ans_slot_size == 0){
            ans_slot_size = ans_size = traits::MIN_DATA_NODE_SLOT_SIZE;
        }
        data_node_ptr new_node = node_allocator.allocate_data_node(ans_slot_size);
        ++this->m_stats.level_node[0];
        ++this->m_stats.data_node;
        new_node->construct(key + start, data + start, ans_size);
        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
        start += ans_size;
    }
}

template<typename _Key, typename _Val, typename traits>
typename traits::slot_type aex_tree<_Key, _Val, traits>::linear_probe(const key_type* const key, const size_type n, data_node_model &m){
    //static_assert(std::is_same<data_node_model, linear_model<key_type, traits> >::value,);
    if (n <= traits::MIN_DATA_NODE_SLOT_SIZE)
        return traits::MIN_DATA_NODE_SLOT_SIZE;
    std::vector<size_type> sta1(2), sta2(2);
    int top1 = 2, top2 = 2, head1 = 1, head2 = 1;
    size_type ret = 0;
    sta1[0] = sta2[0] = 0;
    sta1[1] = sta2[1] = 1;

    slot_type max_n = static_cast<slot_type>(std::min(n, static_cast<size_type>(traits::MAX_NODE_SLOT_SIZE)));

    for (slot_type i = 2; i < max_n; ++i){
        double ERROR = traits::MAX_LINEAR_PROBE_ALLOW_ERROR * log(std::max(traits::MIN_ML_DATA_NODE_SLOT_SIZE, i));
        
        if (cross_product(key[0], -ERROR, key[sta1[head1]], sta1[head1], key[i], i - ERROR) > 0 || cross_product(key[0], ERROR, key[sta2[head2]], sta2[head2], key[i], i + ERROR) < 0){
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
        
        while (head1 < top1 - 1 && cross_product(key[0], -ERROR, key[sta1[head1]], sta1[head1], key[sta1[head1 + 1]], sta1[head1 + 1]) < 0) ++head1;
        while (head2 < top2 - 1 && cross_product(key[0], ERROR, key[sta2[head2]], sta2[head2], key[sta2[head2 + 1]], sta2[head2 + 1]) > 0) ++head2;

    }
    if (ret == 0)
        ret = max_n;

    m.args.end = key[ret - 1];
    m.args.slope = 1.0 / (key[ret - 1] - key[0]);
    double sum = 0;
    for (size_type i = 0; i < ret; ++i){
        sum += 1.0 * i / (ret - 1) - m.args.slope * (key[i] - key[ret - 1]);
    }
    m.args.inter = sum / ret;
    return ret;
}

// split a ordered key array with data array to inner node array. Using linear probe
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_linear_probe(const key_type* const __restrict__ key, const value_type* const __restrict__ data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0;
    while (start < n){
        data_node_model model;
        slot_type size = linear_probe(key + start, n - start, model);
        slot_type slot_size = traits::MIN_DATA_NODE_SLOT_SIZE;
        while (slot_size < size && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
        
        if (size < traits::MIN_DATA_NODE_SLOT_SIZE)
            size = static_cast<slot_type>(std::min(n - start, static_cast<size_type>(slot_size)));

        data_node_ptr new_node = node_allocator.allocate_data_node(slot_size);
        ++this->m_stats.level_node[0];
        ++this->m_stats.data_node;
        if (IS_ML_NODE(new_node))
            new_node->construct(key + start, data + start, size);
        else 
            new_node->construct(key + start, data + start, size, model);
        new_key.push_back(key[start + size - 1]);
        new_child.push_back(new_node);
        start += size;
    }
}

// check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, inner_node_model &m){
    //AEX_HINT("[check retrain]");
    if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
        return true;
    if (slot_size > traits::MAX_NODE_SLOT_SIZE)
        return false;
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
    AEX_PRINT("RESCALE");
    
    if (new_slot_size < traits::MIN_INNER_NODE_SLOT_SIZE || new_slot_size > traits::MAX_NODE_SLOT_SIZE)
        return false;

    node->balance_stats.update_train_frequency(this->balance_stats.get_timestamp());
    std::vector<key_type> key_buf(node->size);
    copy_to_buffer(node, key_buf.data());
    bool flag = self::check_collision(key_buf.data(), node->size, new_slot_size, node->model);
    if (flag == false)
        return false;

    node->slot_size = new_slot_size;
    node_allocator.reallocate(node, new_slot_size);
    node->inplace_construct();
    return true;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(data_node_ptr node, const slot_type new_slot_size){
    if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE || new_slot_size > traits::MAX_NODE_SLOT_SIZE || IS_ML_NODE(node))
        return false;
    node_allocator.reallocate(node, new_slot_size);
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(node_ptr node, const slot_type new_slot_size){
    if (node->prop & LEAF) 
        return rescale(static_cast<data_node_ptr>(node), new_slot_size);
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
        if (node->child_ptr != child)
            std::copy(child, child + node->size, child_buf);
    }
}

// copy keys of a node to key buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* const __restrict__ key_buf){
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
        std::copy(key, key + node->size, key_buf);
    }
}

// copy pointers of a node to pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, node_ptr* __restrict__ child_buf){
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
    int m = new_child.size();
    node_ptr prev_node = node->prev, next_node = node->next;
    inner_node_ptr parent = node->parent;

    if (node->prop & LEAF){
        *static_cast<data_node_ptr>(node) = std::move(*static_cast<data_node_ptr>(new_child[m - 1]));
    }
    else{
        *static_cast<inner_node_ptr>(node) = std::move(*static_cast<inner_node_ptr>(new_child[m - 1]));
    }
    new_child[m - 1] = node;
    node_allocator.free_node(new_child[m - 1]);
    for(slot_type i = 0; i < m - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }
    if (prev_node != nullptr)
        prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;
    new_child[m - 1]->next = next_node;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::fix_data_node(data_node_ptr node){
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

}
