#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array(hash_node_ptr node){
    if (node->slot_size >= traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK * 2){
        #ifdef AEX_DEBUG
        ++const_cast<self*>(this)->opt_stats.lock_array_con_cnt;
        #endif
        lock_array_con(node);
        AEX_ASSERT(test_lock_array_con(node));
        return;
    }
    slot_type max_slot = node->slot_size;
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
inline void aex_tree<_Key, _Val, traits>::XU(hash_node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
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
            }
            case ConcurrencyType::GetChilds :{
                get_childs_unit(static_cast<GetChildsParams*>(params));
                break;
            }
            case ConcurrencyType::ConstructSMO :{
                construct_SMO_unit(static_cast<ConstructSMOParams*>(params));
                break;
            }
            //case ConcurrencyType::HashTableRescale :{
            //    this->hash_table.rescale_unit(static_cast<HashTableRescaleParams*>(params));
            //    break;
            //}
            default:{
                AEX_ASSERT(0 == 1);
            }
        }
        
    }
    return false;
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
    for (slot_type i = params->start; i < params->end; ++i)
        node->lock_array[i].lock();
    params->finish_flag = true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array_con(const hash_node_ptr node){
    const slot_type unit_size = traits::THREAD_UNIT_SIZE;
    const slot_type max_slot = node->slot_size / traits::SLOT_PER_LOCK;
    const size_t worker_num = max_slot / unit_size + (max_slot % unit_size == 0);
    int cnt = 0;
    std::vector<GetChildsParams> worker(worker_num + 1);
    for (int i = 0; i < node->slot_size; i += unit_size, ++cnt){
        worker[i].node = node;
        worker[i].start = i;
        worker[i].end = std::min(i + unit_size, max_slot);
        bool flag = this->work_queue.bounded_push(static_cast<ConcurrencyParams*>(&worker[i]));
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (work_concurrency() == true); 
    for (size_t i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false)
            _mm_pause();
    }
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
    for (int i = node->next_item(worker->start); i < worker->end; i = node->next_item(i)){
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
    //slot_type unit_slot_size = std::max(node->slot_size / this->thread_num, traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK);
    slot_type unit_slot_size = traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK;
    const size_t worker_num = node->slot_size / unit_slot_size + (node->slot_size % unit_slot_size == 0);
    int cnt = 0;
    std::vector<GetChildsParams> worker(worker_num + 1);
    std::vector<int> pre_sum(worker_num);
    for (int i = 0; i < node->slot_size; i += unit_slot_size, ++cnt){
        //std::function<void()> t = std::bind(self::get_childs_unit, this, node, worker_key[i], worker_childs[i], i, std::min(i + unit_slot_size, node->slot_size));
        worker[i].node = node;
        worker[i].start = i;
        worker[i].end = std::min(i + unit_slot_size, node->slot_size);
        bool flag = this->work_queue.bounded_push(&worker[i]);
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (work_concurrency() == true); 
    for (size_t i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false) _mm_pause();
    }
    // join
    pre_sum[0] = 0;
    for (size_t i = 1; i < worker_num; ++i)
        pre_sum[i] = pre_sum[i - 1] + worker[i].key_buf.size();
    key_buf.resize(pre_sum[worker_num - 1]);
    child_buf.resize(pre_sum[worker_num - 1]);
    for (size_t i = 0; i < worker_num; ++i){
        std::copy(worker[i].key_buf.data(), worker[i].key_buf.data() + worker[i].key_buf.size(), key_buf.data() + pre_sum[i]);
        std::copy(worker[i].child_buf.data(), worker[i].child_buf.data() + worker[i].child_buf.size(), child_buf.data() + pre_sum[i]);
    }
}

template<typename traits>
struct alignas(64) _ConstructSMOParams : public ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename components::key_type key_type;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename components::node_ptr node_ptr;
    typedef typename traits::slot_type slot_type;
    _ConstructSMOParams():ConcurrencyParams(ConcurrencyType::GetChilds){}
    hash_node_ptr node;
    key_type* keys;
    node_ptr* childs;
    ULL n;
    slot_type start_pos, end_pos;
    key_type tail_key;
    node_ptr tail_node;
};

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO_unit(ConstructSMOParams* worker){
    hash_node_ptr node = worker->node;
    key_type* keys = worker->keys;
    node_ptr* childs = worker->childs;
    ULL n = worker->n;
    slot_type pos, prev_pos = node->predict(keys[0]), start = 0, next_pos;
    worker->start_pos = prev_pos;
    for (ULL i = 0; i < n; ++i){
        pos = node->predict(keys[i]);
        if (prev_pos != pos){
            next_pos = pos;
            if (pos - prev_pos > 1 && childs[i - 1]->type != NodeType::LeafNode){
                if (childs[i - 1]->size > 1)
                    next_pos = split(node, childs[i - 1], prev_pos, pos);
            }
            AEX_ASSERT(prev_pos < next_pos);
            AEX_ASSERT(node->is_occupied(prev_pos) == false);
            if (i - start > 1){
                const inner_node_ptr new_node = construct(keys + start, childs + start, i - start);
                __construct_insert(node, prev_pos, next_pos, keys[start], new_node);
            }
            else
                __construct_insert(node, prev_pos, next_pos, keys[start], childs[start]);
            prev_pos = pos;
            start = i;
        }
    }
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(node->is_occupied(pos) == false);
    worker->end_pos = pos;
    worker->tail_key = keys[start];
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
    slot_type unit_size = std::min(n / this->thread_num, traits::THREAD_UNIT_SIZE);
    ULL worker_num = n / unit_size + (node->slot_size % unit_size != 0);
    int cnt = 0;
    std::vector<slot_type> block_start(worker_num + 1);
    std::vector<ConstructSMOParams> worker(worker_num + 1);
    worker[worker_num].start_pos = node->slot_size;

    for (slot_type pos = 0, i = 0; i < worker_num; ++i, pos += unit_size) block_start[i] = pos;
    block_start[worker_num] = n;
    for (int i = 1; i < worker_num; ++i){
        block_start[i] = std::max(block_start[i], block_start[i - 1]);
        while (block_start[i] < n && node->predict(block_start[i] - 1) == node->predict(block_start[i])) ++block_start[i];
    }
    for (int i = worker_num; i > 0; --i)
    if (block_start[i] == block_start[i - 1]){
        for (int j = i; j < worker_num; ++j)
            block_start[j] = block_start[j + 1];
        std::move(block_start + i + 1, block_start + i + worker_num, block_start + i + worker_num);
        --worker_num;
    }
    
    for (slot_type i = 0; i < worker_num; ++i){
        worker[i].node = node;
        worker[i].keys = keys + block_start[i];
        worker[i].childs = childs + block_start[i];
        worker[i].n = block_start[i + 1] - block_start[i];
        AEX_ASSERT(block_start[i] < block_start[i + 1]);
        bool flag = this->work_queue.bouned_push(&worker[i]);
        while (!flag) {
            this->work_concurrency();
            flag = this->work_queue.bounded_push(&worker[i]);
        }
    }
    while (this->work_concurrency()); 
    for (int i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag == false) {
            while (this->work_concurrency()); 
            _mm_pause(); 
        }//join
        this->size += worker[i].size;
    }
    
    for (int i = 0; i < worker_num; ++i)
        __construct_insert(node, worker[i].end_pos, worker[i + 1].start_pos, worker[i].tail_key, worker[i].tail_node);
    //this->tail_node = tail_node(node);
    this->tail_node = worker[worker_num - 1].tail_node;
    AEX_ASSERT(this->tail_node == tail_node(node));    
}


}