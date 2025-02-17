#pragma once

namespace aex{
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XL(node_ptr node){
    int restart_count = 0;
XL_start:
    if (restart_count > 0)
        yield(restart_count);
    ++restart_count;
    bool need_restart = false;
    if (node->type == NodeType::HashNode){
        node->meta_lock.writeLockOrRestart(need_restart); 
        if (need_restart) goto XL_start;
    }
    
    node->node_lock.writeLockOrRestart(need_restart); 
    if (need_restart){
        if (node->type == NodeType::HashNode)
            node->meta_lock.writeUnlock();
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
        if (node->type == NodeType::HashNode) node->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XU(hash_node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
        node->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XU(node_ptr node){
    if constexpr (traits::AllowConcurrency){
        node->node_lock.writeUnlock();
        if (node->type == NodeType::HashNode) node->meta_lock.writeUnlock();
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::XUNH(node_ptr node){
    AEX_ASSERT(node->type != NodeType::HashNode);
    node->node_lock.writeUnlock();
}

}