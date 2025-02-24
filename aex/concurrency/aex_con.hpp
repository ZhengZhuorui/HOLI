#pragma once

namespace aex{
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XL(node_ptr node){
    int restart_count = 0;
XL_start:
    AEX_ASSERT(restart_count < 100000000);
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    if (node->type == NodeType::HashNode){
        h_n(node)->meta_lock.writeLockOrRestart(need_restart); 
        if (need_restart) goto XL_start;
    }
    
    node->node_lock.writeLockOrRestart(need_restart); 
    if (need_restart){
        if (node->type == NodeType::HashNode)
            h_n(node)->meta_lock.writeUnlock();
        goto XL_start;
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::TXL(hash_node_ptr node, bool &need_restart){
    if constexpr (traits::AllowConcurrency){
        node->meta_lock.writeLockOrRestart(need_restart);
        if (need_restart) return;
        node->node_lock.writeLockOrRestart(need_restart);
        if (need_restart) node->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::TXL(node_ptr node, bool &need_restart){
    if constexpr (traits::AllowConcurrency){
        if (node->type == NodeType::HashNode) return TXL(h_n(node), need_restart);
        else node->node_lock.writeLockOrRestart(need_restart);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::TUL(hash_node_ptr node, version_type &node_version, bool &need_restart){
    if constexpr (traits::AllowConcurrency){
        node->meta_lock.writeLockOrRestart(need_restart);
        if (need_restart) return;
        node->node_lock.upgradeToWriteLockOrRestart(node_version, need_restart);
        if (need_restart) node->meta_lock.writeUnlock();
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
inline void aex_tree<_Key, _Val, traits>::DL(hash_node_ptr node, version_type &node_version){
    if constexpr (traits::AllowConcurrency){
        node_version = node->node_lock.downgradeLock();
        node->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::DL(dense_node_ptr node, version_type &node_version){
    if constexpr (traits::AllowConcurrency)
        node_version = node->node_lock.downgradeLock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::DL(node_ptr node, version_type &node_version){
    if constexpr (traits::AllowConcurrency){
        node_version = node->node_lock.downgradeLock();
        if (node->type == NodeType::HashNode) h_n(node)->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XU(hash_node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
        h_n(node)->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XU(node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
        if (node->type == NodeType::HashNode) h_n(node)->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XUNH(node_ptr node){
    AEX_ASSERT(node->type != NodeType::HashNode);
    node->node_lock.writeUnlock();
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
    if constexpr (traits::AllowConcurrency){
    ConcurrencyParams* params;
    bool flag = this->work_queue.pop(params);
    if (flag){
        switch(params->type){
            case ConcurrencyType::GetChilds :{
                get_childs_unit(static_cast<GetChildsParams*>(params));
                break;
            }
            case ConcurrencyType::ConstructSMO :{
                construct_SMO_unit(static_cast<ConstructSMOParams*>(params));
                break;
            }
            case ConcurrencyType::HashTableRescale :{
                this->hash_table.rescale_unit(static_cast<HashTableRescaleParams*>(params));
                break;
            }
            default:{
                AEX_ASSERT(0 == 1);
            }
        }
        
    }
    else
        flag = this->hash_table.work_concurrency();
    return flag;
    }
    else
        return false;
}

template<typename traits>
struct _GetChildsParams : ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename components::key_type key_type;
    typedef typename components::node_ptr node_ptr;
    typedef typename traits::slot_type slot_type;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    slot_type start, end;
};

template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::get_childs_unit(const hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf, const slot_type start, const slot_type end){
inline void aex_tree<_Key, _Val, traits>::get_childs_unit(GetChildsParams *worker){
    const hash_node_ptr node = worker->node;
    for (int i = node->next_item(worker->start); i < worker->end; i = node->next_item(i)){
        key_type key;
        node_ptr child;
        std::tie(key, child) = hash_table.find(node, i);
        worker[i]->key_buf.emplace_back(key);
        worker[i]->child_buf.emplace_back(child);
    }
    worker->finish_flag.store(true);
    _mm_mfence();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::get_childs_con(const hash_node_ptr node, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    slot_type unit_slot_size = std::max(node->slot_size / this->thread_num, traits::THREAD_UNIT_SIZE * traits::SLOT_PER_LOCK);
    ULL worker_num = std::min(worker_num, node->slot_size / unit_slot_size + (node->slot_size % unit_slot_size == 0));
    int cnt = 0;
    std::vector<GetChildsParams> worker(worker_num + 1);
    std::vector<int> pre_sum(worker_num);
    for (int i = 0; i < this->slot_size; i += unit_slot_size, ++cnt){
        //std::function<void()> t = std::bind(self::get_childs_unit, this, node, worker_key[i], worker_childs[i], i, std::min(i + unit_slot_size, node->slot_size));
        worker[i].node = node;
        worker[i].start = i;
        worker[i].end = std::min(i + unit_slot_size);
        bool flag = this->queue.push_back(static_cast<ConcurrencyParams*>(&worker[i]));
        while (!flag) {
            while(this->work_concurrency());
            flag = this->work_queue.push(&worker[i]);
        }
    }
    while (work_concurrency() == true); 
    for (int i = 0; i < worker_num; ++i){
        while (worker[i].finish_flag.store(false))
            _mm_pause();
    }
    // join
    for (int i = 1; i < worker_num; ++i)
        pre_sum[i] = pre_sum[i - 1] + worker[i].worker_key.size();
    key_buf.resize(pre_sum[worker_num - 1]);
    child_buf.resize(pre_sum[worker_num - 1]);
    for (int i = 0; i < worker_num; ++i){
        memcpy(worker[i].key_buf.data(), worker[i].key_buf.data() + worker[i].key_buf.size(), key_buf.data() + pre_sum[i]);
        memcpy(worker[i].child_buf.data(), worker[i].child_buf.data() + worker[i].child_buf.size(), child_buf.data() + pre_sum[i]);
    }
}

template<typename traits>
struct _ConstructSMOParams : public ConcurrencyParams{
    typedef aex_default_components<traits> components;
    typedef typename components::key_type key_type;
    typedef typename components::node_ptr node_ptr;
    typedef typename traits::slot_type slot_type;
    node_ptr node;
    key_type* keys;
    node_ptr* childs;
    ULL n;
    slot_type start_pos, end_pos;
    key_type tail_key;
    node_ptr tail_node;
    bool finish_flag;
};

template<typename _Key, typename _Val, typename traits>
//inline void aex_tree<_Key, _Val, traits>::construct_SMO_unit(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n, std::pair<slot_type, slot_type> &meta_pos, std::pair<key_type, node_ptr> &tail_node){
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
    AEX_DEBUG_BLOCK({if (pos >= end) AEX_PRINT(node->predict(keys[n - 1]) << ", pos=" << pos << ", slot_size=" << node->slot_size);});
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
    worker->finish_flag.store(true);
    _mm_mfence();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_SMO_con(hash_node_ptr node, const key_type* keys, node_ptr* childs, const ULL n){
    slot_type unit_size = std::min(n / this->thread_num, traits::THREAD_UNIT_SIZE);
    ULL worker_num = std::min(worker_num, n / unit_size + (node->slot_size % unit_size != 0));
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
        this->queue.push_back(static_cast<ConcurrencyParams*>(&worker[i]));
    }
    while (this->work_concurrency() == true);
    for (int i = 0; i < worker_num; ++i){
        while(worker[i].finish_flag.load() == false) _mm_pause();
    }
    
    for (int i = 0; i < worker_num; ++i)
        __construct_insert(node, worker[i].end_pos, worker[i + 1].start_pos, worker[i].tail_key, worker[i].tail_node);
    //this->tail_node = tail_node(node);
    this->tail_node = worker[worker_num - 1].tail_node;
    AEX_ASSERT(this->tail_node == tail_node(node));
    
}

}