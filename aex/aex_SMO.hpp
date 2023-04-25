#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict node, const key_type &key){
    AEX_ASSERT(parent != node);
    if (parent == nullptr) return false;
    AEX_FORMAT("BEGIN");
    if (parent->prop & node_property::ML_NODE){
        node_ptr* child = parent->child_ptr;
        key_type* node_key = parent->key_ptr;
        bitmap bm = parent->bitmap_ptr;
        size_type old_pos = parent->at(node), new_pos = parent->predict(key);
        if (old_pos == new_pos){
            size_type prev_pos = parent->prev_item(old_pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= new_pos; ++i)
                node_key[i] = key;
            return true;
        }
        else if (old_pos < new_pos){
            if (bitmap_impl::at(bm, new_pos) == 0){
                size_type prev_pos = parent->prev_item(old_pos);
                prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
                for (size_type i = prev_pos; i <= new_pos; ++i){
                    if (i < new_pos && i != old_pos)
                        AEX_ASSERT(bitmap_impl::at(bm, i) == 0);
                    node_key[i] = key;
                }
                bitmap_impl::set_zero(bm, old_pos);
                bitmap_impl::set_one(bm, new_pos);
                parent->slot_bound = std::max(parent->slot_bound, new_pos);
                return true;
            }
            else 
                return false;
        }
        else{
            if (child[new_pos] == node){
                size_type prev_pos = parent->prev_item(old_pos);
                prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
                key_type next_key = (old_pos == parent->slot_bound) ? node_key[old_pos + 1]: this->max_key;
                for (size_type i = new_pos + 1; i <= old_pos; ++i) 
                    node_key[i] = next_key;
                for(size_type i = old_pos; i <= new_pos; ++i)
                    node_key[i] = key;

                bitmap_impl::set_zero(bm, old_pos);
                bitmap_impl::set_one(bm, new_pos);

                if (old_pos == parent->slot_bound)
                    parent->slot_bound = new_pos;
                return true;
            }
            return false;
        }
    }
    else{
        size_type pos = parent->at(node);
        parent->key_ptr[pos] = key;
        return true;
    }
}

/*
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::update_childnode_key(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ node, const key_type &key){
    if (parent == nullptr) return false;
    AEX_FORMAT("BEGIN");
    node_ptr* child = parent->child_ptr;
    key_type* node_key = parent->key_ptr;
    size_type old_pos = parent->at(node), new_pos;
    AEX_PRINT("node=" << node << ", key= "<< key << ", old pos=" << old_pos);
    bitmap bm = parent->bitmap_ptr;
    bool ret = false;
    if (parent->prop & node_property::ML_NODE){
        AEX_FORMAT("slopt=%.4f, inter=%.4f", parent->model.args.slopt, parent->model.args.inter);
        new_pos = parent->predict(key);
        // if new pos <= old pos, there must be gap between [new_pos, old_pos]
        // TODO: if new pos > old pos?
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
        AEX_FORMAT("update ml inner node, new_pos=%d, ret=%d", new_pos, ret);

        //if (ret == false) return false;

        AEX_ASSERT(old_pos < new_pos);

        if (old_pos == new_pos){
            AEX_FORMAT("END");
            return ret;
        }
        //else if (old_pos < new_pos){
        //    size_type prev_pos = parent->prev_item(old_pos);
        //    prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
        //    for (size_type i = prev_pos; i <= new_pos; ++i){
        //        node_key[i] = key;
        //        child[i] = child[old_pos];
        //    }
        //    parent->slot_bound = std::max(parent->slot_bound, new_pos);
        //}
        //else if (old_pos > new_pos){
        else{
            bool update_slot_bound = (old_pos == this->slot_bound - 1);
            size_type prev_pos = parent->prev_item(old_pos);
            prev_pos = (prev_pos == parent->slot_size) ? 0 : prev_pos + 1;
            for (size_type i = prev_pos; i <= new_pos; ++i){
                node_key[i] = key;
            }
            key_type update_key = (old_pos == parent->slot_size) ? 0 : node_key[old_pos + 1];
            node_ptr update_node_ptr = (old_pos == parent->slot_bound - 1) ? nullptr : child[old_pos + 1];
            for (size_type i = new_pos + 1; i <= old_pos; ++i){
                node_key[i] = update_key;
                child[i] = update_node_ptr;
            }
            if (update_slot_bound)
                this->slot_bound = new_pos + 1;
        }
        bitmap_impl::set_zero(bm, old_pos);
        bitmap_impl::set_one(bm, new_pos);
        AEX_FORMAT("END");
        return ret;
    }
    else{
        parent->key_ptr[old_pos] = key;
        if (old_pos < parent->size - 1) return true;
        return false;
    }
}
*/

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::update_childnode_ptr(inner_node_ptr __restrict__ parent, const node_ptr __restrict__ old_node, const node_ptr __restrict__ new_node){
    AEX_ASSERT(old_node != new_node);
    AEX_ASSERT(old_node != parent);
    if (parent == nullptr) return false;
    AEX_FORMAT("update childnode pointer old node=%p, new_node=%p, parent=%p", old_node, new_node, parent);
    size_type pos = parent->at(old_node);
    if (pos == parent->slot_size) 
        return false;
    parent->size += -old_node->size + new_node->size;
    parent->data_size() += -old_node->data_size() + new_node->data_size();
    parent->m_stats.data_node += -old_node->data_node_size() + new_node->data_node_size();
    

    if (parent->prop & node_property::ML_NODE){
        size_type prev_pos = parent->prev_item(pos);
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
void aex_tree<_Key, _Val, traits>::split(data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node){
    AEX_ASSERT(old_node == new_node);
    AEX_ASSERT(old_node->slot_size == new_node->slot_size);
    new_node->prev = old_node;
    new_node->next = old_node->next;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    old_node->next = new_node;
    if (tail_leaf == old_node) tail_leaf = new_node;

    // meta:
    {
        update_node_frequency(old_node);
        new_node->base_stats.write_times = old_node->base_stats.write_times / 2;
        old_node->base_stats.write_times /= 2;
        new_node->base_stats.train_times = old_node->base_stats.train_times / 2;
        old_node->base_stats.train_times /= 2;
        new_node->base_stats.recent_update_timestamp = old_node->base_stats.recent_update_timestamp;
    }
    
    size_type mid = old_node->size >> 1;
    memmove(new_node->key, old_node->key + (old_node->size - mid), (old_node->size - mid) * sizeof(key_type));
    data_memmove(new_node->data, old_node->data + (old_node->size - mid), (old_node->size - mid) * sizeof(value_type));

    new_node->size = old_node->size - mid;
    old_node->size = mid;

    if (old_node->prop & node_property::ML_NODE){
        old_node->train_model();
    }
    if (new_node->prop & node_property::ML_NODE){
        new_node->train_model();
    }

}

// split a ordered key array with child pointers array. Support the old node firstly.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::split_with_old_node(const key_type* const __restrict__ key, const node_ptr* const __restrict__ child, const size_type n, 
                std::vector<key_type> &new_key, std::vector<node_ptr> &new_child, inner_node_ptr __restrict__ node){
    size_type start = 0, end = n;
    Model model;
    bool replace_flag = true;
    AEX_FORMAT("target 0, node->slot_size= ", node->slot_size);
    if (end >= node->real_slot_size() * traits::INNER_NODE_FEW_RATIO){
        size_type size = static_cast<size_type>(node->real_slot_size() * traits::INNER_NODE_FEW_RATIO);
        if (check_rewired(key, size, node->real_slot_size(), model)){
            AEX_FORMAT("target 1 size=%llu", size);
            replace_flag = false;
            if (node->real_slot_size() >= traits::MIN_ML_INNER_NODE_SLOT_SIZE) 
                node->prop |= node_property::ML_NODE;
            node->construct(key, child, size, model);
            new_key.push_back(key[size - 1]);
            new_child.push_back(node);
            start += size;
        }
    }

    split(key + start, child + start, end - start, node->level, new_key, new_child);

    //meta:
    size_type m = new_child.size();
    node_ptr prev_node = node->prev, next_node = node->next;
    if (prev_node != nullptr) prev_node->next = new_child[0];
    new_child[0]->prev = prev_node;
    if (next_node != nullptr) next_node->prev = new_child[m  - 1];
    new_child[m - 1]->next = next_node;
    for(size_type i = 0; i < m - 1; ++i){
        new_child[i + 1]->prev = new_child[i];
        new_child[i]->next = new_child[i + 1];
    }

    return replace_flag;
}

// split a ordered key array with child pointers array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const __restrict__ key, const node_ptr* const __restrict__ child, const unsigned int n, const unsigned int level, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    Model model;
    while (start < end){
        size_type max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (max_slot_size * traits::INNER_NODE_FEW_RATIO < (end - start)) max_slot_size <<= 1;
        for (size_type slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1){
            size_type size = (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE) ? std::min(slot_size, end - start) : std::min((size_type)(slot_size * traits::INNER_NODE_FEW_RATIO), end - start);
            AEX_PRINT("target start=" << start << " end=" << end << " size=" << size << " slot_size=" << slot_size << " key=" << key[start + size - 1]);
            if (check_rewired(key + start, size, slot_size, model)){
                inner_node_ptr new_node = node_allocator.allocate_inner_node(slot_size, this->m_stats.timestamp);
                ++this->m_stats.inner_node;
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

// split a ordered key array with data array to inner node array.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::split(const key_type* const __restrict__ key, const value_type* const __restrict__ data, const unsigned int n, std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    size_type start = 0, end = n;
    Model model;
    while (start < end){
        size_type max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (max_slot_size < (end - start)) max_slot_size <<= 1;
        for (size_type slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1){
            size_type size = (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE) ? std::min(slot_size, end - start) : std::min((size_type)(slot_size * traits::INNER_NODE_FEW_RATIO), end - start);
            AEX_PRINT("target start=" << start << " end=" << end << " size=" << size << " slot_size=" << slot_size << " key=" << key[start + size - 1]);
            if (check_rewired(key + start, size, slot_size, model)){
                data_node_ptr new_node = node_allocator.allocate_data_node(slot_size, this->m_stats.timestamp);
                ++this->m_stats.data_node;
                new_node->construct(key + start, data + start, size, model);
                new_key.push_back(key[start + size - 1]);
                new_child.push_back(new_node);
                start += size;
                break;
            }
        }
    }
}

// check if key buffer can put in a node with slot_size slot size. The model m will be trained if the answer is true
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_rewired(const key_type* const __restrict__ key, const size_type size, const size_type slot_size, Model &m){
    if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
        return true;
    size_type pos;
    m.train(key, size);
    AEX_FORMAT("BEGIN");
    for (size_type i = 0, start=0; i < size; ++i){            
        pos = std::max((size_type)0, std::min((size_type)m.predict(key[i]) * slot_size, slot_size - 1));
        start = std::max(start, pos);
        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            AEX_PRINT("key=" << key[i] << "pos=" << pos << " start=" << start);
        }
        #endif
        if (start - pos >= traits::ERROR_BOUND) return false;
        ++start;
    }
    AEX_FORMAT("RETURN TRUE");
    return true;
}

// rewired the <key, node_ptr> array of a node. Return true if <K, P> array can be rewired. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rewired(inner_node_ptr node){
    if (node->m_stats.rewired_cnt > 0){
        return false;
    }
    node->m_stats.rewired_cnt += this->init_rewired_cnt(node);
    
    Model model;
    bool flag = true;
    if (!(node->prop & node_property::ML_NODE)) return true;
    key_type* new_key = node_allocator.allocate_key_buffer(node->size);
    node_ptr* new_child = node_allocator.allocate_nodeptr_buffer(node->size);

    copy_to_buffer(node, new_key, new_child);

    flag = check_rewired(new_key, node->size, node->real_slot_size(), model);
    if (flag) node->construct(new_key, new_child, node->size, model);
    node_allocator.deallocate(new_key);
    node_allocator.deallocate(new_child);
    return flag;
}

// Rescale a inner node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(inner_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_FORMAT("BEGIN");
    AEX_FORMAT("real_slot_size=%llu, EXPAND_RATIO=%.2f, node level=%llu, max_inner_slot_size=%llu", node->real_slot_size(), traits::EXPAND_RATIO, node->level, this->max_inner_slot_size_func(node->level));
    //if (node->prop & node_property::ML_NODE)
    {
        size_type new_slot_size = node->real_slot_size() * ratio;
        key_type* key_buffer = node_allocator.allocate_key_buffer(node->size);
        node_ptr* child_buffer =  node_allocator.allocate_nodeptr_buffer(node->size);
        copy_to_buffer(node, key_buffer, child_buffer);

        inner_node_ptr __restrict__ new_node = node_allocator.allocate_inner_node(new_slot_size);
        new_node->construct(key_buffer, child_buffer, node->size);
        replace_node(node, new_node);
        //update_node_frequency(node);

        update_childnode_ptr(parent, node, new_node);
        node_allocator.free_node(node);
        AEX_FORMAT("target 4");
        node = new_node;
        node_allocator.deallocate(key_buffer);
        node_allocator.deallocate(child_buffer);
    }
    AEX_FORMAT("END");
    return true;
}

// Rescale a data node slot_size. ratio > 1 means expand and ratio < 1 means narrow. 
// if node expand or narrow successed, the old node will free and return true. Otherwise return false.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(data_node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_FORMAT("BEGIN");
    size_type new_slot_size = node->slot_size * ratio;
    if (new_slot_size < traits::MIN_DATA_NODE_SLOT_SIZE)
        return false;
    data_node_ptr new_node = node_allocator.allocate_data_node(new_slot_size);
    new_node->construct(node->key, node->data, node->size);
    replace_node(node, new_node);
    update_childnode_ptr(parent, node, new_node);
    node_allocator.free_node(node);
    node = new_node;
    return true;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::rescale(node_ptr __restrict__ &node, inner_node_ptr __restrict__ parent, const double ratio){
    AEX_ASSERT(node != parent);
    if (node->prop & LEAF) 
        return rescale(static_cast<data_node_ptr>(node), parent, ratio);
    else
        return rescale(static_cast<inner_node_ptr>(node), parent, ratio);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_node(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ new_node){
    AEX_ASSERT(node != new_node);
    new_node->size = node->size;
    //new_node->slot_bound = node->slot_bound;
    if (!(node->prop & node_property::ML_NODE) && !(new_node->prop & node_property::ML_NODE)){
        AEX_FORMAT("copy node 1" << " " << node << " "<< new_node);
        memcpy(new_node->key_ptr, node->key_ptr, node->size * sizeof(key_type));
        memcpy(new_node->child_ptr, node->child_ptr, node->size * sizeof(node_ptr));
    }
    else if ((node->prop & node_property::ML_NODE) && (new_node->prop & node_property::ML_NODE)){
        AEX_FORMAT("copy node 2");
        key_type* key_buffer = node_allocator.allocate_key_buffer(node->size);
        node_ptr* child_buffer = node_allocator.allocate_nodeptr_buffer(node->size);
        copy_to_buffer(node, key_buffer, child_buffer);
        new_node->construct(key_buffer, child_buffer, node->size);
        node_allocator.deallocate(key_buffer);
        node_allocator.deallocate(child_buffer);
    }
    else if ((node->prop & node_property::ML_NODE) && !(new_node->prop & node_property::ML_NODE)){
        AEX_FORMAT("copy node 3");
        copy_to_buffer(node, new_node->key_ptr, new_node->child_ptr);
    }
    else if (!(node->prop & node_property::ML_NODE) && (new_node->prop & node_property::ML_NODE)){
        AEX_FORMAT("copy node 4");
        new_node->construct(node->key_ptr, node->child_ptr, node->size);
    }
}

// merge right leaf to left leaf.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    memmove(left_node->key + left_node->size, right_node->key, right_node->size * sizeof(key_type));
    data_memmove(left_node->data + left_node->size, right_node->data, right_node->size * sizeof(value_type));
    left_node->size += right_node->size;
    //update_node_frequency(left_node);
    //update_node_frequency(right_node);
    left_node->next = right_node->next;
    if (right_node->next != nullptr) right_node->next->prev = left_node;
    left_node->train_model();
}

// merge left leaf to right leaf.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    memmove(right_node->key + right_node->size, right_node->key, right_node->size * sizeof(key_type));
    data_memmove(right_node->data + right_node->size, right_node->data, right_node->size * sizeof(value_type));
    memmove(right_node->key, left_node->key, left_node->size * sizeof(key_type));
    data_memmove(right_node->data, left_node->data, left_node->size * sizeof(value_type));
    right_node->size += left_node->size;
    //update_node_frequency(left_node);
    //update_node_frequency(right_node);
    right_node->prev = left_node->prev;
    if (left_node->prev != nullptr) left_node->prev->next = right_node;
    right_node->train_model();
}

// merge right inner node to left inner node. require the left inner node and right inner node must be not ML node.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->size + right_node->size > left_node->slot_size);
    memcpy(left_node->key_ptr + left_node->size, right_node->key_ptr, right_node->size * sizeof(key_type));
    memcpy(left_node->child_ptr + left_node->size, right_node->child_ptr, right_node->size * sizeof(node_ptr));

    if (left_node->level == 1){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        left_node->base_stats.write_times += right_node->base_stats.write_times;
        left_node->base_stats.train_times += right_node->base_stats.train_times;
        left_node->size += right_node->size;
        left_node->m_stats.data_size += right_node->m_stats.data_size;
        left_node->m_stats.data_node += right_node->m_stats.data_node;
    }

    left_node->next = right_node->next;
    if (right_node->next != nullptr) right_node->next->prev = left_node;
}

// merge left inner node to right inner node. require the left inner node and right inner node must be not ML node.
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::merge_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->size + right_node->size > right_node->slot_size);
    memmove(right_node->key_ptr + left_node->size, right_node->key_ptr, right_node->size * sizeof(key_type));
    memmove(right_node->child_ptr + left_node->size, right_node->child_ptr, right_node->size * sizeof(node_ptr));

    memmove(right_node->key_ptr, left_node->key_ptr, left_node->size * sizeof(key_type));
    memmove(right_node->child_ptr, left_node->child_ptr, left_node->size * sizeof(node_ptr));
    if (right_node->level == 1){
        update_node_frequency(left_node);
        update_node_frequency(right_node);
        right_node->base_stats.write_times += right_node->base_stats.write_times;
        right_node->base_stats.train_times += right_node->base_stats.train_times;
        right_node->size += left_node->size;
        left_node->m_stats.data_size += right_node->m_stats.data_size;
        left_node->m_stats.data_node += right_node->m_stats.data_node;
    }

    right_node->prev = left_node->prev;
    if (left_node->prev != nullptr) left_node->prev->next = right_node;
}

// shift one item from right leaf to left leaf
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    left_node->key[left_node->size] = right_node->key[right_node->size - 1];
    left_node->data[left_node->size] = right_node->data[right_node->size - 1];
    memmove(right_node->key, right_node->key + 1, (right_node->size - 1) * sizeof(key_type));
    data_memmove(right_node->data, right_node->data + 1, (right_node->size - 1) * sizeof(value_type));
    ++left_node->size;
    --right_node->size;
}

// shift one item from left leaf to right leaf
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_leaf(data_node_ptr __restrict__ left_node, data_node_ptr __restrict__ right_node){
    memmove(right_node->key + 1, right_node->key, (right_node->size) * sizeof(key_type));
    data_memmove(right_node->data + 1, right_node->data, (right_node->size) * sizeof(value_type));
    right_node->key[0] = left_node->key[left_node->size - 1];
    right_node->data[0] = left_node->data[left_node->size - 1];
    ++right_node->size;
    --left_node->size;
}


// shift one item from right inner node to left brother, the left node must be least node, because left node will narrow if left node is ML_NODE
// left node must not be ML_NODE
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_left_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((left_node->prop & node_property::ML_NODE) == 0);
    AEX_ASSERT(left_node->level == right_node->level);
    key_type shift_key = right_node->key_ptr[0];
    node_ptr shift_node = right_node->child_ptr[0];
    if (right_node->level == 1){
        ++left_node->size;
        --right_node->size;
        if (this->allow_balance){
            left_node->m_stats.data_size += shift_node->data_size();
            right_node->m_stats.data_size -= shift_node->data_size();

            left_node->m_stats.data_node += shift_node->data_node_size();
            right_node->m_stats.data_node -= shift_node->data_node_size();

            update_node_frequency(left_node);
            update_node_frequency(right_node);
            update_node_frequency(static_cast<data_node_ptr>(shift_node));

            left_node->base_stats.write_times += shift_node->base_stats.write_times;
            right_node->base_stats.write_times -= shift_node->base_stats.write_times;
            left_node->base_stats.train_times += shift_node->base_stats.train_times;
            right_node->base_stats.train_times -= shift_node->base_stats.train_times;
        }
    }

    erase_son_node(right_node, shift_node);

    left_node->key_ptr[left_node->slot_bound] = shift_key;
    left_node->child_ptr[left_node->slot_bound] = shift_node;
    ++left_node->slot_bound;
}


// shift one item from left inner node to right brother, the left node must be least node, because right node will narrow if right node is node_property::ML_NODE,
// right node must not be ML_NODE
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::shift_to_right_node(inner_node_ptr __restrict__ left_node, inner_node_ptr __restrict__ right_node){
    AEX_ASSERT((right_node->prop & node_property::ML_NODE) == 0);
    key_type shift_key = left_node->key_ptr[left_node->last()];
    node_ptr shift_node = left_node->child_ptr[left_node->last()];
    if (right_node->level == 1){
        ++left_node->size;
        --right_node->size;
        if (this->allow_balance){
            left_node->m_stats.data_size -= shift_node->data_size();
            right_node->m_stats.data_size += shift_node->data_size();

            left_node->m_stats.data_node -= shift_node->data_node_size();
            right_node->m_stats.data_node += shift_node->data_node_size();
            update_node_frequency(left_node);
            update_node_frequency(right_node);
            update_node_frequency(static_cast<data_node_ptr>(shift_node));
            left_node->base_stats.write_times -= shift_node->base_stats.write_times;
            right_node->base_stats.write_times += shift_node->base_stats.write_times;
            left_node->base_stats.train_times -= shift_node->base_stats.train_times;
            right_node->base_stats.train_times += shift_node->base_stats.train_times;
        }
    }

    erase_son_node(left_node, shift_node);

    memmove(right_node->key_ptr + 1, right_node->key_ptr, right_node->slot_bound * sizeof(key_type));
    memmove(right_node->child_ptr + 1, right_node->child_ptr, right_node->slot_bound * sizeof(node_ptr));
    right_node->key_ptr[0] = shift_key;
    right_node->child_ptr[0] = shift_node;
    ++right_node->slot_bound;
    

}

// copy keys and pointers of a node to key buffer and pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* __restrict__ key_buf, node_ptr* __restrict__ child_buf){
    key_type* key = node->key_ptr;
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot] = key[i];
            child_buf[n_slot] = child[i];
            n_slot++;
        }
    }
    else{
        memcpy(key_buf, key, node->slot_bound * sizeof(key_type));
        memcpy(child_buf, child, node->slot_bound * sizeof(node_ptr));
    }
}

// copy keys of a node to key buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, key_type* const __restrict__ key_buf){
    key_type* key = node->key_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            key_buf[n_slot++] = key[i];
        }
    }
    else{
        memcpy(key_buf, key, node->size * sizeof(key_type));
    }
}

// copy pointers of a node to pointers buffer
template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::copy_to_buffer(const inner_node_ptr __restrict__ node, node_ptr* __restrict__ child_buf){
    node_ptr* child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    size_type n_slot = 0;
    if (node->prop & node_property::ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            child_buf[n_slot++] = child[i];
        }
    }
    else{
        memcpy(child_buf, child, node->size * sizeof(node_ptr));
    }
}

// replace new_node to old_node (contain m_stats, level, prev, next of node)
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::replace_node(const inner_node_ptr __restrict__ old_node, inner_node_ptr __restrict__ new_node){
    new_node->m_stats = old_node->m_stats;
    new_node->level = old_node->level;
    new_node->prev = old_node->prev;
    new_node->next = old_node->next;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    if (this->root == old_node)
        this->root = new_node;
}

// replace new_node to old_node (contain m_stats, level, prev, next of node)
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::replace_node(const data_node_ptr __restrict__ old_node, data_node_ptr __restrict__ new_node){
    new_node->base_stats = old_node->base_stats;
    new_node->level = old_node->level;
    new_node->prev = old_node->prev;
    new_node->next = old_node->next;
    if (old_node->prev != nullptr) old_node->prev->next = new_node;
    if (old_node->next != nullptr) old_node->next->prev = new_node;
    if (this->root == old_node)
        this->root = new_node;
}

}