#pragma once
#include "aex/aex.h"

namespace aex{

/* require: node_allocator.allocate_data_node, node_allocator.allocate_inner_node, insert_data, insert_one*/
template<typename _Key, typename _Val, typename traits>
std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    AEX_PRINT("[index] insert(" << key << ", " << value << ")");

    ++this->m_stats.timestamp;
    update_tree_frequency();
    ++this->m_stats.write_times;
    
    node_ptr stack[traits::MAX_DEPTH];
    int top;

    std::pair<iterator, bool> ret;

    if (root == nullptr){
        root = head_leaf = tail_leaf = node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE);
        ++m_stats.data_node;
    }
    //AEX_FORMAT("level=%u, size=%lld, root_size=%lld", root->level, this->m_stats.size, this->root->data_size());

    AEX_PRINT("FIND PATH END");    
    data_node_ptr old_data_node, new_data_node;

    std::false_type fp;
    old_data_node = this->find_leaf_with_trace(key, stack, top, fp);
    //if (this->m_stats.max_key < key){
    //    [[mayby_unused]] bool flag = true;
    //    for (int i = 2; i < top; ++i)
    //        flag |= update_childnode_key(stack[i - 1], stack[i], key);
    //    AEX_ASSERT(flag == false);
    //}
    this->m_stats.min_key = std::min(this->m_stats.min_key, key);
    this->m_stats.max_key = std::max(this->m_stats.max_key, key);

    //if (top > 2){
    for (int i = 1; i < top; ++i){
        update_node_frequency(static_cast<inner_node_ptr>(stack[i]));
        stack[i]->base_stats.write_times++;
    }
    //}

    /* insert to data node */
    AEX_PRINT("INSERT DATA NODE");

    /* find the insert position */
    pos_type pos = old_data_node->find_upper_pos(key);
    if (traits::AllowMultiKey::value == false && pos < old_data_node->slot_size && old_data_node->key[pos] == key){
        return std::pair<iterator, bool>(end(), false);
    }

    /* if data node is full, split the node */
    if (isfull(old_data_node)){
        inner_node_ptr parent = static_cast<inner_node_ptr>(stack[top - 2]);
        if ((this->allow_balance == false) || check_balance_split(old_data_node)){
            AEX_PRINT("INSERT DATA NODE SPLIT");
            // data_node => [old_data_node, new_data_node]
            // parent_node->ptr = [... data_node ...] => [... old_data_node, new_data_node(need insert) ... ] 
            new_data_node = node_allocator.allocate_data_node(old_data_node->slot_size);
            ++m_stats.data_node;
            split(old_data_node, new_data_node);
            update_childnode_key(static_cast<inner_node_ptr>(stack[top - 2]), old_data_node, old_data_node->key[old_data_node->size - 1]);
            if (pos < old_data_node->size){
                pos = old_data_node->insert(key, value);
                ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
            }
            else {
                pos = new_data_node->insert(key, value);
                ret = std::pair<iterator, bool>(iterator(new_data_node, pos), true);
            }
            --top;
            insert_one(stack, top, new_data_node->key[new_data_node->size - 1], new_data_node);
        }
        else{
            rescale(old_data_node, parent, traits::EXPAND_RATIO);
            old_data_node->insert(key, value);
        }
    }
    /* else insert the position of the data node*/ 
    else{
        pos = old_data_node->insert(key, value);
        ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
    }

    //AEX_FORMAT("root->prop=%u, size=%lld", this->root->prop, this->root->size);
    AEX_PRINT("root->prop=" << this->root->prop << ", size=" << this->root->size);

    ++m_stats.size;

    AEX_PRINT("END");
    return ret;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::false_type allow_balance){
    AEX_ASSERT(node != nullptr);
    return node->insert(key, child);
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_node(const inner_node_ptr __restrict__ node, const key_type &key, const node_ptr __restrict__ child, std::true_type allow_balance){
    if (node == nullptr) return false;
    if (!(node->prop & node_property::ML_NODE)) {
        return node->insert(key, child);
    }
    else{
        bitmap bm = node->bitmap_ptr;
        pos_type pred_pos = node->predict(key);
        pos_type inserted_pos = pred_pos;
        for (; inserted_pos < this->slot_size && inserted_pos - pred_pos < traits::ERROR_BOUND; ++inserted_pos)
            if (this->key_ptr[inserted_pos] > key || !bitmap_impl::at(node->bitmap_ptr, inserted_pos)){
                break;
            }
        if (inserted_pos == node->slot_size)
            return false;

        // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
        if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
            return false;
        // pos == node->slot_size represent key is > node->max_key. It's not allow.
        // no multi key
        AEX_ASSERT(node->key_ptr[inserted_pos] == key);
        
        //if (inserted_pos - pred_pos >= traits::ERROR_BOUND)
        //    return false;

        {
            node_ptr node_buffer[traits::ERROR_BOUND + 1];
            size_type buffer_size = 0, first_slot = -1;
            for (size_type i = pred_pos; i <= inserted_pos; ++i){
                if (bitmap_impl::at(bm, i)){
                    AEX_ASSERT(node->predict(node->key_ptr[i]) >= pred_pos);
                    if (first_slot == -1)
                        first_slot = i;
                    node_buffer[buffer_size] = node->child_ptr[i];
                    ++buffer_size;
                }
            }
            node_buffer[buffer_size++] = child;
            if (check_balance_merge(node_buffer, buffer_size)){
                node_ptr new_node = balance_merge_nodes(node_buffer, buffer_size);
                if (new_node != nullptr){
                    pos_type prev_pos = node->prev_item(inserted_pos);
                    for (pos_type i = prev_pos + 1; i < pred_pos; ++i){
                        bitmap_impl::set_zero(node->bitmap_ptr, i);
                        node->key_ptr[i] = key;
                        node->child_ptr[i] = new_node;
                    }
                    if (inserted_pos < node->slot_size - 1){
                        for (pos_type i = pred_pos; i < inserted_pos; ++i){
                            node->key_ptr[i] = node->key_ptr[inserted_pos + 1];
                            node->child_ptr[i] = node->child_ptr[inserted_pos + 1];
                        }
                    }
                    bitmap_impl::set_one(node->bitmap_ptr, pred_pos);
                    
                    new_node->prev = node_buffer[0]->prev;
                    if (node_buffer[0]->prev != nullptr) 
                        node_buffer[0]->prev->next = new_node;
                    new_node->next = node_buffer[buffer_size - 1]->next;
                    if (node_buffer[buffer_size - 1]->next != nullptr) 
                        node_buffer[buffer_size - 1]->next->prev = new_node;
                    
                }
            }
        }
        return node->insert(key, child);
    }
}

// Split an node if the node insert item and the size is larger than upper bound
// if the node is replaced, the node will free
template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_split(inner_node_ptr __restrict__ node, inner_node_ptr __restrict__ parent, const key_type* const key, const node_ptr* const child, const pos_type n,
               std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    key_type* key_buf = node_allocator.allocate_key_buffer((node->size + n));
    node_ptr* child_buf = node_allocator.allocate_nodeptr_buffer((node->size + n));
    bitmap bm = node->bitmap_ptr;

    pos_type j = 0, n_slot = 0;
    AEX_PRINT("BEGIN");
    /* merge key_buffer and node to alloc_key_buf */
    if (node->prop & node_property::ML_NODE){
        for (pos_type i = 0; i < node->slot_size; ++i)
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
        for (pos_type i = 0; i < node->slot_size; ++i){
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
        std::copy(key + j, key + n, key_buf + n_slot);
        std::copy(child + j, child + n, child_buf + n_slot);
    }

    AEX_PRINT("size=" << node->size + n);
    
    // split. if old node is not used, replace flag is true.
    bool replace_flag = split_with_old_node(key_buf, child_buf, node->size + n, new_key, new_child, node);
        
    // update 
    // parent           --->        parent 
    //  ...\                         ...\.
    //  ....old_node                 ....[back_node]

    if (replace_flag){
        update_childnode_ptr(parent, node, new_child.back());
        node_allocator.free_node(node);
        --this->m_stats.inner_node;
    }
    [[maybe_unused]] bool flag = update_childnode_key(parent, new_child.back(), new_key.back());
    AEX_ASSERT(flag == true);

    new_key.pop_back();
    new_child.pop_back();

    AEX_PRINT("END");
    return replace_flag;
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::build_tree(std::vector<key_type> &key_buf, std::vector<node_ptr> &child_buf){
    AEX_ASSERT(key_buf.size() == child_buf.size());
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    //unsigned int height = child_buf[0]->level;
    this->m_stats.height = 0;
    while (key_buf.size() > 1){
        ++this->m_stats.height;
        split(key_buf.data(), child_buf.data(), key_buf.size(), this->m_stats.height, new_key_buf, new_child_buf);
        size_type m = new_child_buf.size();
        new_child_buf[0]->prev = nullptr;
        new_child_buf[m - 1]->next = nullptr;
        for(size_type i = 0; i < m - 1; ++i){
            new_child_buf[i + 1]->prev = new_child_buf[i];
            new_child_buf[i]->next = new_child_buf[i + 1];
        }
        key_buf = std::move(new_key_buf);
        child_buf = std::move(new_child_buf);
        AEX_PRINT(new_key_buf.size());
    }

    root = child_buf[0];
    
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
    AEX_HINT("[INDEX] bulk load");
    std::vector<key_type> key_buf(nums), new_key_buf;
    std::vector<node_ptr> new_child_buf;
    std::vector<value_type> data_buf(nums);
    
    this->deconstruct(this->root);
    for (size_type i = 0; i < nums; ++i){
        key_buf[i] = data[i].first;
        data_buf[i] = data[i].second;
        #ifdef AEX_DEBUG
        if (i > 0)
            AEX_ASSERT(key_buf[i - 1] <= key_buf[i]);
        #endif
    }
    this->m_stats.min_key = key_buf[0];
    this->m_stats.max_key = key_buf[nums - 1];
    
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
    this->tail_leaf = static_cast<data_node_ptr>(new_child_buf[m - 1]);
    
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
                        AEX_PRINT("EXPAND SUCCESS");
                    }
                    else{
                        // it doesn't happen normally
                        AEX_ASSERT(true == false);
                        new_child_flag = true;
                        AEX_PRINT("INNER NODE SPLIT");
                        insert_split(now_node, parent, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                        new_num_buf = new_key_buf.size();
                        break;
                    }
                }

                {
                    AEX_PRINT("CHECK INSERT");
                    /* if can insert, then insert it */
                    if (this->insert_node(now_node, key_buf[i], child_buf[i], this->allow_balance)){
                    }
                    /* else check if insert it after rewire it 
                        TODO: bulk insert */
                    else if (rewired(now_node) && this->insert_node(now_node, key_buf[i], child_buf[i], this->allow_balance)){
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
        inner_node_ptr now_inner_node = node_allocator.allocate_inner_node(num_buf);
        ++this->m_stats.inner_node;
        ++this->m_stats.height;
        now_inner_node->level = this->m_stats.height;
        now_inner_node->base_stats.recent_update_timestamp = this->m_stats.timestamp;
        now_inner_node->prev = now_inner_node->next = nullptr;
        key_buf.push_back(((root->prop & LEAF) ? (static_cast<data_node_ptr>(root)->key[0]) : 
                            (static_cast<inner_node_ptr>(root)->key_ptr[0])));
        child_buf.push_back(root);
        now_inner_node->construct(key_buf.data(), child_buf.data(), num_buf + 1);
        root = now_inner_node;
        //AEX_FORMAT("root->prop=%u, size=%lld", this->root->prop, this->root->size);
        ++m_stats.height;
    }
}



}