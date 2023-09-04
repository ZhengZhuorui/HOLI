#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(){
    AEX_HINT("BEGIN");
    this->init();
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(){
    this->init();
    /* TODO: insert data sequencely */
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.push_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(){
    this->init();
    this->root = this->construct(_index.root);
    this->link_tree_ptr();
    this->m_stats = _index.m_stats;
    this->balance_stats = _index.balance_stats;
    this->node_allocator = _index.node_allocator;
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(){
    this->erase_tree_recursive(this->root);
    
    this->init();
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->tail_leaf = _index.tail_leaf;
    _index.tail_leaf = nullptr;

    this->m_stats = _index.m_stats;
    this->balance_stats = _index.balance_stats;
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::~aex_tree(){
    this->erase_tree_recursive(this->root);
    this->root = this->head_leaf = this->tail_leaf = nullptr;
}


template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::init(){
    if (this->root != nullptr){
        this->deconstruct(this->root);
    }
    this->m_stats.max_key = std::numeric_limits<key_type>::min();
    this->m_stats.min_key = std::numeric_limits<key_type>::max();
    this->inner_node_few_ratio[0] = traits::DATA_NODE_FEW_RATIO, this->inner_node_full_ratio[0] = traits::DATA_NODE_FULL_RATIO;
    for (int i = 1; i < traits::MAX_DEPTH; ++i){
        this->inner_node_few_ratio[i] = this->inner_node_few_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
        this->inner_node_full_ratio[i] = this->inner_node_full_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
    }
    this->max_inner_node_slot_size[0] = 8;
    for (int i = 1; i < traits::MAX_DEPTH; ++i){
        this->max_inner_node_slot_size[i] = traits::MAX_INNER_NODE_SLOT_SIZE;
    }

    this->root = this->head_leaf = this->tail_leaf = nullptr;
    this->node_allocator.clear();
    this->m_stats = aex_stats();
    this->balance_stats = tree_balance_stats();
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::construct(node_ptr node){
    node_ptr new_node;
    if (IS_LEAF_NODE(node)){
        new_node = node_allocator.allocate_data_node(node->slot_size, IS_ML_NODE(node));
        ++this->m_stats.level_node[0];
        data_node_ptr _node = static_cast<data_node_ptr>(node), _new_node = static_cast<data_node_ptr>(new_node);
        *_new_node = *_node;
    }
    else{
        new_node = node_allocator.allocate_inner_node(static_cast<inner_node_ptr>(node)->real_slot_size(), IS_ML_NODE(node));
        ++this->m_stats.level_node[node->level];
        inner_node_ptr _node = static_cast<inner_node_ptr>(node), _new_node = static_cast<inner_node_ptr>(new_node);
        *_new_node = *_node;        
        bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr;
        if (IS_ML_NODE(node)){
            slot_type prev = 0;
            for (slot_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                new_child[i] = construct(child[i]);
                std::fill(new_child + prev, new_child + i, new_child[i]);
                new_child[i]->parent = _new_node;
                prev = i + 1;
            }
            if (prev != node->slot_size)
                std::fill(new_child + prev, new_child + node->slot_size, new_child[prev]);
        }
        else{
            for (slot_type i = 0; i < node->size; ++i){
                new_child[i] = construct(child[i]);
                new_child[i]->parent = _new_node;
            }
        }
    }
    return new_node;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::link_tree_ptr(){
    std::vector<node_ptr> child_buf[2];
    child_buf[0].resize(1);
    child_buf[0][0] = root;
    int t = 0;
    while (child_buf[t].size() > 0){
        //AEX_PRINT("t=" << t << ", child_buf[t].size=" << child_buf[t].size() << ", level=" << child_buf[t][0]->level);
        child_buf[t^1].clear();
        size_type m = child_buf[t].size();
        for (size_type i = 0; i < m - 1; ++i){
            child_buf[t][i + 1]->prev = child_buf[t][i];
            child_buf[t][i]->next = child_buf[t][i + 1];
        }
        child_buf[t][0]->prev = child_buf[t][m - 1]->next = nullptr;
        if (!IS_LEAF_NODE(child_buf[t][0])){
            size_type cnt = 0;
            child_buf[t^1].resize(this->m_stats.level_node[child_buf[t][0]->level - 1]);
            for (auto &node : child_buf[t]){
                copy_to_buffer(static_cast<inner_node_ptr>(node), child_buf[t^1].data() + cnt);
                cnt += node->size;
            }
        }
        else{
            this->head_leaf = static_cast<data_node_ptr>(child_buf[t][0]);
            this->tail_leaf = static_cast<data_node_ptr>(child_buf[t][m - 1]);
        }
        t ^= 1;
    }
}

}