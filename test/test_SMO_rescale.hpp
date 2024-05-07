#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_node_rescale_perf(key_type* key, size_t num_keys, double ratio, int level){
    AEX_HINT("[test SMO--inner node rescale ]");
    mock_aex_tree<key_type, value_type> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    [[maybe_unused]] typedef typename traits::slot_type slot_type;
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;
    std::sort(key, key + num_keys);
    node_ptr* nodeptr_buffer = new node_ptr[num_keys];
    construct_data_node_array<key_type, value_type, node_ptr>(key, num_keys, nodeptr_buffer);

    std::vector<key_type> key_buf;
    std::vector<inner_node_ptr> inner_node_buf;
    std::sort(key, key + num_keys);
    tree.split(key, nodeptr_buffer, num_keys, level, key_buf, inner_node_buf);
    if (inner_node_buf.size() > 1){
        AEX_ERROR("can't split to one inner node, node[0]->slot_size=" << inner_node_buf[0]->slot_size << ", node[0]->size=" << inner_node_buf[0]->size << ", split to " << inner_node_buf.size() << " nodes.");
    }
    
    inner_node_ptr node = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(inner_node_buf[0])->real_slot_size(), IS_ML_NODE(inner_node_buf[0]));
    *node = *static_cast<inner_node_ptr>(inner_node_buf[0]);
    if (tree.rescale(node, static_cast<slot_type>(node->real_slot_size() * ratio)) == false){
        AEX_ERROR("rescale false!");
        return false;
    }

    if (!IS_ML_NODE(node)){
        for (slot_type i = 0; i < node->size - 1; ++i)
            if (node->key_ptr[i] != key[i]){
                AEX_ERROR("key error! i=" << i << "node key=" << node->key_ptr[i] << ", real key=" << key[i]);
                return false;
            }
    }
    else{
        for (slot_type i = 0, cnt = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(node->bitmap_ptr, i)){
            if (node->key_ptr[i] != key[cnt]){
                AEX_ERROR("key error! i=" << i << "node key=" << node->key_ptr[i] << ", real key=" << key[cnt]);
                return false;
            }
            cnt++;
        }
    }
    AEX_SUCCESS("test success!");
    
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    std::vector<inner_node_ptr> test_node_array(ITER);
    for (int i = 0; i < ITER; ++i){
        test_node_array[i] = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(inner_node_buf[0])->real_slot_size(), IS_ML_NODE(inner_node_buf[0]));
        *test_node_array[i] = *static_cast<inner_node_ptr>(inner_node_buf[0]);
    }
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i){
        tree.rescale(test_node_array[i], test_node_array[i]->real_slot_size() * ratio);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta;
    AEX_SUCCESS("split time=" << delta << "ms, OPS=" << OPS);
    for (size_type i = 0; i < num_keys; ++i)
        tree.allocator.free_node(nodeptr_buffer[i]);
    tree.allocator.deallocate(nodeptr_buffer);
    return true;
}