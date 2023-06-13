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
        pos_type old_pos = parent->at(node), pred_pos = parent->predict(key), new_pos = -1, max_slot = std::min(parent->slot_size, pred_pos + traits::ERROR_BOUND);
        for (pos_type i = pred_pos; i < max_slot; ++i){
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
            pos_type prev_pos = parent->prev_item(old_pos);
            std::fill(node_key + prev_pos + 1, node_key + old_pos + 1, key);
            return true;
        }
        else if (old_pos < new_pos){
            if (bitmap_impl::at(bm, new_pos) == 0){
                pos_type prev_pos = parent->prev_item(old_pos);
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
                pos_type prev_pos = parent->prev_item(new_pos);
                std::fill(node_key + prev_pos + 1, node_key + new_pos + 1, key);
                
                bitmap_impl::set_zero(bm, old_pos);
                bitmap_impl::set_one(bm, new_pos);
                return true;
            }
            return false;
        }
    }
    else{
        pos_type pos = parent->at(node);
        parent->key_ptr[pos] = key;
        return true;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_ptr(inner_node_ptr __restrict__ parent, const node_ptr old_node, const node_ptr new_node){
    AEX_ASSERT(old_node != parent);
    if (parent == nullptr) return false;
    if (old_node == new_node) return true;
    AEX_FORMAT("update childnode pointer old node=%p, new_node=%p, parent=%p", old_node, new_node, parent);
    pos_type pos = parent->at(old_node);
    if (pos == parent->slot_size) 
        return false;
    parent->m_stats.data_size += -old_node->data_size() + new_node->data_size();
    parent->m_stats.data_node += -old_node->data_node_size() + new_node->data_node_size();
    
    if (parent->prop & node_property::ML_NODE){
        pos_type prev_pos = parent->prev_item(pos);
        for (pos_type i = prev_pos + 1; i <= pos; ++i)
            parent->child_ptr[i] = new_node;
        if (parent->child_ptr[parent->slot_size - 1] == old_node){
            std::fill(parent->child_ptr + pos + 1, parent->child_ptr + parent->slot_size, new_node);
        }
    }
    else{
        parent->child_ptr[pos] = new_node;
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node){
    AEX_ASSERT(old_node != new_node);
    AEX_ASSERT(old_node->slot_size == new_node->slot_size);

    new_node->prev = old_node;
    new_node->next = old_node->next;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    old_node->next = new_node;

    if (head_leaf == old_node) head_leaf = new_node;

    // meta:
    {
        update_node_frequency(old_node);
        new_node->base_stats.write_times = old_node->base_stats.write_times / 2;
        old_node->base_stats.write_times /= 2;
        new_node->base_stats.train_times = old_node->base_stats.train_times / 2;
        old_node->base_stats.train_times /= 2;
        new_node->base_stats.recent_update_timestamp = old_node->base_stats.recent_update_timestamp;
    }
    
    pos_type mid = (old_node->size >> 1) | 1;
    std::move(old_node->key, old_node->key + mid, new_node->key);
    std::move(old_node->data, old_node->data + mid, new_node->data);
    std::move(old_node->key + mid, old_node->key + old_node->size, old_node->key);
    std::move(old_node->data + mid, old_node->data + old_node->size, old_node->data);

    new_node->size = mid;
    old_node->size -= mid;

    if (old_node->prop & node_property::ML_NODE){
        old_node->train_model();
    }
    if (new_node->prop & node_property::ML_NODE){
        new_node->train_model();
    }

}

// split a ordered key array with child pointers array. Support the old node firstly.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::split_with_old_node(const key_type* const __restrict__ key, const node_ptr* const __restrict__ child, const size_type n, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr __restrict__ node){
    size_type start = 0, end = n;
    inner_node_model model;
    bool replace_flag = true;
    AEX_PRINT("split with old node, node->slot_size=<<" << node->slot_size);
    if (end >= node->real_slot_size() * this->inner_node_few_ratio[node->level]){
        size_type size = static_cast<size_type>(node->real_slot_size() * this->inner_node_few_ratio[node->level]);
        if (check_rewired(key, size, node->real_slot_size(), model)){
            AEX_FORMAT("target 1 size=%lld", size);
            replace_flag = false;
            if (node->real_slot_size() >= traits::MIN_ML_INNER_NODE_SLOT_SIZE) 
                node->prop |= node_property::ML_NODE;
            node->construct(key + end - size, child + end - size, size, model);
            end -= size;
        }
    }

    split(key, child, end - start, node->level, new_key, new_child);
    if (replace_flag){
        new_key.push_back(key[n - 1]);
        new_child.push_back(node);
    }

    //meta:
    size_type m = new_child.size();
    node_ptr prev_node = node->prev, next_node = node->next;

    if (prev_node != nullptr) prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;
    if (next_node != nullptr) next_node->prev = new_child[m  - 1];
    new_child[m - 1]->next = next_node;
    for(size_type i = 0; i < m - 1; ++i){
        new_child[i + 1]->prev = new_child[i];
        new_child[i]->next = new_child[i + 1];
    }
    return replace_flag;
}

// split a ordered key array with child pointers array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, const node_ptr* const child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    inner_node_model model;
    while (start < end){
        size_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
        
        for (; slot_size < (end - start) * this->inner_node_few_ratio[level] && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
            size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), end - start);
            if (!self::check_rewired(key + start, size, slot_size, model)){
                slot_size >>= 1;
                break;
            }
        }
        while (slot_size > (end - start) * this->inner_node_few_ratio[level] && (slot_size >> 1) >= traits::MIN_INNER_NODE_SLOT_SIZE) slot_size >>= 1;
        size_type size = (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE) ? std::min(slot_size, end - start) : std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), end - start);
        //AEX_PRINT("size=" << size);
        inner_node_ptr new_node = node_allocator.allocate_inner_node(slot_size);
        ++this->m_stats.inner_node;
        new_node->level = level;
        new_node->construct(key + start, child + start, size);
        new_key.push_back(key[start + size - 1]);
        new_child.push_back(new_node);
        start += size;
    }
}

// split a ordered key array with data array to inner node array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    data_node_model model;
    while (start < end){
        size_type slot_size = traits::MIN_ML_DATA_NODE_SLOT_SIZE;
        for (; slot_size < end - start && slot_size <= traits::MAX_NODE_SLOT_SIZE; slot_size <<= 1){
            size_type size = std::min(slot_size, end - start);
            model.train(key + start, size);
            if (model.RMSE(key + start, size) >= traits::MAX_ALLOW_ERROR * log(size)){
                slot_size >>= 1;
                break;
            }
        }

        while ((slot_size >> 1) > (end - start) * traits::DATA_NODE_FULL_RATIO) slot_size >>= 1;
        slot_size = std::max(static_cast<size_type>(traits::MIN_DATA_NODE_SLOT_SIZE), slot_size);
        size_type size = std::min(slot_size, end - start);
        data_node_ptr new_node = node_allocator.allocate_data_node(slot_size);
        new_node->base_stats.recent_update_timestamp = this->m_stats.recent_update_timestamp;
        ++this->m_stats.data_node;
        new_node->construct(key + start, data + start, size);
        new_key.push_back(key[start + size - 1]);
        new_child.push_back(new_node);
        start += size;
    }
}


template<typename _Key, typename _Val, typename traits>
typename traits::pos_type aex_tree<_Key, _Val, traits>::linear_probe(const key_type* const key, const size_type n, data_node_model &m){
    //static_assert(std::is_same<data_node_model, linear_model<key_type, traits> >::value,);
    if (n <= traits::MIN_DATA_NODE_SLOT_SIZE)
        return traits::MIN_DATA_NODE_SLOT_SIZE;
    std::vector<size_type> sta1(2), sta2(2);
    int top1 = 2, top2 = 2, head1 = 1, head2 = 1;
    size_type ret = 0;
    sta1[0] = sta2[0] = 0;
    sta1[1] = sta2[1] = 1;

    pos_type max_n = static_cast<pos_type>(std::min(n, static_cast<size_type>(traits::MAX_NODE_SLOT_SIZE)));

    for (pos_type i = 2; i < max_n; ++i){
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
    pos_type start = 0, end = n;
    while (start < end){
        data_node_model model;
        pos_type ret = linear_probe(key + start, end - start, model);
        pos_type slot_size = traits::MIN_DATA_NODE_SLOT_SIZE, size;
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
bool aex_tree<_Key, _Val, traits>::check_rewired(const key_type* const key, const pos_type size, const pos_type slot_size, inner_node_model &m){
    //AEX_HINT("[check rewired]");
    if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
        return true;
    if (slot_size > traits::MAX_NODE_SLOT_SIZE)
        return false;
    pos_type start = 0;
    m.train(key, size);
    for (pos_type i = 0; i < size; ++i){            
        pos_type pos = std::max(0, std::min(static_cast<pos_type>(m.predict(key[i]) * slot_size), static_cast<pos_type>(slot_size + traits::ERROR_BOUND - 1)));
        start = std::max(start, pos);
        if (start - pos >= traits::ERROR_BOUND || start >= slot_size + traits::ERROR_BOUND){
            return false;
        }
        ++start;
    }
    return true;
}

// rewired the <key, node_ptr> array of a node. Return true if <K, P> array can be rewired. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rewired(inner_node_ptr node){
    if (node->m_stats.rewired_cnt > 0){
        return false;
    }
    node->m_stats.rewired_cnt += this->init_rewired_cnt(node);
    
    inner_node_model model;
    bool flag = true;
    if (!(node->prop & node_property::ML_NODE)) return true;
    key_type* new_key = node_allocator.allocate_key_buffer(node->size);
    node_ptr* new_child = node_allocator.allocate_nodeptr_buffer(node->size);

    copy_to_buffer(node, new_key, new_child);

    flag = check_rewired(new_key, node->size, node->real_slot_size(), model);
    if (flag) node->construct(new_key, new_child, node->size, model);
    node_allocator.deallocate(new_key);
    node_allocator.deallocate(new_child);
    return flag;
}

// Rescale a inner node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(inner_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_PRINT("RESCALE");
    
    //if (node->prop & node_property::ML_NODE)
    {
        pos_type new_slot_size = node->real_slot_size() * ratio;
        if (new_slot_size < traits::MIN_INNER_NODE_SLOT_SIZE) return false;
        if (new_slot_size > traits::MAX_NODE_SLOT_SIZE) return false;
        AEX_ASSERT(node->size <= new_slot_size);
        AEX_ASSERT(node->size >= new_slot_size * this->inner_node_few_ratio[node->level]);

        key_type* key_buffer = node_allocator.allocate_key_buffer(node->size);
        node_ptr* child_buffer =  node_allocator.allocate_nodeptr_buffer(node->size);
        copy_to_buffer(node, key_buffer, child_buffer);

        inner_node_ptr __restrict__ new_node = node_allocator.allocate_inner_node(new_slot_size);
        new_node->construct(key_buffer, child_buffer, node->size);
        replace_node(node, new_node);

        update_childnode_ptr(parent, node, new_node);
        node_allocator.free_node(node);
        --this->m_stats.inner_node;
        node = new_node;
        node_allocator.deallocate(key_buffer);
        node_allocator.deallocate(child_buffer);
    }
    AEX_PRINT("END");
    return true;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(data_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_PRINT("BEGIN");
    pos_type new_slot_size = node->slot_size * ratio;
    if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE)
        return false;
    if (new_slot_size > traits::MAX_NODE_SLOT_SIZE) return false;
    data_node_ptr new_node = node_allocator.allocate_data_node(new_slot_size);
    new_node->construct(node->key, node->data, node->size);
    replace_node(node, new_node);
    update_childnode_ptr(parent, node, new_node);
    node_allocator.free_node(node);
    --this->m_stats.data_node;
    node = new_node;
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_ASSERT(node != parent);
    if (node->prop & LEAF) 
        return rescale(static_cast<data_node_ptr>(node), parent, ratio);
    else
        return rescale(static_cast<inner_node_ptr>(node), parent, ratio);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_node(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ new_node){
    AEX_ASSERT(node != new_node);
    new_node->size = node->size;
    if (!(node->prop & node_property::ML_NODE) && !(new_node->prop & node_property::ML_NODE)){
        //AEX_FORMAT("copy node 1 " << node << " "<< new_node);
        std::copy(node->key_ptr, node->key_ptr + node->size, new_node->key_ptr);
        std::copy(node->child_ptr, node->child_ptr + node->size, new_node->child_ptr);
    }
    else if ((node->prop & node_property::ML_NODE) && (new_node->prop & node_property::ML_NODE)){
        AEX_PRINT("copy node 2");
        key_type* key_buffer = node_allocator.allocate_key_buffer(node->size);
        node_ptr* child_buffer = node_allocator.allocate_nodeptr_buffer(node->size);
        copy_to_buffer(node, key_buffer, child_buffer);
        new_node->construct(key_buffer, child_buffer, node->size);
        node_allocator.deallocate(key_buffer);
        node_allocator.deallocate(child_buffer);
    }
    else if ((node->prop & node_property::ML_NODE) && !(new_node->prop & node_property::ML_NODE)){
        AEX_PRINT("copy node 3");
        copy_to_buffer(node, new_node->key_ptr, new_node->child_ptr);
    }
    else if (!(node->prop & node_property::ML_NODE) && (new_node->prop & node_property::ML_NODE)){
        AEX_PRINT("copy node 4");
        new_node->construct(node->key_ptr, node->child_ptr, node->size);
    }
}

// merge right leaf to left leaf.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    std::move(right_node->key, right_node->size, left_node->key + left_node->size);
    std::move(right_node->data, right_node->data + right_node->size, left_node->data + left_node->size);
    left_node->size += right_node->size;
    
    if (this->allow_balance){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        left_node->base_stats.write_times += right_node->base_stats.write_times;
        left_node->base_stats.train_times += right_node->base_stats.train_times;
        left_node->size += right_node->size;
    }

    left_node->next = right_node->next;
    if (right_node->next != nullptr) right_node->next->prev = left_node;
    if (tail_leaf == right_node)
        tail_leaf = left_node;
    left_node->train_model();
}

// merge left leaf to right leaf.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    std::move_backward(right_node->key, right_node->key + right_node->size, right_node->key + left_node->size + right_node->size);
    std::move_backward(right_node->data, right_node->data + right_node->size, right_node->data + left_node->size + right_node->size);
    std::move(left_node->key, left_node->key + left_node->size, right_node->key);
    std::move(left_node->data, left_node->data + left_node->size, right_node->data);
    right_node->size += left_node->size;

    if (this->allow_balance){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        left_node->base_stats.write_times += right_node->base_stats.write_times;
        left_node->base_stats.train_times += right_node->base_stats.train_times;
        left_node->size += right_node->size;
    }

    right_node->prev = left_node->prev;
    if (left_node->prev != nullptr) left_node->prev->next = right_node;
    if (head_leaf == left_node)
        head_leaf = right_node;
    right_node->train_model();
}

// merge right inner node to left inner node. require the left inner node and right inner node must be not ML node.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->size + right_node->size > left_node->slot_size);
    std::move(right_node->key_ptr, right_node->key_ptr + right_node->size, left_node->key_ptr + left_node->size);
    std::move(right_node->child_ptr, right_node->child_ptr + right_node->size, left_node->child_ptr + left_node->size);

    if (this->allow_balance && left_node->level == 1){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        left_node->base_stats.write_times += right_node->base_stats.write_times;
        left_node->base_stats.train_times += right_node->base_stats.train_times;
        left_node->size += right_node->size;
        left_node->m_stats.data_size += right_node->m_stats.data_size;
        left_node->m_stats.data_node += right_node->m_stats.data_node;
    }

    left_node->next = right_node->next;
    if (right_node->next != nullptr) right_node->next->prev = left_node;
}

// merge left inner node to right inner node. require the left inner node and right inner node must be not ML node.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->size + right_node->size > right_node->slot_size);
    
    std::move_backward(right_node->key_ptr, right_node->key_ptr + right_node->size, right_node->key_ptr + left_node->size + right_node->size);
    std::move_backward(right_node->child_ptr, right_node->child_ptr + right_node->size, right_node->child_ptr + left_node->size + right_node->size);

    std::move(left_node->key_ptr, left_node->key_ptr + left_node->size, right_node->key_ptr);
    std::move(left_node->child_ptr, left_node->child_ptr + left_node->size, right_node->child_ptr);

    if (this->allow_balance && right_node->level == 1){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        right_node->base_stats.write_times += right_node->base_stats.write_times;
        right_node->base_stats.train_times += right_node->base_stats.train_times;
        right_node->size += left_node->size;
        right_node->m_stats.data_size += left_node->m_stats.data_size;
        right_node->m_stats.data_node += left_node->m_stats.data_node;
    }

    right_node->prev = left_node->prev;
    if (left_node->prev != nullptr) left_node->prev->next = right_node;
}

// shift one item from right leaf to left leaf
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    left_node->key[left_node->size] = right_node->key[0];
    left_node->data[left_node->size] = right_node->data[0];
    std::move(right_node->key + 1, right_node->key + right_node->size, right_node->key);
    std::move(right_node->data + 1, right_node->data + right_node->size, right_node->data);
    ++left_node->size;
    --right_node->size;
}

// shift one item from left leaf to right leaf
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    std::move_backward(right_node->key, right_node->key + right_node->size, right_node->key + right_node->size + 1);
    std::move_backward(right_node->data, right_node->data + right_node->size, right_node->data + right_node->size + 1);
    right_node->key[0] = left_node->key[left_node->size - 1];
    right_node->data[0] = std::move(left_node->data[left_node->size - 1]);
    ++right_node->size;
    --left_node->size;
}


// shift one item from right inner node to left brother, the left node must be least node, because left node will narrow if left node is ML_NODE
// left node must not be ML_NODE
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node->level == right_node->level);
    key_type shift_key = right_node->key_ptr[0];
    node_ptr shift_node = right_node->child_ptr[0];
    left_node->key_ptr[left_node->size] = shift_key;
    left_node->child_ptr[left_node->size] = shift_node;
    ++left_node->size;
    erase_child_node(right_node, shift_node);
    if (right_node->level == 1){
        if (this->allow_balance){
            left_node->m_stats.data_size += shift_node->data_size();
            right_node->m_stats.data_size -= shift_node->data_size();

            left_node->m_stats.data_node += shift_node->data_node_size();
            right_node->m_stats.data_node -= shift_node->data_node_size();

            update_node_frequency(left_node);
            update_node_frequency(right_node);
            update_node_frequency(static_cast<data_node_ptr>(shift_node));

            left_node->base_stats.write_times += shift_node->base_stats.write_times;
            right_node->base_stats.write_times -= shift_node->base_stats.write_times;
            left_node->base_stats.train_times += shift_node->base_stats.train_times;
            right_node->base_stats.train_times -= shift_node->base_stats.train_times;
        }
    }

}


// shift one item from left inner node to right brother, the left node must be least node, because right node will narrow if right node is node_property::ML_NODE,
// right node must not be ML_NODE
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    pos_type shift_pos = left_node->last();
    key_type shift_key = left_node->key_ptr[shift_pos];
    node_ptr shift_node = left_node->child_ptr[shift_pos];
    erase_child_node(left_node, shift_node);
    std::move_backward(right_node->key_ptr, right_node->key_ptr + right_node->size, right_node->key_ptr + right_node->size + 1);
    std::move_backward(right_node->child_ptr, right_node->child_ptr + right_node->size, right_node->child_ptr + right_node->size + 1);
    right_node->key_ptr[0] = shift_key;
    right_node->child_ptr[0] = shift_node;
    ++right_node->size;
    
    if (right_node->level == 1){
        if (this->allow_balance){
            left_node->m_stats.data_size -= shift_node->data_size();
            right_node->m_stats.data_size += shift_node->data_size();

            left_node->m_stats.data_node -= shift_node->data_node_size();
            right_node->m_stats.data_node += shift_node->data_node_size();
            update_node_frequency(left_node);
            update_node_frequency(right_node);
            update_node_frequency(static_cast<data_node_ptr>(shift_node));
            left_node->base_stats.write_times -= shift_node->base_stats.write_times;
            right_node->base_stats.write_times += shift_node->base_stats.write_times;
            left_node->base_stats.train_times -= shift_node->base_stats.train_times;
            right_node->base_stats.train_times += shift_node->base_stats.train_times;
        }
    }
}

// copy keys and pointers of a node to key buffer and pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* __restrict__ key_buf, node_ptr* __restrict__ child_buf){
    key_type* key = node->key_ptr;
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    pos_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (pos_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot] = key[i];
            child_buf[n_slot] = child[i];
            n_slot++;
        }
    }
    else{
        std::copy(key, key + node->size, key_buf);
        std::copy(child, child + node->size, child_buf);
    }
}

// copy keys of a node to key buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* const __restrict__ key_buf){
    key_type* key = node->key_ptr;
    bitmap bm = node->bitmap_ptr;
    pos_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (pos_type i = 0; i < node->slot_size; ++i)
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
    pos_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (pos_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            child_buf[n_slot++] = child[i];
        }
    }
    else{
        std::copy(child, child + node->size, child_buf);
    }
}

// replace new_node to old_node (contain m_stats, level, prev, next of node)
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::replace_node(const inner_node_ptr __restrict__ old_node, inner_node_ptr __restrict__ new_node){
    AEX_ASSERT(old_node != new_node);
    new_node->m_stats = old_node->m_stats;
    new_node->base_stats = old_node->base_stats;
    new_node->level = old_node->level;
    new_node->prev = old_node->prev;
    new_node->next = old_node->next;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    if (this->root == old_node)
        this->root = new_node;
}

// replace new_node to old_node (contain m_stats, level, prev, next of node)
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::replace_node(const data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node){
    AEX_ASSERT(old_node != new_node);
    new_node->base_stats = old_node->base_stats;
    new_node->level = old_node->level;
    new_node->prev = old_node->prev;
    new_node->next = old_node->next;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    if (this->root == old_node)
        this->root = new_node;
}

}