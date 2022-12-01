#pragma once
#include "aex/aex.h"

namespace aex{

/* require: update_childnode_key, find_upper, node_allocator::allocate_data_node, node_allocator::allocate_inner_node, insert_data, insert_one*/
template<typename _Key, typename _Val, typename traits>
std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    //node *_stack[traits::MAX_LEVEL], *now_node;
    node_ptr now_node;
    // the new child flag is true if  node splited
    bool new_child_flag = false;
    // the update flag is true if and only if only the key is the max key of the tree,
    bool update_max_key_flag = false;
    if (tail_leaf == nullptr)
        update_max_key_flag = true;
    else if (tail_leaf->key[tail_leaf->size - 1] < key)
        update_max_key_flag = true; 

    size_type  num_buf = 0, new_num_buf;
    // key_buf: key buffer child_buf: child pointer buffer; store when split node
    std::vector<key_type> key_buf, new_key_buf;
    std::vector<node_ptr> child_buf, new_child_buf;
    unsigned int dfs_level;
    std::pair<iterator, bool> ret;

    AEX_PRINT("BEGIN");

    if (root == nullptr){
        root = head_leaf = tail_leaf = node_allocator::allocate_data_node();
        ++m_stats.data_node;
    }
    AEX_PRINT("level=" << root->level << ", size=" << this->m_stats.size << ", root_size=" << root->size);        

    now_node = root;
    //AEX_PRINT(&root << " " << now_data_node);

    AEX_PRINT("FIND PATH");
    /* find the path */

    if (update_max_key_flag){
        node_ptr son = tail_leaf;
        for (inner_node_ptr node = static_cast<inner_node_ptr>(tail_leaf->parent); node != nullptr; son = node, node = node->parent_node){
            size_type pos = node->last();
            update_childnode_key(node, son, key);
        }
    }

    AEX_PRINT("FIND PATH END");
    auto iter = find_upper(key);
    node_ptr now_node = iter->_M_node;

    /* insert to data node */
    AEX_PRINT("INSERT DATA NODE");
    data_node_ptr now_data_node = static_cast<data_node_ptr>(now_node);

    data_node_ptr old_data_node;
    data_node_ptr new_data_node;

    // check should the node split and do it if it should.
    split(now_data_node);

    /* find the insert position */
    size_type pos = find_lower_pos(now_data_node, key);
    if (!std::is_same<typename traits::AllowMultiKey, std::true_type>::value && pos < traits::DATA_NODE_SLOT_SIZE && now_data_node->key[pos] == key){
        return std::pair<iterator, bool>(end(), false);
    }

    /* if data node is full, split the node */
    if (isfull(now_data_node)){
        AEX_PRINT("INSERT DATA NODE SPLIT");
        // data_node => [old_data_node, new_data_node]
        // parent_node->ptr = [... data_node ...] => [... old_data_node(need insert), new_data_node ... ] 
        old_data_node = now_data_node;
        new_data_node = node_allocator::allocate_data_node();
        ++m_stats.data_node;
        split(old_data_node, new_data_node);
        if (pos < old_data_node->size){
            pos = insert_data(old_data_node, key, value);
            ret = std::pair<iterator, bool>(iterator(old_data_node, pos), true);
        }
        else {
            pos = insert_data(new_data_node, key, value);
            ret = std::pair<iterator, bool>(iterator(new_data_node, pos), true);
        }
        update_childnode_ptr(old_data_node->parent, old_data_node, new_data_node);
        if (root == old_data_node)
            root = new_data_node;
        new_child_flag = true;
        key_buf.push_back(old_data_node->key[old_data_node->size - 1]);
        child_buf.push_back(old_data_node);
        num_buf = 1;
    }
    /* else insert the position of the data node*/ 
    else{
        size_type pos = insert_data(now_data_node, key, value);
        ret = std::pair<iterator, bool>(iterator(now_data_node, pos), true);
    }

    AEX_PRINT("root->prop=" << this->root->prop << " , size=" << this->root->size);

    // if a node is split, recursive insert in inner node
    if (new_child_flag) 
        insert_one(now_node->parent, new_node->last_key[new_node->size - 1], new_node);

    ++m_stats.size;
    AEX_PRINT("END");
    return ret;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::insert_data(data_node_ptr node, const key_type &key, const value_type &data){
    key_type* key_ptr = node->key;
    size_type pos = find_lower_pos(node, key);
    memmove(key_ptr + pos + 1, key_ptr + pos, (node->size - pos) * sizeof(key_type));
    data_memmove(node->data + pos + 1, node->data + pos, (node->size - pos) * sizeof(value_type));
    key_ptr[pos] = key;
    node->data[pos] = data;
    node->size++;
    if (node->prop & COMPLEX_MODEL)
        node->model->insert(key);
    else {
        
    }
    AEX_PRINT("END");
    return pos;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::insert_node(inner_node_ptr node, const key_type &key, const node_ptr child){
    AEX_PRINT("BEGIN. key=" << key);
    size_type pos = find_upper_pos(node, key);
    if (pos == node->slot_size) pos = std::max(node->predict(key), node->last() + 1);
    key_type* node_key = node->key_ptr;
    node_ptr* node_child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;
    AEX_PRINT("pos=" << pos);
    if (node->prop & ML_NODE){
        size_type empty_slot = pos;

        #ifdef AEX_DEBUG
        if (this->debug_level >= 1){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i))
                AEX_PRINT("key=" << node_key[i] << " pos=" << i << " child=" << node_child[i]);
        }
        #endif

        for (size_type i = pos; i < pos + traits::ERROR_BOUND; ++i)
        if (!bitmap_impl::at(bm, i)){
            empty_slot = i;
            break;
        }
        AEX_PRINT("empty_slot=" << empty_slot);
        // shift item to next position
        
        memmove(node_key + pos + 1, node_key + pos, (empty_slot - pos) * sizeof(key_type));
        memmove(node_child + pos + 1, node_child + pos, (empty_slot - pos) * sizeof(node_ptr));            
        
        size_type prev_pos = node->prev(pos);
        AEX_PRINT("prev_pos=" << prev_pos << "pos=" << pos);
        prev_pos = (prev_pos == node->slot_size) ? 0 : prev_pos + 1;
        for (size_type i = prev_pos; i <= pos; ++i){
            node_key[i] = key;
            node_child[i] = child;
        }
        if (bitmap_impl::at(bm, pos)){
            bitmap_impl::set_one(bm, empty_slot);
        }
        else{
            bitmap_impl::set_one(bm, pos);
        }
        node->size++;

        return pos;
    }
    else{
        memmove(node_key + pos +  1, node_key + pos, (node->size - pos) * sizeof(key_type));
        memmove(node_child + pos + 1, node_child + pos, (node->size - pos) * sizeof(node_ptr));
        node_key[pos] = key;
        node_child[pos] = child;
        node->size++;
        AEX_PRINT("pos=" << pos << "\n END");
        return pos;
    }
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::check_insert(const inner_node_ptr node, const key_type &key){
    if (node == nullptr) return false;
    if (!(node->prop & ML_NODE)) return true;
    /* TODO: 
    * optimization: use bit operartion
    */
    
    key_type* node_key = node->key_ptr;
    size_type pos = find_upper_pos(node, key), pred_pos = node->predict(key);
    pos = (pos == node->slot_size) ? std::min(node->slot_size - 1, std::max(node->last() + 1, pred_pos)) : pos;
    #ifdef AEX_DEBUG
    if (this->debug_level >= 1){
        for (size_type i = 0; i < node->slot_size; ++i){
            AEX_PRINT("pos=" << i << " key=" << node->key_ptr[i] << " child=" << node->child_ptr[i]);
        }
        AEX_PRINT("key=" << key << " pos=" << pos << " predict=" << node->predict(key));
    }
    #endif
    // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
    if (pos - pred_pos >= traits::ERROR_BOUND) return false;
    bitmap bm = node->bitmap_ptr;
    size_type max_slot = std::min(node->slot_size, pos + traits::ERROR_BOUND);
    for (size_type i = pos; i < max_slot; ++i)
    if (key < node_key[i]){
        pos = i;
        break;
    }
    // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
    max_slot = std::min(pos + traits::ERROR_BOUND, node->slot_size);
    for (size_type i = pos; i < max_slot; ++i)
    if (bitmap_impl::at(bm, i)){
        size_type shift_pos = node->predict(node_key[i]);
        if (i + 1 - shift_pos >= traits::ERROR_BOUND) return false;
    }
    else{
        return true;
    }

    // if need shift move more than ERROR_BOUND item, return false
    return false;
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::insert_split(inner_node_ptr node, inner_node_ptr parent, const key_type* const key, const node_ptr* const child, 
               const unsigned int n,
               std::vector<key_type> &new_key, std::vector<node_ptr> &new_child){
    key_type* key_buf = allocator::allocate_key_buffer((node->size + n));
    key_type* node_key = node->key_ptr;
    node_ptr* child_buf = allocator::allocate_nodeptr_buffer((node->size + n));
    node_ptr* node_child = node->child_ptr;
    bitmap bm = node->bitmap_ptr;

    size_type j = 0, n_slot = 0;
    AEX_PRINT("BEGIN");
    /* merge key_buffer and node */
    if (node->prop & ML_NODE){
        for (size_type i = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)){
            while (j < n && key[j] < node_key[i]){
                key_buf[n_slot] = key[j];
                child_buf[n_slot] = child[j];
                n_slot++;j++;
            }
            key_buf[n_slot] = node_key[i];
            child_buf[n_slot] = node_child[i];
            n_slot++;
        }
    }
    else{
        for (size_type i = 0; i < node->slot_size; ++i){
            while (j < n && key[j] < node_key[i]){
                key_buf[n_slot] = key[j];
                child_buf[n_slot] = child[j];
                n_slot++;j++;
            }
            key_buf[n_slot] = node_key[i];
            child_buf[n_slot] = node_child[i];
            n_slot++;
        }
    }

    if (j < n){
        memcpy(key_buf + n_slot, key + j, (n - j) * sizeof(key_type));
        memcpy(child_buf + n_slot, child + j, (n - j) * sizeof(node_ptr));
    }
    AEX_PRINT("size=" <<  node->size + n);
    /* split */
    bool replace_flag = split_with_old_node(key_buf, child_buf, node->size + n, node->level, new_key, new_child, node);
    
    if (node != new_child.back()) {
        if (parent != nullptr)
            update_childnode_ptr(parent, node, new_child.back());
        if (root == node)
            root = new_child.back();
    }
    new_key.pop_back();
    new_child.pop_back();

    if (replace_flag){
        node_allocator::free(node);
    }

    allocator::_free(key_buf);
    allocator::_free(child_buf);
    AEX_PRINT("END");
    return replace_flag;
}

template<typename _Key, typename _Val, typename traits>
void bulk_load_node(std::vector<key_type>& key_buf, std::vector<node_ptr>& child_buf){
    bool new_child_flag = true;
    AEX_ASSERT(key_buffer.size() == child_buffer.size());
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;
    int level = child_buf[0]->level;
    while (key_buf.size() > 1){
        split(key_buf.data(), child_buf.data(), new_key_buf, new_child_buf);
        key_buf = std::move(new_key_buf);
        child_buf = std::move(new_child_buf);
        ++level;
    }

    {
        root = child_buf[0];
        this->m_stats.level = level;
    }
}

template<typename _Key, typename _Val, typename traits>
void bulk_load(const std::pair<key_type, value_type>* const data, const size_type nums){
    size_type n = 0;
    /* 
    * TODO: use buffer instead of vector
    */
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    data_node_ptr data_node;
    inner_node_ptr inner_node;
    size_type size, leaf_slot_size = traits::MIN_DATA_NODE_SLOT_SIZE, cnt = 0;
    double min_cost, cost = 0;
    unsigned int level = 0;
    
    this->deconstruct(this->root);

    while (slot_size < nums && leaf_slot_size < traits::MAX_DATA_NODE_SLOT_SIZE) leaf_slot_size <<= 1;
    
    for (size_type i = 0, cnt = 0; i < nums; i += leaf_slot_size){
        size_type leaf_size = std::min(nums - i, leaf_slot_size);
        data_node = node_allocator::allocate_data_node(leaf_slot_size);
        data_node->construct(data + i, leaf_size);
        data_node->size = leaf_size;
        key_buf.push_back(data_node->key[data_node->size - 1]);
        child_buf.push_back(data_node);
    }
    this->head_leaf = child_buf[0];
    this->tail_leaf = child_buf[child_buf.size() - 1];
    
    this->bulk_load(key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_many(inner_node_ptr node, vector<key_type> &key_buf, vector<node_ptr>& child_buf){
    insert_subtree(node, key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_one(inner_node_ptr node, const key_type &key, const node_ptr child){
    bool new_child_flag = true;
    std::vector<key_type> key_buf(1);
    std::vector<node_ptr> child_buf(1);
    key_buf[0] = key;
    child_buf[0] = child;
    insert_subtree(node, key_buf, child_buf);
}

template<typename _Key, typename _Val, typename traits>
void aex_tree<_Key, _Val, traits>::insert_subtree(inner_node_ptr node, const vector<key_type> &key_buf, const vector<node_ptr> &child_buf){
    inner_node_ptr now_node = node;
    bool new_child_flag = true;
    size_type dfs_level = node->level;
    std::vector<key_type> new_key_buf;
    std::vector<node_ptr> new_child_buf;

    while (now_node != nullptr && new_child_flag){
        inner_node_ptr parent = now_node->parent;
        if (new_child_flag){
            new_child_flag = false;
            for (size_type i = 0; i < num_buf; ++i){
                /* if current node is full */
                if (isfull(now_inner_node)) {
                    if (expand(now_inner_node, parent_node)) {
                        AEX_PRINT("EXPAND SUCCESS");
                    }
                    else{
                        new_child_flag = true;
                        AEX_PRINT("INNER NODE SPLIT");
                        insert_split(now_inner_node, parent_node, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                        new_num_buf = new_key_buf.size();
                        break;
                    }
                }

                {
                    AEX_PRINT("CHECK INSERT");
                    /* if can insert, then insert it */
                    if (check_insert(now_inner_node, key_buf[i])){
                        _insert(now_inner_node, key_buf[i], child_buf[i]);
                    }
                    /* else check if insert it after rewire it 
                        TODO: bulk insert */
                    else if (rewired(now_inner_node) && check_insert(now_inner_node, key_buf[i])){
                        _insert(now_inner_node, key_buf[i], child_buf[i]);
                    }
                    /* else split it */
                    else{
                        new_child_flag = true;
                        insert_split(now_inner_node, parent_node, key_buf.data() + i, child_buf.data() + i, num_buf - i, new_key_buf, new_child_buf);
                        new_num_buf = new_key_buf.size();
                        break;
                    }
                }
            }
            ++dfs_level;
        }
        /* swap buffer */
        key_buf = std::move(new_key_buf);
        child_buf = std::move(new_child_buf);
        num_buf = new_num_buf;
        new_num_buf = 0;
        node = node->parent;
    }

    /* if new child, create a new root */
    if (new_child_flag){
        inner_node_ptr now_inner_node = nullptr;
        now_inner_node = node_allocator::allocate_inner_node(num_buf);
        now_inner_node->level = dfs_level;
        ++m_stats.inner_node;
        key_buf.push_back(((root->prop & LEAF) ? (static_cast<data_node_ptr>(root)->key[root->size - 1]) : 
                            (static_cast<inner_node_ptr>(root)->key_ptr[static_cast<inner_node_ptr>(root)->last()])));
        child_buf.push_back(root);
        now_inner_node->construct(key_buf.data(), child_buf.data(), num_buf + 1);
        root = now_inner_node;
        AEX_PRINT("root->prop=" << this->root->prop << " , size=" << this->root->size);
        ++m_stats.height;
    }
}



}