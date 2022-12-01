#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_key(inner_node_ptr parent, const node_ptr node, const key_type &key){
    if (parent == nullptr) return false;
    AEX_PRINT("BEGIN");
    node_ptr* child = parent->child_ptr;
    key_type* node_key = parent->key_ptr;
    size_type old_pos = parent->at(node), new_pos;
    AEX_PRINT("node=" << node << ", key= "<< key << ", old pos=" << old_pos);
    bitmap bm = parent->bitmap_ptr;
    bool ret = false;
    if (parent->prop & ML_NODE){
        AEX_PRINT("slopt=" << parent->model.args.slopt << " inter=" << parent->model.args.inter);
        new_pos = parent->predict(key);
        size_type upper_bound = std::min(new_pos + traits::ERROR_BOUND, parent->slot_size);
        for (size_type i = new_pos; i < upper_bound; ++i){
            if (!bitmap_impl::at(bm, i) || old_pos == i){
                new_pos = i;
                ret = true;
                break;
            }
        }
        AEX_ASSERT(ret == true);
        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            for (size_type i = 0; i < parent->slot_size; ++i)
                AEX_PRINT("key=" << parent->key_ptr[i] << "child=" << parent->child_ptr[i]);
        }
        #endif
        AEX_PRINT("update ml inner node, new_pos=" << new_pos << "ret=" << ret);

        //if (ret == false) return false;

        if (old_pos <= new_pos){
            size_type prev_pos = parent->prev(old_pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= new_pos; ++i){
                node_key[i] = key;
                child[i] = child[old_pos];
            }
        }
        else if (old_pos > new_pos){
            size_type prev_pos = parent->prev(old_pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= new_pos; ++i){
                node_key[i] = key;
                //child[i] = child[old_pos];
            }
            key_type udpate_key = (old_pos == parent->slot_size) ? 0 : node_key[old_pos + 1];
            node_ptr update_node_ptr = (old_pos == parent->slot_size) ? nullptr : child[old_pos + 1];
            for (size_type i = new_pos + 1; i <= old_pos; ++i){
                node_key[i] = udpate_key;
                child[i] = update_node_ptr;
            }
        }
        bitmap_impl::set_zero(bm, old_pos);
        bitmap_impl::set_one(bm, new_pos);

        return ret;
        AEX_PRINT("END");
    }
    else{
        parent->key_ptr[old_pos] = key;
        if (old_pos < parent->size - 1) return true;
        return false;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_ptr(inner_node_ptr parent, const node_ptr old_node, const node_ptr new_node){
    if (parent == nullptr) return false;
    AEX_PRINT("update childnode pointer old node=" << old_node << " new node=" << new_node << "parent=" << parent);
    size_type pos = parent->at(old_node);
    if (pos == parent->slot_size) 
        return false;
    if (parent->prop & ML_NODE){
        size_type prev_pos = parent->prev(pos);
        node_ptr* node_child = parent->child_ptr;
        prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
        for (size_type i = prev_pos; i <= pos; ++i)
            node_child[i] = new_node;
    }
    else{
        parent->child_ptr[pos] = new_node;
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(data_node_ptr old_node, data_node_ptr new_node){
    new_node->prev = old_node;
    new_node->next = old_node->next;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    old_node->next = new_node;
    if (tail_leaf == old_node) tail_leaf = new_node;
    
    size_type mid = traits::DATA_NODE_SLOT_SIZE >> 1;
    memmove(new_node->key, old_node->key + (old_node->size - mid), (old_node->size - mid) * sizeof(key_type));
    data_memmove(new_node->data, old_node->data + (old_node->size - mid), (old_node->size - mid) * sizeof(value_type));

    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::split_with_old_node(const key_type* const key, const node_ptr* const child, const unsigned int n, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr node){
    size_type start = 0, end = n, max_slot_size = MIN_INNER_NODE_SLOT_SIZE;
    Model model;
    bool replace_flag = true;
    AEX_PRINT("target 0 " << node->slot_size);
    if (end >= node->real_slot_size() * traits::INNER_NODE_FEW_RATIO){
        size_type size = static_cast<size_type>(node->real_slot_size() * traits::INNER_NODE_FEW_RATIO);
        if (check_rewired(key, size, node->real_slot_size(), model)){
            AEX_PRINT("target 1 size=" << size);
            replace_flag = false;
            if (node->real_slot_size() >= traits::MIN_ML_NODE_SLOT_SIZE) 
                node->prop |= ML_NODE;
            node->construct(key, child, size, model);
            new_key.push_back(key[size - 1]);
            new_child.push_back(node);
            start += size;
        }
    }

    split(key + start, child + start, end - start, node->level, new_key, new_child);

    return replace_flag;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const key, const node_ptr* const child, const unsigned int n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    while (start < end){
        max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (max_slot_size < (end - start)) max_slot_size <<= 1;
        for (size_type slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1){
            size_type size = (slot_size < traits::MIN_ML_NODE_SLOT_SIZE) ? std::min(slot_size, end - start) : std::min((size_type)(slot_size * traits::INNER_NODE_FEW_RATIO), end - start);
            //size_type size = std::min(slot_size, end - start);
            AEX_PRINT("target start=" << start << " end=" << end << " size=" << size << " slot_size=" << slot_size << " key=" << key[start + size - 1]);
            if (check_rewired(key + start, size, slot_size, model)){
                inner_node_ptr new_node = node_allocator::allocate_inner_node(slot_size);
                new_node->level = level;
                new_node->construct(key + start, child + start, size, model);
                new_key.push_back(key[start + size - 1]);
                new_child.push_back(new_node);
                start += size;
                break;
            }
        }
    }
}


template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_rewired(const key_type* const key, const size_type size, const size_type slot_size, Model &m){
    if (slot_size < traits::MIN_ML_NODE_SLOT_SIZE)
        return true;
    size_type pos;
    m.train(key, size, slot_size);
    AEX_PRINT("BEGIN");
    for (size_type i = 0, start=0; i < size; ++i){            
        pos = std::max(0, std::min(m.predict(key[i]) * slot_size, slot_size - 1));
        start = std::max(start, pos);
        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            AEX_PRINT("key=" << key[i] << "pos=" << pos << " start=" << start);
        }
        #endif
        if (start - pos >= traits::ERROR_BOUND) return false;
        ++start;
    }
    AEX_PRINT("RETURN TRUE");
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rewired(inner_node_ptr node){
    Model model;
    bool flag = true;
    if (!(node->prop & ML_NODE)) return true;
    key_type* new_key = allocator::allocate_key_buffer(node->size);
    node_ptr* new_child = allocator::allocate_nodeptr_buffer(node->size);

    copy_to_buffer(node, new_key, new_child);

    flag = check_rewired(new_key, node->size, node->real_slot_size(), model);
    if (flag) node->construct(new_key, new_child, node->size, model);
    allocator::_free(new_key);
    allocator::_free(new_child);
    return flag;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::expand(inner_node_ptr &node, const inner_node_ptr &parent){
    AEX_PRINT("BEGIN");
    AEX_PRINT(node->real_slot_size() << " " << traits::EXPAND_RATIO << " " << node->level << " " << this->max_inner_slot_size_func(node->level));
    if (node->real_slot_size() * traits::EXPAND_RATIO > this->max_inner_slot_size_func(node->level)) 
        return false;
    /* TODO: ML_NODE -> NODE */
    //if (node->prop & ML_NODE)
    {
        size_type new_slot_size = node->real_slot_size() * traits::EXPAND_RATIO;
        key_type* key_buffer = allocator::allocate_key_buffer(node->size);
        copy_to_buffer(node, key_buffer);

        Model m;
        bool ml_flag = true;
        if (new_slot_size >= traits::MIN_ML_NODE_SLOT_SIZE && !check_rewired(key_buffer, node->size, new_slot_size, m)){
            ml_flag = false;
        }
        AEX_ASSERT(ml_flag == true);

        allocator::_free(key_buffer);

        inner_node_ptr new_node = node_allocator::allocate_inner_node(new_slot_size, ml_flag);
        new_node->level = node->level;

        copy_node(node, new_node);

        if (root == node) 
            root = new_node;
        if (parent != nullptr){
            node_ptr* child = parent->child_ptr;
            for (size_type i = 0; i < parent->slot_size; ++i)
            if (child[i] == node) child[i] = new_node;
        }
        node_allocator::free(node);
        AEX_PRINT("target 4");
        node = new_node;
    }
    AEX_PRINT("END");
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::narrow(inner_node_ptr &node){
    size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
    key_type* key_buf = allocator::allocate_key_buffer(node->size);
    node_ptr* child_buf = allocator::allocate_nodeptr_buffer(node->size);
    key_type* node_k = node->key_ptr;
    Model model;
    bool flag;

    copy_to_buffer(node, key_buf, child_buf);
    flag = check_rewired(key_buf, node->size, new_slot_size, model);
    if (flag){
        size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
        inner_node_ptr new_node = node_allocator::allocate_inner_node(new_slot_size);
        new_node->level = node->level;
        new_node->construct(key_buf, child_buf, node->size);
        if (root == node) root = new_node;
        node_allocator::free(node);
        node = new_node;
    }
    allocator::_free(key_buf);
    allocator::_free(child_buf);
    return true;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_node(inner_node_ptr node, inner_node_ptr new_node){
    new_node->size = node->size;
    if (!(node->prop & ML_NODE) && !(new_node->prop & ML_NODE)){
        AEX_PRINT("copy node 1" << " " << node << " "<< new_node);
        memcpy(new_node->key_ptr, node->key_ptr, node->size * sizeof(key_type));
        memcpy(new_node->child_ptr, node->child_ptr, node->size * sizeof(node_ptr));
    }
    else if ((node->prop & ML_NODE) && (new_node->prop & ML_NODE)){
        AEX_PRINT("copy node 2");
        key_type* key_buffer = allocator::allocate_key_buffer(node->size);
        node_ptr* child_buffer = allocator::allocate_nodeptr_buffer(node->size);
        copy_to_buffer(node, key_buffer, child_buffer);
        new_node->construct(key_buffer, child_buffer, node->size);
        allocator::_free(key_buffer);
        allocator::_free(child_buffer);
    }
    else if ((node->prop & ML_NODE) && !(new_node->prop & ML_NODE)){
        AEX_PRINT("copy node 3");
        copy_to_buffer(node, new_node->key_ptr, new_node->child_ptr);
    }
    else if (!(node->prop & ML_NODE) && (new_node->prop & ML_NODE)){
        AEX_PRINT("copy node 4");
        //for (size_type i = 0 )
        new_node->construct(node->key_ptr, node->child_ptr, node->size);
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node){
    memmove(left_node->key + left_node->size, right_node->key, right_node->size * sizeof(key_type));
    data_memmove(left_node->data + left_node->size, right_node->data, right_node->size * sizeof(value_type));
    left_node->size += right_node->size;

    left_node->next = right_node->next;
    if (right_node->next != nullptr) right_node->next->prev = left_node;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_node(inner_node_ptr left_node, inner_node_ptr right_node){
    AEX_ASSERT((left_node->prop & ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & ML_NODE) == 0);
    memmove(left_node->key_ptr + left_node->size, right_node->key_ptr, right_node->size * sizeof(key_type));
    memmove(left_node->child_ptr + left_node->size, right_node->child_ptr, right_node->size * sizeof(node_ptr));
    left_node->size += right_node->size;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_node(inner_node_ptr left_node, inner_node_ptr right_node){
    AEX_ASSERT((left_node->prop & ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & ML_NODE) == 0);

    memmove(right_node->key_ptr + right_node->size, right_node->key_ptr, right_node->size * sizeof(key_type));
    data_memmove(right_node->chlid_ptr() + right_node->size, right_node->chlid_ptr(), right_node->size * sizeof(value_type));
    memmove(right_node->key_ptr, left_node->key_ptr, left_node->size * sizeof(key_type));
    data_memmove(right_node->chlid_ptr(), left_node->chlid_ptr(), left_node->size * sizeof(value_type));
    right_node->size += left_node->size;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node){
    memmove(right_node->key + right_node->size, right_node->key, right_node->size * sizeof(key_type));
    data_memmove(right_node->data + right_node->size, right_node->data, right_node->size * sizeof(value_type));

    memmove(right_node->key, left_node->key, left_node->size * sizeof(key_type));
    data_memmove(right_node->data, left_node->data, left_node->size * sizeof(value_type));
    right_node->size += left_node->size;

    right_node->prev = left_node->prev;
    if (left_node->prev != nullptr) left_node->prev->next = right_node;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node){
    left_node->key[left_node->size] = right_node->key[right_node->size - 1];
    left_node->data[left_node->size] = right_node->data[right_node->size - 1];
    ++left_node->size;

    memmove(right_node->key, right_node->key + 1, (right_node->size - 1) * sizeof(key_type));
    data_memmove(right_node->data, right_node->data + 1, (right_node->size - 1) * sizeof(value_type));
    --right_node->size;        
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node){
    memmove(right_node->key + 1, right_node->key, (right_node->size) * sizeof(key_type));
    data_memmove(right_node->data + 1, right_node->data, (right_node->size) * sizeof(value_type));
    right_node->key[0] = left_node->key[left_node->size - 1];
    right_node->data[0] = left_node->data[left_node->size - 1];
    ++right_node->size;
    --left_node->size;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_node(inner_node_ptr left_node, inner_node_ptr right_node){
    AEX_ASSERT((left_node->prop & ML_NODE) == 0);
    size_type pos = right_node->first();
    left_node->key_ptr[left_node->size] = right_node->key_ptr[pos];
    left_node->child_ptr[left_node->size] = right_node->child_ptr[pos];
    erase_son_node(right_node->child_ptr[pos], right_node);

    ++left_node->size;
    --right_node->size;

    if (is_few(right_node))
        narrow(right_node);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_node(inner_node_ptr left_node, inner_node_ptr right_node){
    AEX_ASSERT((right_node->prop & ML_NODE) == 0);
    size_type pos = left_node->last();
    memmove(right_node->key_ptr + 1, right_node->key_ptr, right_node->size * sizeof(key_type));
    memmove(right_node->child_ptr + 1, right_node->child_ptr, right_node->size * sizeof(node_ptr));
    right_node->key_ptr[0] = left_node->key_ptr[pos];
    right_node->child_ptr[0] = left_node->child_ptr[pos];
    erase_son_node(left_node->child_ptr[pos], left_node);

    ++right_node->size;
    --left_node->size;

    if (is_few(left_node))
        narrow(left_node);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, key_type* key_buf, node_ptr* child_buf){
    key_type* key = node->key_ptr;
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot] = key[i];
            child_buf[n_slot] = child[i];
            n_slot++;
        }
    }
    else{
        memcpy(key_buf, key, node->size * sizeof(key_type));
        memcpy(child_buf, child, node->size * sizeof(node_ptr));
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, key_type* const key_buf){
    key_type* key = node->key_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot] = key[i];
            n_slot++;
        }
    }
    else{
        memcpy(key_buf, key, node->size * sizeof(key_type));
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr node, node_ptr* child_buf){
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            child_buf[n_slot] = child[i];
            n_slot++;
        }
    }
    else{
        memcpy(child_buf, child, node->size * sizeof(node_ptr));
    }
}



}