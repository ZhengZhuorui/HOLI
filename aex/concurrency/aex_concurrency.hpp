#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array(hash_node_ptr node){
    if (node->slot_size >= traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK * 2){
        #ifdef AEX_DEBUG
        ++const_cast<self*>(this)->opt_stats.lock_array_con_cnt;
        #endif
        lock_array_con(node);
        return;
    }
    const slot_type max_slot = node->slot_size / traits::SLOT_PER_LOCK;
    for (slot_type i = 0; i < max_slot; ++i){
        node->lock_array[i].lock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XL(node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeLock();
        if (node->type == NodeType::HashNode)
            lock_array(h_n(node));
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::TUL(hash_node_ptr node, version_type &node_version, bool &need_restart){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
        if (!need_restart) lock_array(node);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::TUL(node_ptr node, version_type &node_version, bool &need_restart){
    if constexpr (traits::AllowConcurrency){
        if (node->type == NodeType::HashNode) TUL(h_n(node), node_version, need_restart);
        else node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XU(node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
    }
}

//template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::_yield(int count){
//    AEX_ASSERT(traits::AllowConcurrency);
//    if (this->hash_table.isLocked()){
//        this->solve_hash_table();
//    }
//    else if (this->construct_pool() != empty()){
//        this->solve_construct_con();
//    }
//    else if (count>3)
//        sched_yield();
//    else
//        _mm_pause();
//}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::work_concurrency(){
    ConcurrencyParams* params;
    bool flag = this->work_queue.pop(params);
    if (flag){
        switch(params->type){
            case ConcurrencyType::LockArray:{
                lock_array_unit(static_cast<LockArrayParams*>(params));
                break;
            }
            case ConcurrencyType::GetChilds :{
                get_childs_unit(static_cast<GetChildsParams*>(params));
                break;
            }
            case ConcurrencyType::ConstructSMO :{
                construct_SMO_unit(static_cast<ConstructSMOParams*>(params));
                break;
            }
            default:{
                AEX_ASSERT(0 == 1);
            }
        }
    }
    return flag;
}

template<typename traits>
struct alignas(64) _LockArrayParams : ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename traits::slot_type slot_type;
    typedef typename components::hash_node_ptr hash_node_ptr;
    _LockArrayParams():ConcurrencyParams(ConcurrencyType::LockArray){}
    hash_node_ptr node;
    slot_type start, end;
};

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array_unit(LockArrayParams *params){
    const hash_node_ptr node = params->node;
    for (slot_type i = params->start; i < params->end; ++i){
        node->lock_array[i].lock();
    }
    params->finish_flag = true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array_con(const hash_node_ptr node){
    AEX_PRINT("lock_array_con");
    const slot_type unit_size = traits::THREAD_UNIT_SIZE;
    const slot_type max_slot = node->slot_size / traits::SLOT_PER_LOCK;
    const int worker_num = max_slot / unit_size + (max_slot % unit_size != 0);
    std::vector<LockArrayParams> worker(worker_num);
    for (slot_type i = 0, pos = 0; i < worker_num; ++i, pos += unit_size){
        worker[i].node = node;
        worker[i].start = pos;
        worker[i].end = std::min(pos + unit_size, max_slot);
        bool flag = this->work_queue.bounded_push(static_cast<ConcurrencyParams*>(&worker[i]));
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (this->work_concurrency()); 
    for (int i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false) _mm_pause();
    }
    AEX_PRINT("lock_array_con finish");
}

template<typename traits>
struct alignas(64) _GetChildsParams : ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename components::key_type key_type;
    typedef typename components::node_ptr node_ptr;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename traits::slot_type slot_type;
    _GetChildsParams():ConcurrencyParams(ConcurrencyType::GetChilds){}
    hash_node_ptr node;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    slot_type start, end;
};

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs_unit(GetChildsParams *worker) const {
    const hash_node_ptr node = worker->node;
    for (int i = node->next_item(worker->start); i < worker->end; i = node->next_item(i + 1)){
        key_type key;
        node_ptr child;
        std::tie(key, child) = this->hash_table.find(node, i);
        worker->key_buf.emplace_back(key);
        worker->child_buf.emplace_back(child);
    }
    worker->finish_flag = true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs_con(const hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf) {
    AEX_WARNING("get_childs_con, node->slot_size=" << node->slot_size);
    slot_type unit_slot_size = traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK;
    const int worker_num = node->slot_size / unit_slot_size + (node->slot_size % unit_slot_size == 0);
    std::vector<GetChildsParams> worker(worker_num + 1);
    std::vector<int> pre_sum(worker_num + 1);
    for (int i = 0, pos = 0; i < worker_num; ++i, pos += unit_slot_size){
        worker[i].node = node;
        worker[i].start = pos;
        worker[i].end = std::min(pos + unit_slot_size, node->slot_size);
        bool flag = this->work_queue.bounded_push(&worker[i]);
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (this->work_concurrency() == true); 
    for (int i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false) _mm_pause();
    }
    // join
    pre_sum[0] = 0;
    for (int i = 1; i < worker_num + 1; ++i){
        pre_sum[i] = pre_sum[i - 1] + worker[i - 1].key_buf.size();
    }
    key_buf.resize(pre_sum[worker_num]);
    child_buf.resize(pre_sum[worker_num]);
    for (int i = 0; i < worker_num; ++i){
        std::copy(worker[i].key_buf.data(),   worker[i].key_buf.data()   + worker[i].key_buf.size(),   key_buf.data() +   pre_sum[i]);
        std::copy(worker[i].child_buf.data(), worker[i].child_buf.data() + worker[i].child_buf.size(), child_buf.data() + pre_sum[i]);
    }
    AEX_WARNING("get_childs_con finish");
}

template<typename traits>
struct alignas(64) _ConstructSMOParams : public ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename components::key_type key_type;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename components::node_ptr node_ptr;
    typedef typename traits::slot_type slot_type;
    _ConstructSMOParams():ConcurrencyParams(ConcurrencyType::ConstructSMO){}
    hash_node_ptr node;
    key_type* keys;
    node_ptr* childs;
    ULL n;
    slot_type start_pos, end_pos, size;
    key_type tail_key;
    node_ptr tail_node;
};

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::slot_type aex_tree<_Key, _Val, traits>::split_con(hash_node_ptr node, node_ptr &split_node, const slot_type start_pos, const slot_type end_pos, slot_type &worker_size){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(node->type != NodeType::LeafNode);
    AEX_ASSERT(split_node->type != NodeType::LeafNode);
    AEX_ASSERT(check_lock(node));
    key_type key;
    node_ptr child;
    bool is_deleted = false;
    std::vector<key_type> key_buf(0);
    std::vector<node_ptr> child_buf(0);
    slot_type prev_pos, pos, start = 0, ret = end_pos;

    if (split_node->size == 1){
        return end_pos;
    }

    XL(split_node);
    if constexpr (traits::AllowConcurrency){
        if (split_node->type == NodeType::HashNode)
            for (slot_type i = 0; i < h_n(split_node)->slot_size / traits::SLOT_PER_LOCK; ++i)
                h_n(split_node)->lock_array[i].unlock();
    }

    if (split_node->type == NodeType::HashNode){
        std::tie(key, child) = this->hash_table.find(h_n(split_node), h_n(split_node)->prev_item_find(h_n(split_node)->slot_size - 1));
        if (node->predict(key) == start_pos){
            XU(h_n(split_node));return end_pos;
        }
    }
    else{
        if (node->predict(d_n(split_node)->key_ptr[split_node->size - 1]) == start_pos){
            XU(split_node); return end_pos;
        }
    }

    get_childs(i_n(split_node), key_buf, child_buf);
    #ifdef AEX_DEBUG
    ++opt_stats.inner_node_split_cnt;
    opt_stats.inner_node_split_size += key_buf.size();
    #endif
    prev_pos = node->predict(key_buf[0]);
    AEX_ASSERT(prev_pos == start_pos);
    AEX_ASSERT(prev_pos < end_pos);
    const ULL size = key_buf.size();
    for (ULL i = 1; i < size; ++i){
        pos = node->predict(key_buf[i]);
        AEX_ASSERT(prev_pos < end_pos);
        if (pos != prev_pos){
            if (pos >= end_pos)
                break;
            if (start == 0){
                ret = pos;
                AEX_ASSERT(prev_pos == start_pos);
                if (i == 1){
                    // means all childs insert to the node dependly without the split_node, delete it.
                    is_deleted = true;
                    free_node_helper(split_node);
                    split_node = child_buf[0];
                }
                else{
                    construct(i_n(split_node), key_buf.data(), child_buf.data(), i);
                }
            }
            else{
                AEX_ASSERT(node->is_occupied(prev_pos) == false);
                ++worker_size;
                if (i - start > 1){
                    const inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, i - start);
                    __construct_insert_con(node, prev_pos, pos, key_buf[start], new_node);
                }
                else
                    __construct_insert_con(node, prev_pos, pos, key_buf[start], child_buf[start]);
            }
            start = i;
            prev_pos = pos;
        }
    }
    if (start == 0)
        ret = end_pos;
    else{
        AEX_ASSERT(prev_pos < end_pos);
        AEX_ASSERT(node->is_occupied(prev_pos) == false);
        ++worker_size;
        if (size - start > 1){
            const inner_node_ptr new_node = construct(key_buf.data() + start, child_buf.data() + start, size - start);
            __construct_insert_con(node, prev_pos, end_pos, key_buf[start], new_node);
        }
        else
            __construct_insert_con(node, prev_pos, end_pos, key_buf[start], child_buf[start]);
    }
    if (!is_deleted)
        XU(split_node);
    AEX_ASSERT(check_unlock(split_node));
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO_unit(ConstructSMOParams* worker){
    hash_node_ptr node = worker->node;
    key_type* keys = worker->keys;
    node_ptr* childs = worker->childs;
    ULL n = worker->n;
    slot_type pos, prev_pos = node->predict(keys[0]), start = 0, next_pos;
    worker->start_pos = prev_pos;
    worker->size = 0;
    for (ULL i = 0; i < n; ++i){
        pos = node->predict(keys[i]);
        if (prev_pos != pos){
            next_pos = pos;
            if (pos - prev_pos > 1 && childs[i - 1]->type != NodeType::LeafNode){
                if (childs[i - 1]->size > 1)
                    next_pos = split_con(node, childs[i - 1], prev_pos, pos, worker->size);
            }
            AEX_ASSERT(prev_pos < next_pos);
            AEX_ASSERT(node->is_occupied(prev_pos) == false);
            if (i - start > 1){
                const inner_node_ptr new_node = construct(keys + start, childs + start, i - start);
                __construct_insert_con(node, prev_pos, next_pos, keys[start], new_node);
            }
            else
                __construct_insert_con(node, prev_pos, next_pos, keys[start], childs[start]);
            ++worker->size;
            prev_pos = pos;
            start = i;
        }
    }
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(node->is_occupied(pos) == false);
    worker->end_pos = pos;
    AEX_ASSERT(worker->end_pos == node->predict(keys[n - 1]));
    worker->tail_key = keys[start];
    ++worker->size;
    if (n - start > 1){
        const inner_node_ptr new_node = construct(keys + start, childs + start, n - start);
        worker->tail_node = new_node;
    }
    else{
        worker->tail_node = childs[start];
    }
    worker->finish_flag = true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO_con(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n){
    const slot_type unit_size = traits::THREAD_UNIT_SIZE;
    int worker_num = n / unit_size + (n % unit_size != 0);
    std::vector<slot_type> block_start(worker_num + 1);
    std::vector<ConstructSMOParams> worker(worker_num + 1);
    AEX_PRINT("construct_SMO_con, n=" << n << ", worker_num=" << worker_num << ", node->slot_size=" << node->slot_size);
    AEX_ASSERT(node->predict(keys[0]) == 0);

    for (slot_type pos = 0, i = 0; i < worker_num; ++i, pos += unit_size) block_start[i] = pos;
    block_start[worker_num] = n;
    for (int i = 1; i < worker_num; ++i){
        block_start[i] = std::max(block_start[i], block_start[i - 1]);
        while (block_start[i] < (slot_type)n && node->predict(keys[block_start[i] - 1]) == node->predict(keys[block_start[i]])) ++block_start[i];
    }
    for (int i = worker_num; i > 0; --i)
    if (block_start[i] == block_start[i - 1]){
        for (int j = i; j < worker_num; ++j)
            block_start[j] = block_start[j + 1];
        std::move(block_start.data() + i + 1, block_start.data() + worker_num + 1, block_start.data() + i);
        --worker_num;
    }
    worker[worker_num].start_pos = node->slot_size;
    AEX_PRINT("worker_num=" << worker_num);
    for (int i = 0; i < worker_num; ++i){
        worker[i].node = node;
        worker[i].keys =   const_cast<key_type*>(keys + block_start[i]);
        worker[i].childs = const_cast<node_ptr*>(childs + block_start[i]);
        worker[i].n = block_start[i + 1] - block_start[i];
        AEX_ASSERT(block_start[i] < block_start[i + 1]);
        AEX_DEBUG_BLOCK({if (i > 0) AEX_ASSERT(node->predict(keys[block_start[i] - 1]) != node->predict(keys[block_start[i]]));});
        bool flag = this->work_queue.bounded_push(&worker[i]);
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (this->work_concurrency()); 

    for (int i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false) {
            this->work_concurrency(); 
        }//join
        node->size += worker[i].size;
    }
    for (int i = 0; i < worker_num; ++i){
        AEX_DEBUG_BLOCK({
        if (worker[i].end_pos >= worker[i + 1].start_pos){
            AEX_PRINT("i=" << i << ", " << worker[i].end_pos << ", next=" << worker[i + 1].start_pos);
        }
        });
        AEX_ASSERT(worker[i].end_pos < worker[i + 1].start_pos);
        __construct_insert_con(node, worker[i].end_pos, worker[i + 1].start_pos, worker[i].tail_key, worker[i].tail_node);
    }
    //this->tail_node = tail_node(node);
    node->tail_node = worker[worker_num - 1].tail_node;
    AEX_ASSERT(node->tail_node == tail_node(node));
    AEX_DEBUG_BLOCK({slot_type cnt = 0; 
        for (slot_type i = 0; i < node->slot_size; i = node->next_item(i + 1)) ++cnt; 
        AEX_ASSERT(cnt == node->size);
        for (slot_type i = 0; i < node->slot_size; i += traits::SLOT_PER_SHORTCUT){
            AEX_ASSERT(hash_table.find(node, i).second != nullptr);
        }
    });
    AEX_WARNING("construct_SMO_con finish");
}


}