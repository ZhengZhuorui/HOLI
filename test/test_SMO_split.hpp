#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_SMO_data_split_perf(key_type* key, value_type* data, size_t num_keys){
    AEX_HINT("[test SMO--data split ]");
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;

    mock_aex_tree<key_type, value_type, traits> tree;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> data_node_buf;
    std::sort(key, key + num_keys);
    tree.split(key, data, num_keys, key_buf, data_node_buf);
    AEX_IMPORTANT("split data node size=" << data_node_buf.size());
    size_type cnt = 0;
    std::vector<value_type> node_data;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        cnt += data_node_buf[i]->size;
    }
    if (cnt != static_cast<size_type>(num_keys)){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "data node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_type i = 0; i < data_node_buf.size(); ++i){
        node_ptr inode = data_node_buf[i];
        for (pos_type j = 0; j < inode->size; ++j, ++cnt){
            if (static_cast<data_node_ptr>(inode)->key[j] != key[cnt]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << static_cast<data_node_ptr>(inode)->key[j]);
                return false;
            }
        }
    }
    
    AEX_SUCCESS("test success! Next test performance...");
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i){
        key_buf.clear();
        data_node_buf.clear();
        tree.split(key, data, num_keys, key_buf, data_node_buf);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double NPS = 1.0 * 1e6 * num_keys * ITER / delta;
    AEX_SUCCESS("split time=" << delta << "ms, NPS=" << NPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_SMO_data_split_with_linear_probe_perf(key_type* key, value_type* data, size_t num_keys){
    AEX_HINT("[test SMO--data split with linear probe]");
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;

    mock_aex_tree<key_type, value_type, traits> tree;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> data_node_buf;
    std::sort(key, key + num_keys);
    tree.split_with_linear_probe(key, data, num_keys, key_buf, data_node_buf);
    AEX_IMPORTANT("split data node size=" << data_node_buf.size());
    size_type cnt = 0;
    std::vector<value_type> node_data;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        cnt += data_node_buf[i]->size;
    }
    if (cnt != num_keys){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "data node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        node_ptr inode = data_node_buf[i];
        for (pos_type j = 0; j < inode->size; ++j, ++cnt){
            if (static_cast<data_node_ptr>(inode)->key[j] != key[cnt]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << static_cast<data_node_ptr>(inode)->key[j]);
                return false;
            }
        }
    }
    
    AEX_SUCCESS("test success! Next test performance...");
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i){
        key_buf.clear();
        data_node_buf.clear();
        tree.split_with_linear_probe(key, data, num_keys, key_buf, data_node_buf);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double NPS = 1.0 * 1e6 * num_keys * ITER / delta;
    AEX_SUCCESS("split time=" << delta << "ms, NPS=" << NPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_SMO_node_split_perf(key_type* key, size_t num_keys){
    AEX_HINT("[test SMO--inner node split ]");
    mock_aex_tree<key_type, value_type> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    
    node_ptr* nodeptr_buffer = tree.node_allocator.allocate_nodeptr_buffer(num_keys);

    std::sort(key, key + num_keys);
    for (size_type i = 0; i < num_keys; ++i){
        nodeptr_buffer[i] = tree.node_allocator.allocate_data_node(1);
        static_cast<data_node_ptr>(nodeptr_buffer[i])->key[0] = key[i];
    }
    std::vector<key_type> key_buf;
    std::vector<node_ptr> inner_node_buf;
    std::sort(key, key + num_keys);
    tree.split(key, nodeptr_buffer, num_keys, 1, key_buf, inner_node_buf);
    AEX_IMPORTANT("split inner node size=" << inner_node_buf.size());
    size_type cnt = 0;
    std::vector<key_type> node_key(num_keys);
    for (size_t i = 0; i < inner_node_buf.size(); ++i){
        cnt += inner_node_buf[i]->size;
    }
    if (cnt != num_keys){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "inner node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_t i = 0; i < inner_node_buf.size(); ++i){
        node_ptr inode = inner_node_buf[i];
        tree.copy_to_buffer(static_cast<inner_node_ptr>(inode), node_key.data() + cnt);
        for (size_type j = cnt; j < cnt + inode->size; ++j){
            if (node_key[j] != key[j]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[j] << "inner node key=" << node_key[j]);
                return false;
            }
        }
        cnt += inode->size;
    }
    
    AEX_SUCCESS("test success! Next test performance...");
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i){
        key_buf.clear();
        inner_node_buf.clear();
        tree.split(key, nodeptr_buffer, num_keys, 1, key_buf, inner_node_buf);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double NPS = 1.0 * 1e6 * num_keys * ITER / delta;
    AEX_SUCCESS("split time=" << delta << "ms, NPS=" << NPS);
    for (size_type i = 0; i < num_keys; ++i)
        tree.node_allocator.free_node(nodeptr_buffer[i]);
    tree.node_allocator.deallocate(nodeptr_buffer);
    return true;
}