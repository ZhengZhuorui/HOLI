#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_node_expand_perf(key_type* key, size_t num_keys){
    AEX_HINT("[test SMO--inner node expand]");
    aex_tree<key_type, value_type> tree;
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::hash_node_ptr  hash_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::slot_type slot_type;
    std::sort(key, key + num_keys);
    node_ptr* nodeptr_buffer = new node_ptr[num_keys];
    construct_data_node_array<key_type, value_type, node_ptr>(key, num_keys, nodeptr_buffer);

    std::vector<key_type> key_buf;
    std::sort(key, key + num_keys);
    inner_node_ptr node = tree.construct(node, key, nodeptr_buffer, num_keys);
    if (node->type != NodeType::HashNode){
        AEX_ERROR("Node type is not hash node");
        return false;
    }
    expand(node);
    tree.print_stats();
    AEX_SUCCESS("test success!");
    
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    std::vector<inner_node_ptr> test_node_array(ITER);
    for (int i = 0; i < ITER; ++i)
        test_node_array[i] = tree.construct(node, key, nodeptr_buffer, num_keys);
    
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i)
        expand(test_node_array[i]);
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta;
    AEX_SUCCESS("expand time=" << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_node_narrow_perf(key_type* key, size_t num_keys){
    return false;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_node_rebuild_perf(key_type* key, size_t num_keys){
    return false;
}
