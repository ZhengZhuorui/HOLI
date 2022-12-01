#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    AEX_PRINT("BEGIN");
    this->init();
    AEX_PRINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    this->init();
    /* TODO: insert data sequencely */
    for (_InputIterator iter = __first; iter != __last; ++iter){
        this->insert(*iter);
    }
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
aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(){
    this->erase_subtree_recursive(this->root);
    
    this->init();
    this->root = _index.root;
    _index.root = nullptr;
    this->head_leaf = _index.head_leaf;
    _index.head_leaf = nullptr;
    this->tail_leaf = _index.tail_leaf;
    this->m_stats.size = _index.m_stats.size;
    this->m_stats.height = _index.m_stats.height;
}

template<typename _Key, typename _Val, typename traits>
aex_tree<_Key, _Val, traits>::~aex_tree(){
    AEX_PRINT("BEGIN");
    AEX_PRINT(this->root->size << " " << this->root->prop);
    //this->deconstruct(this->root);
    this->erase_subtree_recursive(this->root);
    this->root = this->head_leaf = this->tail_leaf = nullptr;
    AEX_PRINT("END");
}


template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::init(){
    AEX_PRINT("BEGIN");
    this->max_inner_node_slot_size[0] = this->max_inner_node_slot_size[1] = traits::MIN_INNER_NODE_SLOT_SIZE;
    for (size_type i = 2; i < 7; ++i)
    if (this->max_inner_node_slot_size[i - 1] < 0x3ffffff) 
        this->max_inner_node_slot_size[i] = this->max_inner_node_slot_size[i - 1] * this->max_inner_node_slot_size[i - 1];
    else this->max_inner_node_slot_size[i] = this->max_inner_node_slot_size[i - 1];
    for (size_type i = 0 ; i < 7; ++i)
        AEX_PRINT(" " << this->max_inner_node_slot_size[i]);
    AEX_PRINT("END");
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::construct(node_ptr node, node_ptr &new_node){
    if (node->prop&LEAF){
        new_node = node_allocator::allocate_data_node();
        static_cast<data_node>(new_node)->copy(node);
    }
    else{
        new_node = node_allocator::allocate_inner_node(node->slot_size, ((node->prop & ML_NODE) > 0));
        static_cast<inner_node_ptr>(new_node)->copy(node);
        bitmap bm = static_cast<inner_node_ptr>(new_node)->bitmap_ptr;
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr;
        if (node->prop & ML_NODE){
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
    erase_recursive(node);
    /*
    if (node == nullptr) return;
    AEX_PRINT("DECONSTRUCT NODE");
    if (node->prop & LEAF){
        node_allocator::free(static_cast<data_node_ptr>(node));
        return;
    }
    else{
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        if (node->prop & ML_NODE){
            bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
            for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                this->deconstruct(child[i]);
            }
        }
        else{
            for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                this->deconstruct(child[i]);
            }
        }
        node_allocator::free(static_cast<inner_node_ptr>(node));
    }
    */
}

}