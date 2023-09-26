#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_data_split_with_exponential_probe_perf(key_type* key, value_type* data, size_t num_keys){
    AEX_HINT("[test SMO--data split ]");
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;

    mock_aex_tree<key_type, value_type, traits> tree;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> data_node_buf;
    std::sort(key, key + num_keys);
    tree.split_with_exponential_probe(key, data, num_keys, key_buf, data_node_buf);
    AEX_IMPORTANT("split data node size=" << data_node_buf.size());
    size_type cnt = 0, ml_node_cnt = 0;
    std::vector<value_type> node_data;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        cnt += data_node_buf[i]->size;
        ml_node_cnt += IS_ML_NODE(data_node_buf[i]);
    }
    if (cnt != static_cast<size_type>(num_keys)){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << ", data node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_type i = 0; i < data_node_buf.size(); ++i){
        node_ptr inode = data_node_buf[i];
        for (slot_type j = 0; j < inode->size; ++j, ++cnt){
            if (static_cast<data_node_ptr>(inode)->key[j] != key[cnt]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << static_cast<data_node_ptr>(inode)->key[j]);
                return false;
            }
        }
    }
    AEX_SUCCESS("mechine learing node rate=" << 1.0 * ml_node_cnt / data_node_buf.size());
    AEX_SUCCESS("test success! Next test performance...");
    std::chrono::system_clock::time_point t1, t2;
    const int ITER = 10;
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITER; ++i){
        key_buf.clear();
        data_node_buf.clear();
        tree.split_with_exponential_probe(key, data, num_keys, key_buf, data_node_buf);
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
    typedef typename traits::slot_type slot_type;

    mock_aex_tree<key_type, value_type, traits> tree;
    std::vector<key_type> key_buf;
    std::vector<node_ptr> data_node_buf;
    std::sort(key, key + num_keys);
    tree.split_with_linear_probe(key, data, num_keys, key_buf, data_node_buf);
    AEX_IMPORTANT("split data node size=" << data_node_buf.size());
    size_type cnt = 0, ml_node_cnt = 0;
    std::vector<value_type> node_data;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        cnt += data_node_buf[i]->size;
        ml_node_cnt += IS_ML_NODE(data_node_buf[i]);
    }
    if (cnt != num_keys){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "data node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_t i = 0; i < data_node_buf.size(); ++i){
        node_ptr inode = data_node_buf[i];
        for (slot_type j = 0; j < inode->size; ++j, ++cnt){
            if (static_cast<data_node_ptr>(inode)->key[j] != key[cnt]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << static_cast<data_node_ptr>(inode)->key[j]);
                return false;
            }
        }
        //for (slot_type j = 0; j < inode->size; ++j)
        //if (IS_ML_NODE(inode)){
        //    if (std::abs(j - static_cast<data_node_ptr>(inode)->predict(static_cast<data_node_ptr>(inode)->key[j])) > traits::ERROR_BOUND){
        //        AEX_ERROR("Model Error! j=" << j <<  "pred_pos=" << static_cast<data_node_ptr>(inode)->predict(static_cast<data_node_ptr>(inode)->key[j]) << "key[" << j << "]=" << key[j] << "size=" << inode->size);
        //        return false;
        //    }
        //}
    }

    AEX_SUCCESS("mechine learing node rate=" << 1.0 * ml_node_cnt / data_node_buf.size());
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
    std::sort(key, key + num_keys);
    node_ptr* nodeptr_buffer = tree.node_allocator.allocate_nodeptr_buffer(num_keys);
    construct_data_node_array<key_type, value_type, node_ptr>(key, num_keys, nodeptr_buffer);

    std::vector<key_type> key_buf;
    std::vector<node_ptr> inner_node_buf;
    std::sort(key, key + num_keys);
    tree.split(key, nodeptr_buffer, num_keys, 2, key_buf, inner_node_buf);
    size_type cnt = 0, ml_node_size = 0;
    std::vector<key_type> node_key(num_keys);
    for (size_t i = 0; i < inner_node_buf.size(); ++i){
        cnt += inner_node_buf[i]->size;
        ml_node_size += IS_ML_NODE(inner_node_buf[i]);
    }
    if (cnt != num_keys){
        AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "inner node key=" << cnt);
        return false;
    }
    cnt = 0;
    for (size_t i = 0; i < inner_node_buf.size(); ++i){
        node_ptr inode = inner_node_buf[i];
        tree.copy_to_buffer(static_cast<inner_node_ptr>(inode), node_key.data() + cnt);
        for (size_type j = cnt; j < cnt + inode->size - 1; ++j){
            if (node_key[j] != key[j]){
                AEX_ERROR("Key Error! key[" << cnt << "]=" << key[j] << "inner node key=" << node_key[j]);
                return false;
            }
        }
        cnt += inode->size;
    }
    AEX_SUCCESS("split inner node size=" << inner_node_buf.size() << "ml node size=" << 1.0 * ml_node_size / inner_node_buf.size());
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