#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    AEX_FORMAT("BEGIN");
    this->init();
    AEX_FORMAT("END");
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
    this->head_leaf = find_head_leaf(root);
    this->tail_leaf = find_tail_leaf(root);
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
    //memcpy(this->m_stats, _)

}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::~aex_tree(){
    AEX_FORMAT("BEGIN");
    AEX_FORMAT("root->size=%llu, root->prop=%llu", this->root->size, this->root->prop);
    //this->deconstruct(this->root);
    this->erase_tree_recursive(this->root);
    this->root = this->head_leaf = this->tail_leaf = nullptr;
    AEX_FORMAT("END");
}


template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::init(){
    AEX_FORMAT("BEGIN");
    this->max_key = std::numeric_limits<key_type>::min();
    this->min_key = std::numeric_limits<key_type>::max();
    this->lambda = 1 - 1.0 / traits::LAMBDA_;
    AEX_FORMAT("END");
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::construct(node_ptr node, node_ptr &new_node){
    if (node->prop&LEAF){
        new_node = node_allocator.allocate_data_node(new_node->slot_size);
        static_cast<data_node>(new_node)->copy(node);
    }
    else{
        new_node = node_allocator.allocate_inner_node(node->slot_size, this->m_stats.timestamp, ((node->prop & node_property::ML_NODE) > 0));
        static_cast<inner_node_ptr>(new_node)->copy(node);
        bitmap bm = static_cast<inner_node_ptr>(new_node)->bitmap_ptr;
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr;
        if (node->prop & node_property::ML_NODE){
            size_type prev = 0;
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                construct(child[i], new_child[i]);
                memcpy(new_child + prev, child + prev, (i - prev) * sizeof(node_ptr));
                prev = i;
            }
            if (prev != node->slot_size - 1)
                memcpy(new_child + prev, child + prev, (node->slot_size - prev) * sizeof(node_ptr));
        }
        else{
            for (size_type i = 0; i < node->size; ++i){
                construct(child[i], new_child[i]);
            }
        }
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::deconstruct(node_ptr node){
    erase_tree_recursive(node);
}

}