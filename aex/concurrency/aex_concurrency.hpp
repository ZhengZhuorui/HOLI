#pragma once

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::lock_array(hash_node_ptr node){
    for (slot_type i = 0; i < pos2slot(node->slot_size); ++i){
        uint64_t expected = 0ULL;
        uint64_t result   = ~expected;
        while (!node->lock_array[i].compare_exchange_weak(expected, result)) {
            expected = 0ULL;
            _mm_pause();
        }
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


}