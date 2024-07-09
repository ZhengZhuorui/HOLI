#pragma once
//#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree():root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(), allocator(this), empty_leaf(nullptr){
    AEX_HINT("BEGIN");
    this->init();
    AEX_HINT("END");
}

template<typename _Key, typename _Val, typename traits>
template<typename _InputIterator>
inline aex_tree<_Key, _Val, traits>::aex_tree(_InputIterator __first, _InputIterator __last): root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(), allocator(this), empty_leaf(nullptr){
    this->init();
    /* TODO: insert data sequencely */
    std::vector<std::pair<key_type, value_type> > data;
    for (auto it = __first; it != __last; ++it)
        data.push_back(*it);
    std::sort(data.begin(), data.end());
    this->bulk_load(data.data(), data.size());
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(const self& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(),  allocator(this), empty_leaf(nullptr){
    this->init();
    this->root = this->construct(_index.root);
    this->link_tree_ptr();
    this->m_stats = _index.m_stats;
    this->balance_stats = _index.balance_stats;
    this->allocator = _index.allocator;
}

template<typename _Key, typename _Val, typename traits>
inline aex_tree<_Key, _Val, traits>::aex_tree(self&& _index):root(nullptr), head_leaf(nullptr), tail_leaf(nullptr), m_stats(), balance_stats(),  allocator(this), empty_leaf(nullptr){
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
inline aex_tree<_Key, _Val, traits>::~aex_tree(){
    this->erase_tree_recursive(this->root);
    this->root = nullptr;
    this->head_leaf = this->tail_leaf = nullptr;
    if (empty_leaf != nullptr)
        delete empty_leaf;
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::init(){

    if (this->root != nullptr){
        this->deconstruct(this->root);
    }
    if (empty_leaf == nullptr){
        empty_leaf = allocator.allocate_data_node();
        empty_leaf->prev = nullptr;
        empty_leaf->next = empty_leaf;
        empty_leaf->size = 0;
    }

    std::fill(this->tree_stack, this->tree_stack + traits::MAX_DEPTH, nullptr);
    //this->m_stats.max_key = std::numeric_limits<key_type>::lowest();
    //this->m_stats.min_key = std::numeric_limits<key_type>::max();
    self::inner_node_few_ratio[0] = traits::DATA_NODE_FEW_RATIO;
    self::inner_node_full_ratio[0] = traits::DATA_NODE_FULL_RATIO;
    
    self::log_inner_node_few_ratio[0] = traits::LOG_DATA_NODE_FEW_RATIO;
    self::log_inner_node_full_ratio[0] = traits::LOG_DATA_NODE_FULL_RATIO;

    self::inner_node_few_ratio[1] = traits::DATA_NODE_FEW_RATIO / 2;
    self::inner_node_full_ratio[1] = traits::DATA_NODE_FULL_RATIO / 2;

    self::log_inner_node_few_ratio[1] = traits::LOG_DATA_NODE_FEW_RATIO + 1;
    self::log_inner_node_full_ratio[1] = traits::LOG_DATA_NODE_FULL_RATIO + 1;
    //AEX_PRINT("log_slot_size=" << traits::LOG_INNER_NODE_SLOT_SIZE);
    for (int i = 2; i < traits::MAX_DEPTH; ++i){
        self::inner_node_few_ratio[i] = self::inner_node_few_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
        self::inner_node_full_ratio[i] = self::inner_node_full_ratio[i - 1] * traits::DENSITY_NARROW_RATIO;
        self::log_inner_node_few_ratio[i] = self::log_inner_node_few_ratio[i - 1] + traits::LOG_DENSITY_NARROW_RATIO;
        self::log_inner_node_full_ratio[i] = self::log_inner_node_full_ratio[i - 1] + traits::LOG_DENSITY_NARROW_RATIO;
        //AEX_PRINT("i=" << i << ": " << (1 << self::log_inner_node_few_ratio[i]) << ", " << 1.0 / self::inner_node_few_ratio[i]);
        //AEX_PRINT("i=" << i << ": " << (1 << self::log_inner_node_full_ratio[i]) << ", " << 1.0 / self::inner_node_full_ratio[i]);
        AEX_ASSERT(std::abs(self::inner_node_few_ratio[i] * (1 << self::log_inner_node_few_ratio[i]) - 1) < 1e-5);
        AEX_ASSERT(std::abs(self::inner_node_full_ratio[i] * (1 << self::log_inner_node_full_ratio[i]) - 1) < 1e-5);
    }

    this->max_inner_node_slot_size[0] = traits::MAX_DATA_NODE_SLOT_SIZE;
    for (int i = 1; i < traits::MAX_DEPTH; ++i){
        this->max_inner_node_slot_size[i] = traits::MAX_INNER_NODE_SLOT_SIZE;
        //self::inner_node_few_ratio[i] *= 0.5;
        //AEX_PRINT(this->self::inner_node_few_ratio[i] << ", " << this->self::inner_node_full_ratio[i]);
    }

    this->root = this->head_leaf = this->tail_leaf = nullptr;
    this->allocator.clear();
    this->m_stats = aex_stats();
    this->balance_stats = tree_balance_stats();
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::construct(node_ptr node){
    //AEX_PRINT("node=" << node << ", IS LEAF?" << IS_LEAF_NODE(node));
    if (node == nullptr)
        return nullptr;
    node_ptr new_node;
    if (IS_LEAF_NODE(node)){
        //new_node = allocator.allocate_data_node(node->slot_size, IS_ML_NODE(node));
        if constexpr (traits::AllowDynamicDataNode)
            new_node = allocator.allocate_data_node(node->slot_size, IS_ML_NODE(node));
        else
            new_node = allocator.allocate_data_node();
        ++this->m_stats.level_node[0];
        data_node_ptr _node = static_cast<data_node_ptr>(node), _new_node = static_cast<data_node_ptr>(new_node);
        *_new_node = *_node;
    }
    else{
        new_node = allocator.allocate_inner_node(static_cast<inner_node_ptr>(node)->slot_size, node->level, IS_ML_NODE(node));
        inner_node_ptr _node = static_cast<inner_node_ptr>(node), _new_node = static_cast<inner_node_ptr>(new_node);
        ++this->m_stats.level_node[_node->level];
        *_new_node = *_node;        
        bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr;
        node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr;
        node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr;
        if (IS_ML_NODE(node)){
            slot_type prev = 0;
            for (slot_type i = 0; i < _node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                new_child[i] = construct(child[i]);
                std::fill(new_child + prev, new_child + i, new_child[i]);
                prev = i + 1;
            }
            //new_child[_node->slot_size - 1] = construct(child[_node->slot_size - 1]);
            //std::fill(new_child + prev, new_child + _node->slot_size, new_child[_node->slot_size - 1]);

            for (slot_type i = 0; i < _new_node->hash_array_size(); ++i)
                for (slot_type j = 0; j < _new_node->hash_table.size_ptr[i]; ++j)
                    _new_node->hash_table.child_ptr[i * traits::ERROR_BOUND + j] = construct(_node->hash_table.child_ptr[i * traits::ERROR_BOUND + j]);
        }
        else{
            for (slot_type i = 0; i < node->size; ++i){
                new_child[i] = construct(child[i]);
            }
        }
    }
    return new_node;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::link_tree_ptr(){
    if (this->root == nullptr)
        return;
    std::vector<node_ptr> child_buf[2];
    std::vector<key_type> key_buf;
    child_buf[0].resize(1);
    child_buf[0][0] = root;
    int t = 0;
    while (child_buf[t].size() > 0){
        child_buf[t ^ 1].clear();
        size_type m = child_buf[t].size();
        for (size_type i = 0; i < m - 1; ++i){
            child_buf[t][i + 1]->prev = child_buf[t][i];
            child_buf[t][i]->next = child_buf[t][i + 1];
        }
        child_buf[t][0]->prev = child_buf[t][m - 1]->next = nullptr;
        if (!IS_LEAF_NODE(child_buf[t][0])){
            size_type cnt = 0;
            int level = static_cast<inner_node_ptr>(child_buf[t][0])->level;
            child_buf[t^1].resize(this->m_stats.level_node[level - 1]);
            key_buf.resize(this->m_stats.level_node[level - 1]);
            for (auto &node : child_buf[t]){
                copy_to_buffer(static_cast<inner_node_ptr>(node), key_buf.data(), child_buf[t^1].data() + cnt);
                cnt += node->size;
            }
        }
        else{
            this->head_leaf = static_cast<data_node_ptr>(child_buf[t][0]);
            this->tail_leaf = static_cast<data_node_ptr>(child_buf[t][m - 1]);
        }
        t ^= 1;
    }
    this->tail_leaf->next = empty_leaf;
    empty_leaf->prev = this->tail_leaf;
}

}