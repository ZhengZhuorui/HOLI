#pragma once
#include "aex/aex.h"

namespace aex{

/* require: update_childnode_key, find_upper, node_allocator.allocate_data_node, node_allocator.allocate_inner_node, insert_data, insert_one*/
template<typename _Key, typename _Val, typename traits>
std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    //node *_stack[traits::MAX_LEVEL], *now_node;
    // the update flag is true if and only if only the key is the max key of the tree,
    ++this->m_stats.timestamp;
    update_tree_frequency();
    ++this->m_stats.write_times;
    
    node_ptr stack[traits::MAX_LEVEL];
    int top;

    this->m_stats.lambda_timestamp = std::fma(this->m_stats.lambda_timestamp, this->lambda, 1);

    std::pair<iterator, bool> ret;

    AEX_FORMAT("BEGIN");

    if (root == nullptr){
        root = head_leaf = tail_leaf = node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE);
        ++m_stats.data_node;
    }
    AEX_FORMAT("level=%u, size=%llu, root_size=%llu", root->level, this->m_stats.size, this->root->data_size());

    AEX_FORMAT("FIND PATH END");
    
    data_node_ptr old_data_node, new_data_node;
    iterator iter;
    if (this->max_key > key){
        std::false_type fp;
        iter = this->find_lower_with_trace(this->max_key, stack, top, fp);
        this->max_key = key;
        for (int i = 1; i < top - 1; ++i){
            bool flag __attribute__((unused));
            flag = update_childnode_key(static_cast<inner_node_ptr>(stack[i]), stack[i + 1], key);
            AEX_ASSERT(flag == true);
        }
    }

    std::false_type fp;
    //iter = this->find_lower_with_trace(key, stack, top, this->allow_balance);
    iter = this->find_lower_with_trace(key, stack, top, fp);
    if (top > 2){
        update_node_frequency(static_cast<inner_node_ptr>(stack[top - 2]));
        stack[top - 2]->base_stats.write_times++;
    }
    update_node_frequency(static_cast<data_node_ptr>(stack[top - 1]));
    static_cast<data_node_ptr>(stack[top - 1])->base_stats.write_times++;

    /* insert to data node */
    AEX_FORMAT("INSERT DATA NODE");

    /* find the insert position */
    size_type pos = old_data_node->find_lower_pos(key);
    if (pos < old_data_node->slot_size && old_data_node->key[pos] == key){
        return std::pair<iterator, bool>(end(), false);
    }

    /* if data node is full, split the node */
    if (isfull(old_data_node)){
        inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
        if (check_balance_split(old_data_node)){
            AEX_FORMAT("INSERT DATA NODE SPLIT");
            // data_node => [old_data_node, new_data_node]
            // parent_node->ptr = [... data_node ...] => [... old_data_node(need insert), new_data_node ... ] 
            new_data_node = node_allocator.allocate_data_node(old_data_node->slot_size);
            ++m_stats.data_node;
            update_childnode_ptr(parent, old_data_node, new_data_node);
            split(old_data_node, new_data_node);
            if (pos < old_data_node->size){
                pos = insert_data(old_data_node, key, value);
                ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
            }
            else {
                pos = insert_data(new_data_node, key, value);
                ret = std::pair<iterator, bool>(iterator(new_data_node, pos), true);
            }
            --top;
            if (root == old_data_node)
                root = new_data_node;
            insert_one(stack, top, new_data_node->key[new_data_node->size - 1], new_data_node);
        }
        else{
            rescale(old_data_node, parent, traits::EXPAND_RATIO);
        }
    }
    /* else insert the position of the data node*/ 
    else{
        size_type pos = insert_data(old_data_node, key, value);
        ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
    }

    AEX_FORMAT("root->prop=%u, size=%llu", this->root->prop, this->root->size);

    ++m_stats.size;

    AEX_FORMAT("END");
    return ret;
}

// insert an item(<key, data>) to data node
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::insert_data(data_node_ptr node, const key_type &key, const value_type &data){
    key_type* key_ptr = node->key;
    size_type pos = node->find_lower_pos(key);
    memmove(key_ptr + pos + 1, key_ptr + pos, (node->size - pos) * sizeof(key_type));
    data_memmove(node->data + pos + 1, node->data + pos, (node->size - pos) * sizeof(value_type));
    key_ptr[pos] = key;
    node->data[pos] = data;
    node->size++;
    if (node->prop & COMPLEX_MODEL)
        node->insert(pos);
    
    AEX_FORMAT("END");
    return pos;
}

// Unused.
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_insert(const inner_node_ptr node, const key_type &key){
    AEX_ASSERT(node == nullptr);
    //if (node == nullptr) return false;
    if (!(node->prop & node_property::ML_NODE)) 
        return true;
    
    size_type inserted_pos = node->find_lower_pos(key), pred_pos = node->predict(key);

    // pos == node->slot_size represent key is > node->max_key. It's not allow.
    AEX_ASSERT(inserted_pos == node->slot_size);

    // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
    if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
        return node->slot_size;

    #ifdef AEX_DEBUG
    if (this->debug_level >= 1){
        for (size_type i = 0; i < node->slot_size; ++i){
            AEX_PRINT("pos=" << i << " key=" << node->key_ptr[i] << " child=" << node->child_ptr[i]);
        }
        AEX_PRINT("key=" << key << " pos=" << pos << " predict=" << node->predict(key));
    }
    #endif

    // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
    size_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, node->slot_size);
    bitmap bm = node->bitmap_ptr;
    for (size_type i = inserted_pos; i < max_slot; ++i){
        if (bitmap_impl::at(bm, i)){
            size_type shift_pos = node->predict(node->key_ptr[i]);
            if (i + 1 - shift_pos >= traits::ERROR_BOUND) 
                return false;
        }
        else{
            return true;
        }
    }

    // if need shift move more than ERROR_BOUND item, return false
    return false;
}

// try to insert an node to inner node. If no position to insert, return false
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::false_type allow_balance){
    if (node == nullptr) return false;
    if (!(node->prop & node_property::ML_NODE)) {
        size_type pos = node->find_lower_pos(key);
        memmove(node->key_ptr + pos + 1, node->key_ptr + pos, (node->size - pos) * sizeof(key_type));
        memmove(node->child_ptr + pos + 1, node->child_ptr + pos, (node->size - pos) * sizeof(node_ptr));
        ++node->slot_bound;
        ++node->size;
        node->m_stats.data_size += node->data_size();
        node->m_stats.data_node += node->data_node_size();
        return true;
    }
    else{
        bitmap bm = node->bitmap_ptr;
        size_type inserted_pos = node->find_lower_pos(key), pred_pos = node->predict(key);
        if (inserted_pos == node->slot_size)
            return false;
        // pos == node->slot_size represent key is > node->max_key. It's not allow.
        // no multi key
        AEX_ASSERT(node->key_ptr[inserted_pos] == key);
        
        // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
        if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
            return false;

        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            for (size_type i = 0; i < node->slot_size; ++i){
                AEX_PRINT("pos=" << i << " key=" << node->key_ptr[i] << " child=" << node->child_ptr[i]);
            }
            AEX_PRINT("key=" << key << " pos=" << pos << " predict=" << node->predict(key));
        }
        #endif
        
        // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
        size_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, node->slot_size);
        for (size_type i = inserted_pos; i < max_slot; ++i){
            if (bitmap_impl::at(bm, i)){
                size_type shift_pos = node->predict(node->key_ptr[i]);
                if (i + 1 - shift_pos >= traits::ERROR_BOUND) 
                    return false;
            }
            else{
                node->slot_bound = std::max(node->slot_bound, i);
                memmove(node->key_ptr + inserted_pos + 1, node->key_ptr + inserted_pos, (i - inserted_pos - 1) * sizeof(key_type));
                memmove(node->child_ptr + inserted_pos + 1, node->child_ptr + inserted_pos, (i - inserted_pos - 1) * sizeof(node_ptr));
                bitmap_impl::set_one(bm, i);
                size_type prev_pos = node->prev_item(inserted_pos);
                prev_pos = (prev_pos == node->slot_size) ? 0 : prev_pos + 1;
                for (size_type i = prev_pos; i <= inserted_pos; ++i){
                    node->key_ptr[i] = key;
                    node->child_ptr[i] = child;
                }
                ++node->size; 
                node->m_stats.data_size += child->data_size();
                node->m_stats.data_node += child->data_node_size();
                return true;
            }
        }
        // if need shift move more than ERROR_BOUND item, return false
        return false;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::true_type allow_balance){
    if (node == nullptr) return false;
    if (!(node->prop & node_property::ML_NODE)) {
        size_type pos = find_lower(node, key);
        memmove(node->key_ptr + pos + 1, node->key + pos, (node->size - pos) * sizeof(key_type));
        memmove(node->key_ptr + pos + 1, node->key + pos, (node->size - pos) * sizeof(node_ptr));
        ++node->slot_bound;
        ++node->size;
        node->m_stats.data_size += child->data_size();
        node->m_stats.data_node += child->data_node_size();
        return true;
    }
    else{
        bitmap bm = node->bitmap_ptr;
        size_type inserted_pos = node->find_lower_pos(key), pred_pos = node->predict(key);
        if (inserted_pos == node->slot_size)
            return false;
        // pos == node->slot_size represent key is > node->max_key. It's not allow.
        // no multi key
        AEX_ASSERT(node->key_ptr[inserted_pos] == key);
        
        // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
        //if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
        //    return false;


        {
            node_ptr node_buffer[traits::ERROR_BOUND];
            size_type buffer_size = 0;
            for (size_type i = pred_pos; i < inserted_pos; ++i){
                if (bitmap_impl::at(bm, i)){
                    AEX_ASSERT(node->predict(node->key_ptr[i]) > pred_pos);
                    node_buffer[buffer_size] = node->child_ptr[i];
                    ++buffer_size;
                }
            }
            if (check_insert_balance(node_buffer, buffer_size)){
                node_ptr new_node = balance_merge_node(node_buffer, buffer_size);
                key_type new_key = new_node->max_key();
                size_type prev_pos = node->prev_item(pred_pos);
                prev_pos = (prev_pos == node->slot_size) ? 0 : prev_pos + 1;
                
                
            }
        }

        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            for (size_type i = 0; i < node->slot_size; ++i){
                AEX_PRINT("pos=" << i << " key=" << node->key_ptr[i] << " child=" << node->child_ptr[i]);
            }
            AEX_PRINT("key=" << key << " pos=" << pos << " predict=" << node->predict(key));
        }
        #endif
        
        // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
        size_type max_slot = std::min(pred_pos + traits::ERROR_BOUND, node->slot_size);
        for (size_type i = inserted_pos; i < max_slot; ++i){
            if (bitmap_impl::at(bm, i)){
                size_type shift_pos = node->predict(node->key_ptr[i]);
                if (i + 1 - shift_pos >= traits::ERROR_BOUND) 
                    return false;
            }
            else{
                node->slot_bound = std::max(node->slot_bound, i);
                memmove(node->key_ptr + inserted_pos + 1, node->key_ptr + inserted_pos, (i - inserted_pos - 1) * sizeof(key_type));
                memmove(node->child_ptr + inserted_pos + 1, node->child_ptr + inserted_pos, (i - inserted_pos - 1) * sizeof(node_ptr));
                bitmap_impl::set_one(bm, i);
                size_type prev_pos = node->prev_item(inserted_pos);
                prev_pos = (prev_pos == node->slot_size) ? 0 : prev_pos + 1;
                for (size_type i = prev_pos; i <= inserted_pos; ++i){
                    node->key_ptr[i] = key;
                    node->child_ptr[i] = child;
                }
                ++node->size;
                node->m_stats.data_size += child->data_size();
                node->m_stats.data_node_size += child->data_node_size();
                return true;
            }
        }
        // if need shift move more than ERROR_BOUND item, return false
        return false;
    }
}

// Split an node if the node insert item and the size is larger than upper bound
// if the node is replaced, the node will free
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_split(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ parent, const key_type* const key, const node_ptr* const child, const unsigned int n,
               std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    key_type* key_buf = node_allocator.allocate_key_buffer((node->size + n));
    node_ptr* child_buf = node_allocator.allocate_nodeptr_buffer((node->size + n));
    bitmap bm = node->bitmap_ptr;

    size_type j = 0, n_slot = 0;
    AEX_FORMAT("BEGIN");
    /* merge key_buffer and node to alloc_key_buf */
    if (node->prop & node_property::ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            while (j < n && key[j] < node->key_ptr[i]){
                key_buf[n_slot] = key[j];
                child_buf[n_slot] = child[j];
                n_slot++;
                j++;
            }
            key_buf[n_slot] = node->key_ptr[i];
            child_buf[n_slot] = node->child_ptr[i];
            n_slot++;
        }
    }
    else{
        for (size_type i = 0; i < node->slot_size; ++i){
            while (j < n && key[j] < node->key_ptr[i]){
                key_buf[n_slot] = key[j];
                child_buf[n_slot] = child[j];
                n_slot++;j++;
            }
            key_buf[n_slot] = node->key_ptr[i];
            child_buf[n_slot] = node->child_ptr[i];
            n_slot++;
        }
    }

    if (j < n){
        memcpy(key_buf + n_slot, key + j, (n - j) * sizeof(key_type));
        memcpy(child_buf + n_slot, child + j, (n - j) * sizeof(node_ptr));
    }

    AEX_FORMAT("size=%llu" << node->size + n);
    
    // split. if old node is not used, replace flag is true.
    bool replace_flag = split_with_old_node(key_buf, child_buf, node->size + n, new_key, new_child, node);
    for (size_type i = 0; i < node->size + n; ++i)
        new_child[i]->level = node->level;

    // update 
    // parent           --->        parent 
    //  ...\                         ...\.
    //  ....old_node                 ....[back_node]
    if (node != new_child.back()) {
        if (parent != nullptr)
            update_childnode_ptr(parent, node, new_child.back());
        if (root == node)
            root = new_child.back();
    }
    new_key.pop_back();
    new_child.pop_back();

    if (replace_flag)
        node_allocator.free_node(node);

    node_allocator.deallocate(key_buf);
    node_allocator.deallocate(child_buf);
    AEX_FORMAT("END");
    return replace_flag;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::build_tree(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf){
    AEX_ASSERT(key_buffer.size() == child_buffer.size());
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    unsigned int height = child_buf[0]->level;
    while (key_buf.size() > 1){
        split(key_buf.data(), child_buf.data(), key_buf.size(), 0, new_key_buf, new_child_buf);

        size_type m = new_child_buf.size();
        new_child_buf[0]->prev = nullptr;
        new_child_buf[m - 1]->next = nullptr;
        for(size_type i = 0; i < m - 1; ++i){
            new_child_buf[i + 1]->prev = new_child_buf[i];
            new_child_buf[i]->next = new_child_buf[i + 1];
        }
        key_buf = std::move(new_key_buf);
        child_buf = std::move(new_child_buf);
        ++height;
    }

    {
        root = child_buf[0];
        this->m_stats.height = height;
    }
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
    /* 
    * TODO: use buffer instead of vector
    */
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    this->deconstruct(this->root);
    for (size_type i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
    }

    split(key_buf.data(), data_buf.data(), nums, new_key_buf, new_child_buf);
    
    size_type m = new_child_buf.size();
    new_child_buf[0]->prev = nullptr;
    new_child_buf[m - 1]->next = nullptr;
    for(size_type i = 0; i < m - 1; ++i){
        new_child_buf[i + 1]->prev = new_child_buf[i];
        new_child_buf[i]->next = new_child_buf[i + 1];
    }
    
    this->m_stats.size = nums;

    this->head_leaf = static_cast<data_node_ptr>(new_child_buf[0]);
    this->tail_leaf = static_cast<data_node_ptr>(new_child_buf[new_child_buf.size() - 1]);
    
    this->build_tree(new_key_buf, new_child_buf);

}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_one(const node_ptr* stack, int top, const key_type &key, const node_ptr child){
    std::vector<key_type> key_buf(1);
    std::vector<node_ptr> child_buf(1);
    key_buf[0] = key;
    child_buf[0] = child;
    insert_many(stack, top, key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_many(const node_ptr* stack, int top, std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    inner_node_ptr now_node;
    bool new_child_flag = (key_buf.size() > 0);
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    size_type num_buf = key_buf.size(), new_num_buf = 0;

    while (top > 0 && new_child_flag){
        now_node = static_cast<inner_node_ptr>(stack[top - 1]);
        inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 1]);
        if (new_child_flag){
            new_child_flag = false;
            for (size_type i = 0; i < num_buf; ++i){
                /* if current node is full */
                if (isfull(now_node)) {
                    if (rescale(now_node, parent, traits::EXPAND_RATIO)) {
                        AEX_FORMAT("EXPAND SUCCESS");
                    }
                    else{
                        // it doesn't happen normally
                        AEX_ASSERT(true == false);
                        new_child_flag = true;
                        AEX_FORMAT("INNER NODE SPLIT");
                        insert_split(now_node, parent, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                        new_num_buf = new_key_buf.size();
                        break;
                    }
                }

                {
                    AEX_FORMAT("CHECK INSERT");
                    /* if can insert, then insert it */
                    if (insert_node(now_node, key_buf[i], child_buf[i], this->allow_balance)){
                    }
                    /* else check if insert it after rewire it 
                        TODO: bulk insert */
                    else if (rewired(now_node) && insert_node(now_node, key_buf[i], child_buf[i], this->allow_balance)){
                    }
                    /* else split it */
                    else{
                        new_child_flag = true;
                        insert_split(now_node, parent, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                        new_num_buf = new_key_buf.size();
                        break;
                    }
                }
            }
        }
        /* swap buffer */
        key_buf = std::move(new_key_buf);
        child_buf = std::move(new_child_buf);
        num_buf = new_num_buf;
        new_num_buf = 0;
    }

    /* if new child, create a new root */
    if (new_child_flag){
        inner_node_ptr now_inner_node = nullptr;
        now_inner_node = node_allocator.allocate_inner_node(num_buf, this->m_stats.timestamp);
        ++this->m_stats.inner_node;
        now_inner_node->level = stack[1]->level + 1;
        now_inner_node->base_stats.recent_update_timestamp = this->m_stats.timestamp;
        now_inner_node->prev = now_inner_node->next = nullptr;
        key_buf.push_back(((root->prop & LEAF) ? (static_cast<data_node_ptr>(root)->key[root->size - 1]) : 
                            (static_cast<inner_node_ptr>(root)->key_ptr[static_cast<inner_node_ptr>(root)->last()])));
        child_buf.push_back(root);
        now_inner_node->construct(key_buf.data(), child_buf.data(), num_buf + 1);
        root = now_inner_node;
        AEX_FORMAT("root->prop=%llu, size=%llu", this->root->prop, this->root->size);
        ++m_stats.height;
    }
}



}