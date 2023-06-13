#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    AEX_HINT("BEGIN");
    this->init();
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    this->init();
    /* TODO: insert data sequencely */
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.push_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    this->init();
    this->construct(_index.root, root);
    this->m_stats.size = _index.m_stats.size;
    this->m_stats.height = _index.m_stats.height;
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree(self&& _index){
    this->erase_tree_recursive(this->root);
    
    this->init();
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->tail_leaf = _index.tail_leaf;
    this->m_stats = _index.m_stats;

    this->split = self::split_with_exponential_probe;
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::~aex_tree(){
    AEX_HINT("BEGIN");
    this->erase_tree_recursive(this->root);
    if (this->m_stats.data_node != this->node_allocator.data_node_nums){
        AEX_ERROR("data node number error! tree data node=" << this->m_stats.data_node << ", allocator data node number=" << this->node_allocator.data_node_nums);
    }
    if (this->m_stats.inner_node != this->node_allocator.inner_node_nums){
        AEX_ERROR("inner node number no free! tree inner node=" << this->m_stats.inner_node << ", allocator inner node number=" << this->node_allocator.inner_node_nums);
    }
    this->root = this->head_leaf = this->tail_leaf = nullptr;
    AEX_HINT("END");
}


template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::init(){
    AEX_HINT("BEGIN");
    this->m_stats.max_key = std::numeric_limits<key_type>::min();
    this->m_stats.min_key = std::numeric_limits<key_type>::max();
    this->inner_node_few_ratio[0] = traits::DATA_NODE_FEW_RATIO, this->inner_node_full_ratio[0] = traits::DATA_NODE_FULL_RATIO;
    for (int i = 1; i < traits::MAX_DEPTH; ++i){
        this->inner_node_few_ratio[i] = this->inner_node_few_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
        this->inner_node_full_ratio[i] = this->inner_node_full_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
    }
    //this->lambda = 1 - 1.0 / traits::LAMBDA_;
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::construct(node_ptr node, node_ptr &new_node){
    if (node->prop & node_property::LEAF){
        new_node = node_allocator.allocate_data_node(new_node->slot_size);
        static_cast<data_node>(new_node)->copy(node);
    }
    else{
        new_node = node_allocator.allocate_inner_node(node->slot_size);
        ++this->m_stats.inner_node;
        static_cast<inner_node_ptr>(new_node)->copy(node);
        bitmap bm = static_cast<inner_node_ptr>(new_node)->bitmap_ptr;
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr;
        if (node->prop & node_property::ML_NODE){
            pos_type prev = 0;
            for (pos_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                construct(child[i], new_child[i]);
                std::fill(new_child + prev, new_child + i, new_child[i]);
                prev = i;
            }
            if (prev != node->slot_size - 1)
                std::fill(new_child + prev, new_child + node->slot_size, new_child[prev]);
        }
        else{
            for (pos_type i = 0; i < node->size; ++i){
                construct(child[i], new_child[i]);
            }
        }
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::deconstruct(node_ptr node){
    erase_tree_recursive(node);
}

}