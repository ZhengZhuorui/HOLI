#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(inner_node_ptr node, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    std::vector<key_type> key_buf(node->size);
    std::vector<node_ptr> child_buf(node->size);
    copy_to_buffer(node, key_buf.data(), child_buf.data());
    split(key_buf.data(), child_buf.data(), node->size, node->level, new_key, new_child);
    node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
    update_node_list_frequency(node, new_child.data(), new_child.size());
    link_node_list_and_replace_last_node(node, new_child.data(), new_child.size());
    new_child.pop_back();
}


// Split an data node to many nodes
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(data_node_ptr node, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    if constexpr(traits::AllowDynamicDataNode){
        std::vector<key_type> new_key_2;
        std::vector<node_ptr> new_child_2;

        //split_with_linear_probe(node->key, node->data, node->size / 2, new_key, new_child);
        //split_with_linear_probe(node->key + node->size / 2, node->data + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);

        split_with_exponential_probe(node->key, node->data, node->size / 2, new_key, new_child);
        split_with_exponential_probe(node->key + node->size / 2, node->data + node->size / 2, node->size - node->size / 2, new_key_2, new_child_2);

        size_t new_m = new_key_2.size();
        new_key.push_back(node->key[node->size / 2]);
        for (size_t i = 0; i < new_m; ++i){
            new_key.push_back(new_key_2[i]);
            new_child.push_back(new_child_2[i]);
        }
        node->balance_stats.update_SMO_frequency(this->balance_stats.get_timestamp());
        update_node_list_frequency(node, new_child.data(), new_child.size());
        link_node_list_and_replace_last_node(node, new_child.data(), new_child.size());
        new_child.pop_back();
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::retrain(inner_node_ptr node){
    AEX_PRINT("retrain");
    node->update_SMO_frequency(this->balance_stats.get_timestamp());
    if (IS_ML_NODE(node) && node->model.train(node->key_ptr, node->size - 1, node->slot_size)){
        if (check_collision(node->key_ptr, node->size, node->slot_size, node->model)){
            node->inplace_construct();
            return true;
        }
        //else{
        //    AEX_ERROR("can't check collision");
        //}
    }
    return false;
}

template<typename _Key, typename _Val, typename traits>
std::tuple<typename traits::slot_type, typename traits::slot_type, bool> aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const size_type n, const unsigned int level){
    slot_type slot_size = min_slot_size(traits::MIN_ML_INNER_NODE_SIZE, this->inner_node_few_ratio[level], traits::MIN_INNER_NODE_SLOT_SIZE);
    slot_type ans_slot_size = 0, ans_size = 0;
    bool flag = false, train_flag;
    InnerNodeModel model;

    for (; slot_size * this->inner_node_few_ratio[level] <= n && static_cast<size_type>(slot_size) <= this->max_inner_node_slot_size[level]; slot_size <<= 1){        
        size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), n);
        train_flag = model.train(key, size - 1, slot_size);
        if (train_flag){
            if (self::check_collision(key, size - 1, slot_size, model)){
                flag = true;
                ans_slot_size = slot_size;
                ans_size = size;
            }
            else {
                break;
            }
        }
        else 
            break;
    }
    
    if (static_cast<size_type>(ans_size) < n && ans_slot_size * this->inner_node_full_ratio[level] >= n){
        train_flag = model.train(key, n - 1, ans_slot_size);
        if (train_flag){
            if (self::check_collision(key, n - 1, ans_slot_size, model))
                return std::tuple(n, ans_slot_size, true);
        }
    }
    
    //AEX_PRINT("ans_size=" << ans_size << "ans_slot_size=" << ans_slot_size);
    if (ans_slot_size == 0) {
        ans_slot_size = (n > traits::MIN_ML_INNER_NODE_SIZE) ? (traits::MIN_ML_INNER_NODE_SIZE) : min_slot_size(n, traits::MIN_INNER_NODE_SLOT_SIZE);
        ans_size = std::min(n, static_cast<size_type>(ans_slot_size));
    }        
    return std::tuple(ans_size, ans_slot_size, flag);
}

template<typename _Key, typename _Val, typename traits>
std::tuple<typename traits::slot_type, typename traits::slot_type, bool> aex_tree<_Key, _Val, traits>::split_with_exponential_probe_reverse(const key_type* const key, const size_type n, const unsigned int level){
    slot_type slot_size = min_slot_size(traits::MIN_ML_INNER_NODE_SIZE, this->inner_node_few_ratio[level], traits::MIN_INNER_NODE_SLOT_SIZE);
    slot_type ans_slot_size = 0, ans_size = 0;
    bool flag = false, train_flag;
    InnerNodeModel model;

    for (; slot_size * this->inner_node_few_ratio[level] <= n && static_cast<size_type>(slot_size) <= this->max_inner_node_slot_size[level]; slot_size <<= 1){        

        size_type size = std::min((size_type)(slot_size * this->inner_node_few_ratio[level]), n);
        train_flag = model.train(key + n - size, size - 1, slot_size);
        if (train_flag){
            if (self::check_collision(key + n - size, size - 1, slot_size, model)){
                flag = true;
                ans_slot_size = slot_size;
                ans_size = size;
            }
            else {
                break;
            }
        }
        else 
            break;
    }
    
    if (static_cast<size_type>(ans_size) < n && ans_slot_size * this->inner_node_full_ratio[level] >= n){
        train_flag = model.train(key, n - 1, ans_slot_size);
        if (train_flag){
            if (self::check_collision(key, n - 1, ans_slot_size, model))
                return std::tuple(n, ans_slot_size, true);
        }
    }
    
    if (ans_slot_size == 0) {
        ans_slot_size = (n > traits::MIN_ML_INNER_NODE_SIZE) ? (traits::MIN_ML_INNER_NODE_SIZE) : min_slot_size(n, traits::MIN_INNER_NODE_SLOT_SIZE);
        ans_size = std::min(n, static_cast<size_type>(ans_slot_size));
    }        
    return std::tuple(ans_size, ans_slot_size, flag);
}

// split a ordered key array with child pointers array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, node_ptr* child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child){
    AEX_ASSERT(level > 0);
    AEX_ASSERT(n > 0);
    size_type start = 0, ans_slot_size, ans_size;
    bool flag;
    for (; start < n; start += ans_size){ 
        std::tie(ans_size, ans_slot_size, flag) = split_with_exponential_probe(key + start, n - start, level);
        //AEX_PRINT("ans_size=" << ans_size << ", left size=" << n - start << ", ans_slot_size=" << ans_slot_size << ", timestamp=" << this->balance_stats.get_timestamp());
        if (start + ans_size < n && n - start - ans_size < traits::MIN_INNER_NODE_SLOT_SIZE / 2){
            if (flag && (ans_size / 2 > traits::MIN_ML_INNER_NODE_SIZE)){
                ans_size <<= 1;
                ans_slot_size <<= 1;
            }
            else{
                flag = false;
                ans_size = (n - start)/ 2;
                ans_slot_size = min_slot_size(ans_size, traits::MIN_INNER_NODE_SLOT_SIZE);
                //AEX_PRINT("ans_size=" << ans_size << ", left=" << n - start);
                AEX_ASSERT(ans_size >= traits::MIN_INNER_NODE_SLOT_SIZE / 2);
            }
        }
        
        inner_node_ptr new_node = allocator.allocate_inner_node(ans_slot_size, flag);
        ++this->m_stats.level_node[level];
        new_node->level = level;
        if (IS_ML_NODE(new_node)){
            new_node->model.train(key + start, ans_size - 1, ans_slot_size);
            //AEX_ASSERT(self::check_collision(key + start, ans_size - 1, ans_slot_size, new_node->model) == true);
        }
        new_node->construct(key + start, child + start, ans_size);
        if (start + ans_size < n)
            new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
    }
    AEX_ASSERT(start == n);
    //AEX_ASSERT(new_key.size() + 1 == new_child.size());
    if (new_child.size() == 1)
        SET_FLAG(new_child[0], CAN_MERGED);
}

//template<typename _Key, typename _Val, typename traits>
//void aex_tree<_Key, _Val, traits>::split_with_sample(const key_type* const key, node_ptr* child, const size_type n, const unsigned int level, std::vector<key_type> &new_key, std::vector<inner_node_ptr> &new_child){
//    new_key.clear();
//    new_child.clear();
//    AEX_ASSERT(level > 0);
//    size_type start = 0, ans_slot_size, ans_size;
//    bool flag;
//    for (; start < n; start += ans_size){ 
//        std::tie(ans_size, ans_slot_size, flag) = split_with_exponential_probe(key + start, n - start, level);
//        //AEX_PRINT("ans_size=" << ans_size << ", left size=" << n - start << ", ans_slot_size=" << ans_slot_size);
//        //if (level == 2)
//            //AEX_PRINT("ans_size=" << ans_size << ", ans_slot_size=" << ans_slot_size << ", flag=" << flag);
//        inner_node_ptr new_node = allocator.allocate_inner_node(ans_slot_size, flag);
//        ++this->m_stats.level_node[level];
//        new_node->level = level;
//        if (IS_ML_NODE(new_node)){
//            new_node->model.train(key + start, ans_size - 1, ans_slot_size);
//            AEX_ASSERT(self::check_collision(key + start, ans_size - 1, ans_slot_size, new_node->model) == true);
//        }
//        new_node->construct(key + start, child + start, ans_size);
//        if (start + ans_size < n)
//            new_key.push_back(key[start + ans_size - 1]);
//        new_child.push_back(new_node);
//    }
//    AEX_ASSERT(start == n);
//    AEX_ASSERT(new_key.size() + 1 == new_child.size());
//}

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_with_exponential_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    if (!traits::AllowDynamicDataNode)
        return;
    size_type start = 0;
    DataNodeModel model;
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
        
        data_node_ptr new_node = allocator.allocate_data_node(ans_slot_size, ml_flag);
        ++this->m_stats.level_node[0];
        
        if (IS_ML_NODE(new_node)){
            new_node->model.train(key + start, ans_size);
            new_node->construct(key + start, data + start, ans_size);
        }
        else
            new_node->construct(key + start, data + start, ans_size);

        new_key.push_back(key[start + ans_size - 1]);
        new_child.push_back(new_node);
        start += ans_size;
    }
    new_key.pop_back();
}

// split a ordered key array with data array to data nodes.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split_to_static_data_node(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    new_key.clear();
    new_child.clear();
    for (size_type i = 0; i < n; i += traits::MIN_DATA_NODE_SLOT_SIZE){
        //data_node_ptr new_node = allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
        data_node_ptr new_node = allocator.allocate_data_node();
        ++this->m_stats.level_node[0];
        size_type size = std::min(static_cast<size_type>(traits::MIN_DATA_NODE_SLOT_SIZE), n - i);
        new_node->construct(key + i, data + i, size);
        if (i + size != n)
            new_key.push_back(MID_KEY(key[i + size - 1], key[i + size]));
        new_child.push_back(new_node);
    }
    //new_key.pop_back();
}

//template<typename _Key, typename _Val, typename traits>
//void aex_tree<_Key, _Val, traits>::split_to_static_data_node_with_gap(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
//    AEX_ASSERT(traits::AllowDynamicDataNode==false);
//    if constexpr (traits::AllowMultiKey == true){
//        split_to_static_data_node(key, data, n, new_key, new_child);
//        return;
//    }
//    if (n < traits::MIN_DATA_NODE_SLOT_SIZE){
//        split_to_static_data_node(key, data, n, new_key, new_child);
//        return;
//    }
//
//    new_key.clear();
//    new_child.clear();
//    size_type slot_size = 1;
//    while (slot_size * traits::MIN_DATA_NODE_SLOT_SIZE < n) slot_size <<= 1;
//    key_type gap = (key[n - 1] - key[0]) / slot_size;
//    std::vector<key_type> gap(n);
//    for (size_type i = 0; i < n - 1; ++i) gap[i] = data[i + 1] - data[i];
//    std::sort(gap, gap + n - 1);
//    key_type standard_gap = gap[n - n / traits::MIN_DATA_NODE_SLOT_SIZE / 2];
//    for (size_type i = 0; i < n; ){
//        slot_type slot_size;
//        if (n - i <= traits::MIN_DATA_NODE_SLOT_SIZE){
//            slot_size = n - i;
//        }
//        else{
//            key_type max_gap = 0;
//            for (slot_type j = (traits::MIN_DATA_NODE_SLOT_SIZE << 1); j < traits::MIN_DATA_NODE_SLOT_SIZE; ++j)
//            if (f[i + j + 1] - f[i + j] > max_gap){
//                max_gap = f[i + j + 1] - f[i + j];
//                slot_size = j;
//            }
//            if (max_gap < standard_gap)
//                slot_size = traits::MIN_DATA_NODE_SLOT_SIZE;
//        }
//        data_node_ptr new_node = allocator.allocate_data_node();
//        ++this->m_stats.level_node[0];
//        i -= j;
//        new_node->construct(key + i, data + i, slot_size);
//        if (i + slot_size != n)
//            new_key.push_back(static_cast<key_type>(1.0 * (key[i + slot_size - 1] + key[i + slot_size]) / 2));
//        new_child.push_back(new_node);
//        i += j;
//    }
//    AEX_PRINT("node size=" << new_key.size());
//}
//
template<typename _Key, typename _Val, typename traits>
typename traits::slot_type aex_tree<_Key, _Val, traits>::linear_probe(const key_type* const key, const size_type n, DataNodeModel &m){
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
void aex_tree<_Key, _Val, traits>::split_with_linear_probe(const key_type* const key, const value_type* const data, const size_type n, std::vector<key_type> &new_key, std::vector<data_node_ptr> &new_child){
    if (!traits::AllowDynamicDataNode)
        return;
    AEX_ASSERT((std::is_same_v<data_node, typename self::dynamic_data_node>));
    size_type start = 0;
    new_key.clear();
    new_child.clear();
    while (start < n){
        DataNodeModel model;
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

        data_node_ptr new_node = allocator.allocate_data_node(slot_size, ml_flag);
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
template<typename Model>
bool aex_tree<_Key, _Val, traits>::check_collision(const key_type* const key, const slot_type size, const slot_type slot_size, Model &m){
    if (size + 1 < traits::MIN_ML_INNER_NODE_SIZE || slot_size < traits::MIN_INNER_NODE_SLOT_SIZE)
        return true;
    if (slot_size > traits::MAX_INNER_NODE_SLOT_SIZE)
        return false;
    //if (std::is_same_v<InnerNodeModel, piecewise_linear_model<key_type, traits> >)
    //    return true;
    slot_type start = 0;
    for (slot_type i = 0; i < size; ++i){            
        slot_type pos = std::max(0, static_cast<slot_type>(m.predict(key[i]) * slot_size));
        start = std::max(start, pos);
        //AEX_PRINT("start=" << start);
        if (start - pos >= traits::ERROR_BOUND || start >= slot_size + traits::ERROR_BOUND - 1){
            AEX_WARNING("size=" << size << ", i=" << i << ", start=" << start << ", slot_size=" << slot_size << ", pos=" << pos << ", key=" << key[i] << ", prev_key=" << key[i-1] << ", max error=" << m.max_error(key, size, slot_size));
            AEX_WARNING("seg_nums=" << m.args.seg_nums);
            for (unsigned int j = 0; j < m.args.seg_nums; ++j)
                AEX_WARNING("slope=" << m.args.slope[j] << ", end=" << m.args.end[j]);
            for (int j = 0; j < size; ++j){
                [[maybe_unused]]slot_type pos = std::max(0, static_cast<slot_type>(m.predict(key[j]) * slot_size));
                AEX_PRINT("key=" << key[j] << ", pred_pos=" << m.predict(key[j]) << ", pos=" << pos);
            }
            //for (unsigned int j = 0; j < m.args.seg_nums; ++j)
            //    AEX_PRINT("slope=" << m.args.slope[j] << ", end=" << m.args.end[j]);
            return false;
        }
        ++start;
    }
    return true;
}

// Rescale a inner node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::rescale(inner_node_ptr node, const slot_type new_slot_size){
    //AEX_PRINT("rescale");
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_rescale_cnt;
    #endif
    //AEX_PRINT("new_slot_size=" << new_slot_size << ", IS_ML_NODE(node)="  << IS_ML_NODE(node) << ", node->slot_size=" << node->slot_size << ", size=" << node->size);
    if (new_slot_size < traits::MIN_INNER_NODE_SLOT_SIZE || new_slot_size > traits::MAX_INNER_NODE_SLOT_SIZE)
        return false;

    if (node->size >= traits::MIN_ML_INNER_NODE_SIZE){
        bool flag = rescale_implement(node, new_slot_size);
        if (flag){
            SET_FLAG(node, node_property::ML_NODE);
            return true;
        }
        else
            return false;
    }
    else{
        allocator.reallocate_and_copy(node, new_slot_size);
        UNSET_FLAG(node, node_property::ML_NODE);
        return true;
    }
}
    
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale_implement(inner_node_ptr node, const slot_type new_slot_size){
    //AEX_WARNING("RESCALE IMPLEMENT, node=" << node << ", node size=" << node->size << ", old slot size=" << node->slot_size << ", new_slot_size=" << new_slot_size << ", node->level=" << node->level << ", IS_ML_NODE?" << IS_ML_NODE(node));
    //std::vector<key_type> key_buf(node->size);
    //std::vector<node_ptr> child_buf(node->size);
    bool ret = true;
    key_type* key_buf = allocator.allocate_key_buffer(node->size);
    node_ptr* child_buf = allocator.allocate_nodeptr_buffer(node->size);
    size_type size = node->size;
    copy_to_buffer(node, key_buf, child_buf);
    if (node->model.train(key_buf, node->size - 1, new_slot_size) == false){
        ret = false;
        goto rescale_implement_end;
    }
    if (self::check_collision(key_buf, node->size - 1, new_slot_size, node->model) == false){
        ret = false;
        goto rescale_implement_end;
    }
    SET_FLAG(node, node_property::ML_NODE);
    
    allocator.reallocate(node, new_slot_size);
    node->construct(key_buf, child_buf, size);

    rescale_implement_end:
    allocator.deallocate(key_buf);
    allocator.deallocate(child_buf);
    return ret;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(data_node_ptr node, const slot_type new_slot_size){
    if constexpr(traits::AllowDynamicDataNode){
        #ifdef AEX_DEBUG
        ++opt_stats.data_node_rescale_cnt;
        #endif

        if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE || new_slot_size > traits::MAX_DATA_NODE_SLOT_SIZE)
            return false;
        allocator.reallocate(node, new_slot_size);
        return true;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(node_ptr node, const slot_type new_slot_size){
    if (IS_LEAF_NODE(node)) {
        if (IS_STATIC_NODE(node))
            return rescale(static_cast<data_node_ptr>(node), new_slot_size);
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
        child_buf[n_slot++] = child[node->slot_size - 1];
    }
    else{
        if (node->key_ptr != key_buf)
            std::copy(key, key + node->size - 1, key_buf);
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
            std::copy(key, key + node->size - 1, key_buf);
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
        child_buf[n_slot++] = child[node->slot_size - 1];
    }
    else{
        if (node->child_ptr != child_buf)
            std::copy(child, child + node->size, child_buf);
    }
}

// update 
// parent           --->        parent 
//  ...\                         ...\.
//  ....old_node                 .[node_list..., old_node]
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::link_node_list_and_replace_last_node(node_ptr node, node_ptr* new_child, slot_type m){
    //AEX_ASSERT(node->level == new_child[0]->level);
    node_ptr prev_node = node->prev, next_node = node->next;

    if (IS_LEAF_NODE(node))
        *static_cast<data_node_ptr>(node) = std::move(*static_cast<data_node_ptr>(new_child[m - 1]));
    else
        *static_cast<inner_node_ptr>(node) = std::move(*static_cast<inner_node_ptr>(new_child[m - 1]));

    allocator.free_node(new_child[m - 1]);
    --this->m_stats.level_node[node->level];
    //if (IS_LEAF_NODE(node))
    //    --this->m_stats.level_node[0];
    //else
    new_child[m - 1] = node;
    for(slot_type i = 0; i < m - 1; ++i){
        new_child[i]->next = new_child[i + 1];
        new_child[i + 1]->prev = new_child[i];
    }

    if (prev_node != nullptr)
        prev_node->next = new_child[0];
    if (next_node != nullptr)
        next_node->prev = new_child[m - 1];
    new_child[0]->prev = prev_node;
    new_child[m - 1]->next = next_node;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(data_node_ptr new_node, data_node_ptr old_node){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    #ifdef AEX_DEBUG
    ++opt_stats.data_node_split_cnt;
    #endif
    new_node->next = old_node;
    new_node->prev = old_node->prev;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    old_node->prev = new_node;
    if (head_leaf == old_node) head_leaf = new_node;
    //new_node->parent = old_node->parent;
    
    size_type mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    std::move(old_node->key, old_node->key + mid, new_node->key);
    std::move(old_node->data, old_node->data + mid, new_node->data);
    std::move(old_node->key + mid, old_node->key + old_node->size, old_node->key);
    std::move(old_node->data + mid, old_node->data + old_node->size, old_node->data);

    old_node->size = old_node->size - mid;
    new_node->size = mid;
}

template<typename _Key, typename _Val, typename traits>
inline _Key aex_tree<_Key, _Val, traits>::split_dense_inner_node(inner_node_ptr new_node, inner_node_ptr old_node){
    AEX_ASSERT(IS_ML_NODE(old_node) == false);
    AEX_ASSERT(IS_ML_NODE(new_node) == false);
    AEX_ASSERT(old_node->slot_size == new_node->slot_size);
    key_type ret;
    old_node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    old_node->balance_stats = new_node->balance_stats = node_balance_stats(this->balance_stats.get_timestamp(), 
                                                                            old_node->balance_stats.get_SMO_times() / 2, 
                                                                            old_node->balance_stats.get_write_times() / 2);

    new_node->next = old_node;
    new_node->prev = old_node->prev;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    old_node->prev = new_node;
    
    //size_type mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    size_type mid = old_node->size >> 1;
    new_node->construct(old_node->key_ptr, old_node->child_ptr, mid);
    ret = old_node->key_ptr[mid - 1];
    //std::move(old_node->key_ptr, old_node->key_ptr + mid - 1, new_node->key_ptr);
    //std::move(old_node->child_ptr, old_node->child_ptr + mid, new_node->child_ptr);
    std::move(old_node->key_ptr + mid, old_node->key_ptr + old_node->size - 1, old_node->key_ptr);
    std::move(old_node->child_ptr + mid, old_node->child_ptr + old_node->size, old_node->child_ptr);
    old_node->size -= mid;
    new_node->size = mid;
    std::fill(old_node->key_ptr + old_node->size - 1, old_node->key_ptr + old_node->slot_size, std::numeric_limits<key_type>::max());
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::add_root(const key_type* key_buf, node_ptr* child_buf, slot_type n){
    AEX_PRINT("add root, root=" << root << ", root->size=" << root->size << ", height=" << this->m_stats.height << ", n=" << n << "tree->size=" << this->size());
    //for (unsigned int i = 0; i < this->m_stats.height; ++i)
    //    AEX_PRINT("level_node[" << i << "]=" << this->m_stats.level_node[i]);
    size_type slot_size = min_slot_size(n + 1, traits::MIN_INNER_NODE_SLOT_SIZE);
    inner_node_ptr now_inner_node = allocator.allocate_inner_node(slot_size, false);
    now_inner_node->level = this->m_stats.height;
    ++this->m_stats.level_node[this->m_stats.height];
    ++this->m_stats.height;
    now_inner_node->balance_stats.update_frequency(this->balance_stats.get_timestamp());
    now_inner_node->prev = now_inner_node->next = nullptr;
    now_inner_node->construct(key_buf, child_buf, n);
    {
        now_inner_node->key_ptr[now_inner_node->size - 1] = key_buf[n - 1];
        now_inner_node->child_ptr[now_inner_node->size] = root;
        ++now_inner_node->size;
    }
    this->root = now_inner_node;
}

}

