#pragma once



template<typename key_type,
        typename value_type,
        typename traits=test_allow_dynamic_data_node_traits<key_type, value_type> >
bool test_inner_node_insert_balance_perf(vector<key_type> &data, size_t n, size_t batch, int level){
    
    AEX_PRINT("[test data node insertion performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;

    vector<key_type> node_data(n);
    std::copy(data.begin(), data.end(), node_data.begin());
    vector<key_type> insert_data(batch);
    printf("prepare dataset...\n");
    split_dataset(node_data, insert_data, batch);
    printf("prepare dataset target 0\n");
    n -= batch;
    for (size_type i = 1; i < n; ++i)
        if (node_data[0] > node_data[i]) 
            std::swap(node_data[0], node_data[i]);
            
    for (size_type i = 0; i < batch; ++i)
        if (node_data[0] > insert_data[i]) 
            std::swap(node_data[0], insert_data[i]);
    std::sort(node_data.begin(), node_data.end());

    node_ptr* child_ptr = tree.node_allocator.allocate_nodeptr_buffer(n);
    node_ptr* insert_node_ptr = tree.node_allocator.allocate_nodeptr_buffer(batch);
    construct_data_node_array<key_type, value_type, node_ptr>(node_data.data(), node_data.size(), child_ptr);
    construct_data_node_array<key_type, value_type, node_ptr>(insert_data.data(), insert_data.size(), insert_node_ptr);

    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;
    tree.split(node_data.data(), child_ptr, n, level, key_buf, child_buf);
    AEX_PRINT("cosntruct finish.");

    if (child_buf.size() > 1){
        AEX_ERROR("can't be construct in a inner node.");
        return false;
    }
    if (IS_ML_NODE(child_buf[0]) == false){
        AEX_WARNING("node is not ml node");
        return false;
    }
    
    inner_node_ptr node = tree.node_allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), IS_ML_NODE(child_buf[0]));
    *node = *static_cast<inner_node_ptr>(child_buf[0]);

    size_type insert_failed = 0;
    vector<key_type> final_node_data(n);
    std::copy(node_data.data(), node_data.data() + n, final_node_data.data());

    bool full_flag = false;
    for (size_t i = 0; i < batch; ++i){       
        std::cout << "i=" << i << "key=" << insert_data[i] << ", node=" << insert_node_ptr[i] << std::endl;
        std::true_type tp;
        if (!tree.insert_node(node, insert_data[i], insert_node_ptr[i], tp)){
            ++insert_failed;
            AEX_PRINT("insert failed!");
        }
        else final_node_data.push_back(insert_data[i]);

        if (!full_flag && tree.isfull(node)){
            full_flag = true;
            AEX_ERROR("node slot full, slot size=" << node->slot_size);
        }
    }
    std::sort(final_node_data.begin(), final_node_data.end());
    printf("insert failed=%lld, fail ratio=%.4f\n", insert_failed, 1.0 * insert_failed / batch);
    AEX_PRINT("merge nodes=" << n + batch - insert_failed - node->size);

    size_type bit_cnt = 0;
    for (slot_type i = 0; i < node->slot_size; ++i){
        if (i > 1 && node->key_ptr[i] < node->key_ptr[i - 1]){
            AEX_ERROR("Key Error! slot[" << i << "]=" << node->key_ptr[i] << ", slot[" << i - 1 << "]=" << node->key_ptr[i - 1]);
            return false;
        }
        if (i < node->slot_size - 1 && bitmap_impl::at(node->bitmap_ptr, i)){
            if (node->key_ptr[i] == node->key_ptr[i + 1]){
                AEX_ERROR("Error slot[" << i << "] key changes, but bitmap is set");
                return false;
            }
            if (node->child_ptr[i] == node->child_ptr[i + 1] && node->key_ptr[i + 1] != std::numeric_limits<key_type>::max()){
                AEX_ERROR("Error slot[" << i << "] child no change, but bitmap is set");
                return false;
            }
        }

        if (i < node->slot_size - 1 && (!bitmap_impl::at(node->bitmap_ptr, i))){
            if (node->key_ptr[i] != node->key_ptr[i + 1]){
                AEX_ERROR("Error slot[" << i << "] key changes, but bitmap is empty");
                return false;
            }
            if (node->child_ptr[i] != node->child_ptr[i + 1]){
                AEX_ERROR("Error slot[" << i << "] child changes, but bitmap is empty");
                return false;
            }
        }
        bit_cnt += ((bitmap_impl::at(node->bitmap_ptr, i)) != 0);
    }
    if (bit_cnt != node->size){
        AEX_ERROR("bit one cnt not equal items, 1 bits=" << bit_cnt << " n=" << n + batch - insert_failed);
        return false;
    }
}