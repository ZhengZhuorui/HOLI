#pragma once

namespace aex{


template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const key_type &key){
    AEX_ASSERT(parent != node);
    if (parent == nullptr) return false;
    AEX_PRINT("[SMO] upate_childnode_key begin");
    if (parent->prop & node_property::ML_NODE){
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

// Split an inner node to many nodes
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(inner_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    std::vector<key_type> new_key;
    std::vector<node_ptr> new_child;
    copy_to_buffer(node, node->key_ptr, node->child_ptr);
    split(node->key_ptr, node->child_ptr, node->size, node->m_stats.level, new_key, new_child);

    int m = new_child.size();
    node_ptr prev_node = node->prev;
    for (size_type i = 0; i < m; ++i)
        new_child[i]->parent = node->parent;
    if (traits::AllowRWBalace::value){
        update_node_frequency(node);
        for(size_type i = 0; i < m; ++i){
            new_child[i]->read_times = node->read_times * (1.0 * new_child[i]->size / node->size);
            new_child[i]->write_times = node->write_times * (1.0 * new_child[i]->size / node->size);
            new_child[i]->update_times = node->update_times * (1.0 * new_child[i]->size / node->size);
        }
    }

    node = std::move(new_child[m - 1]);
    node_allocator.free(new_child[m - 1]);

    for(size_type i = 0; i < m - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }
    prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;
    new_key.pop_back();
    new_child.pop_back();
}

// split a ordered key array with child pointers array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    inner_node_model model;

    while (start < end){
        size_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE, ans_slot_size = 0, ans_size = 0;
        while (slot_size * this->inner_node_few_ratio[level] <= (end - start) && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
        slot_size >>= 1;
        if (self::check_retrain(key, n, slot_size, model)){
            ans_slot_size = slot_size;
            ans_size = n;
        }
        else{
            slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
            for (; slot_size * this->inner_node_few_ratio[level] <= (end - start) && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
                size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), end - start);
                if (self::check_retrain(key + start, size, slot_size, model)){
                    ans_slot_size = slot_size;
                    ans_size = size;
                }
                else 
                    break;
            }
        }
        //while (slot_size * this->inner_node_few_ratio[level]> (end - start) && (slot_size >> 1) >= traits::MIN_INNER_NODE_SLOT_SIZE) slot_size >>= 1;
        if (ans_slot_size == 0) {
            ans_slot_size = ans_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        }
        //AEX_PRINT("size=" << size);
        inner_node_ptr new_node = node_allocator.allocate_inner_node(ans_slot_size);
        new_node->base_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
        ++this->m_stats.inner_node;
        new_node->level = level;
        new_node->construct(key + start, child + start, ans_size);
        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
        start += ans_size;
    }
}

// split a ordered key array with data array to inner node array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    data_node_model model;
    while (start < end){
        size_type slot_size = traits::MIN_ML_DATA_NODE_SLOT_SIZE, ans_slot_size = 0, ans_size = 0;
        for (; slot_size * traits::DATA_NODE_FEW_RATIO <= end - start && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
            size_type size = std::min(slot_size, end - start);
            model.train(key + start, size);
            if (model.RMSE(key + start, size) < traits::MAX_ALLOW_ERROR * log(size)){
                ans_slot_size = slot_size;
                ans_size = size;
            }
            else
                break;
        }
        if (ans_slot_size == 0){
            ans_slot_size = ans_size = traits::MIN_DATA_NODE_SLOT_SIZE;
        }
        data_node_ptr new_node = node_allocator.allocate_data_node(ans_slot_size);
        new_node->base_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
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
    slot_type start = 0, end = n;
    while (start < end){
        data_node_model model;
        slot_type ret = linear_probe(key + start, end - start, model);
        slot_type slot_size = traits::MIN_DATA_NODE_SLOT_SIZE, size;
        while (slot_size < ret && slot_size <= traits::MAX_NODE_SLOT_SIZE) slot_size <<= 1;
        if (ret < traits::MIN_ML_DATA_NODE_SLOT_SIZE)
            size = std::min(slot_size, end - start);
        else
            size = ret;
        data_node_ptr new_node = node_allocator.allocate_data_node(slot_size);
        new_node->base_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
        ++this->m_stats.data_node;
        if (ret < traits::MIN_ML_DATA_NODE_SLOT_SIZE)
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
bool aex_tree<_Key, _Val, traits>::check_retrain(const key_type* const key, const slot_type size, const slot_type slot_size, inner_node_model &m){
    //AEX_HINT("[check retrain]");
    if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
        return true;
    if (slot_size > traits::MAX_NODE_SLOT_SIZE)
        return false;
    slot_type start = 0;
    m.train(key, size, slot_size);
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
void aex_tree<_Key, _Val, traits>::rescale(inner_node_ptr node, const double ratio){
    AEX_PRINT("RESCALE");
    
    slot_type new_slot_size = node->real_slot_size() * ratio;
    if (new_slot_size < traits::MIN_INNER_NODE_SLOT_SIZE || new_slot_size > traits::MAX_NODE_SLOT_SIZE) return false;

    std::vector<key_type> key_buf(node->size);
    copy_to_buffer(node, key_buf);
    model m;
    bool flag = self::check_retrain(key_buf, node->size, new_slot_size, m);
    if (flag == false)
        return false;

    node->slot_size = new_slot_size;
    node_allocator.reallocate(node, new_slot_size);
    node->inplace_construct(m);

    AEX_PRINT("END");
    return true;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::rescale(data_node_ptr node, const double ratio){
    slot_type new_slot_size = node->slot_size * ratio;
    if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE || new_slot_size > traits::MAX_NODE_SLOT_SIZE)
        return false;
    node_allocator.reallocate(node, new_slot_size);
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::rescale(node_ptr node, const double ratio){
    if (node->prop & LEAF) 
        return rescale(static_cast<data_node_ptr>(node), ratio);
    else
        return rescale(static_cast<inner_node_ptr>(node), ratio);
}

// copy keys and pointers of a node to key buffer and pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf){
    key_type* key = node->key_ptr;
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    slot_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
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
    if (node->prop & node_property::ML_NODE){
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
    if (node->prop & node_property::ML_NODE){
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
void aex_tree<_Key, _Val, traits>::link_node_list(node_ptr node, std::vector<node_ptr> &node_buf){
    int m = new_child.size();
    node_ptr prev_node = node->prev, next_node = node->next, parent = node->parent;
    if (node->prop & LEAF){
        static_cast<data_node>(*node) = std::move(static_cast<data_node>(*new_child[m - 1]));
    }
    else{
        static_cast<inner_node>(*node) = std::move(static_cast<inner_node>(*new_child[m - 1]));
    }
    new_child[m - 1] = node;
    node_allocator.free(new_child[m - 1]);
    for (slot_type i = 0; i < m; ++i)
        new_child[i]->parent = parent;
    for(size_type i = 0; i < m - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }
    if (prev_node != nullptr)
        prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;

    new_child[m - 1]->next = next_node;
    new_key.pop_back();
    new_child.pop_back();
}

}