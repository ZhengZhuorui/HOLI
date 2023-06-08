#pragma once

//test inner node construction and insertion
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_insert_perf(vector<key_type> &data, size_t n, size_t batch){
    AEX_PRINT("[test data node insertion performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;
    pos_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    AEX_PRINT("ratio=" << tree.inner_node_few_ratio[1]);
    while (slot_size * tree.inner_node_few_ratio[1] < n - batch) slot_size <<= 1;
    slot_size >>= 1;
    inner_node_ptr node = tree.node_allocator.allocate_inner_node(slot_size);
    //printf("node slot size=%lld", node->slot_size);
    AEX_PRINT("node slot size" << node->slot_size);
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

    //printf("node slot size=%lld\n", node->slot_size);
    AEX_PRINT("node slot size=" << node->slot_size);
    node_ptr* child_ptr = tree.node_allocator.allocate_nodeptr_buffer(n);
    node_ptr* insert_node_ptr = tree.node_allocator.allocate_nodeptr_buffer(batch);

    for (size_type i = 0; i < n; ++i){
        child_ptr[i] = tree.node_allocator.allocate_data_node(1);
        data_node_ptr now_node = static_cast<data_node_ptr>(child_ptr[i]);
        now_node->key[0] = node_data[i];
        now_node->size = 1;
        std::cout << node_data[i] << " ";
    }
    std::cout << std::endl;

    for (size_type i = 0; i < batch; ++i){
        insert_node_ptr[i] = tree.node_allocator.allocate_data_node(1);
        data_node_ptr now_node = static_cast<data_node_ptr>(insert_node_ptr[i]);
        now_node->key[0] = insert_data[i];
        now_node->size = 1;
        std::cout << insert_data[i] << " ";
    }
    std::cout << std::endl;
    
    node->construct(node_data.data(), child_ptr, n);
    printf("construct finish.\n");
    //if (!(node->prop & aex::node_property::ML_NODE)){
    //    printf("Error! no ML NODE\n");
    //    return false;
    //}
    printf("is ml node?(0 or 1): %d\n", (((node->prop) & aex::node_property::ML_NODE) > 0));
    fflush(stdout);
    if (!(node->prop & aex::node_property::ML_NODE)) return false;
    size_type insert_failed = 0;
    vector<key_type> final_node_data(n);
    std::copy(node_data.data(), node_data.data() + n, final_node_data.data());

    for (size_t i = 0; i < batch; ++i){       
        std::cout << "i=" << i << "key=" << insert_data[i] << ", node=" << insert_node_ptr[i] << std::endl;
        if (!node->insert(insert_data[i], insert_node_ptr[i])){
            ++insert_failed;
            AEX_PRINT("insert failed!");
        }
        else final_node_data.push_back(insert_data[i]);
        if (tree.isfull(node)){
            AEX_ERROR("node slot full");
            return false;
        }
    }
    std::sort(final_node_data.begin(), final_node_data.end());
    assert(final_node_data.size() == n + batch - insert_failed);
    printf("insert failed=%lld, fail ratio=%.4f\n", insert_failed, 1.0 * insert_failed / batch);
    size_type bit_cnt = 0;
    //for (int i = 0; i < node->slot_size; ++i){
    //    std::cout << node->key_ptr[i] << " : " << node->child_ptr[i] << " : " << (bitmap_impl::at(node->bitmap_ptr, i) != 0) << " | ";
    //}

    for (pos_type i = 0; i < node->slot_size; ++i){
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
        if (bitmap_impl::at(node->bitmap_ptr, i)){
            if (node->key_ptr[i] != final_node_data[bit_cnt]){
                AEX_ERROR("Key error, node key[" << i<< "]=" << node->key_ptr[i] << ", real key=" << final_node_data[bit_cnt]);
                return false;
            }
        }
        bit_cnt += ((bitmap_impl::at(node->bitmap_ptr, i)) != 0);
    }
    if (bit_cnt != n + batch - insert_failed){
        AEX_ERROR("bit one cnt not equal items, 1 bits=" << bit_cnt << " n=" << n + batch - insert_failed);
        return false;
    }
    

    AEX_PRINT("test insert node performance");
    system_clock::time_point t1, t2;
    const int ITER = 1000;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        node->construct(node_data.data(), child_ptr, n);
        t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch; ++i){
            node->insert(insert_data[i], insert_node_ptr[i]);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("insert use " << delta << "ms, OPS=" << OPS);

    return true;

}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_inner_node_erase_perf(vector<key_type> &data, size_t n, size_t batch){
    AEX_HINT("[test data node erase performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    AEX_ASSERT(n > batch);
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;
    size_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * traits::DATA_NODE_FULL_RATIO < n) slot_size <<= 1;
    inner_node_ptr node = tree.node_allocator.allocate_inner_node(slot_size);
    AEX_PRINT("node slot size=%lld" << node->slot_size);
    AEX_PRINT("prepare dataset...");
    node_ptr* child_ptr = tree.node_allocator.allocate_nodeptr_buffer(n);

    for (size_type i = 0; i < n; ++i){
        child_ptr[i] = tree.node_allocator.allocate_data_node(1);
        data_node_ptr now_node = static_cast<data_node_ptr>(child_ptr[i]);
        now_node->key[0] = data[i];
        now_node->size = 1;
    }

    vector<size_type> del_pos(batch);
    vector<node_ptr> del_node(batch);

    //generate_query(pack_data, query, answer, batch);
    //generate_unique_dataset<size_type, uniform_int_distribution, >(child_ptr, , 0, n - 1);
    generate_unique_dataset<size_type, std::uniform_int_distribution<size_type>, size_type>(del_pos, batch, 1, n - 1);
    for (size_t i = 0; i < batch; ++i)
        del_node[i] = child_ptr[del_pos[i]];
    
    node->construct(data.data(), child_ptr, n);
    AEX_PRINT("is ml node?(0 or 1): " << (((node->prop) & aex::node_property::ML_NODE) > 0));
    AEX_PRINT("construct finish.");
    for (size_t i = 0; i < batch; ++i){
        bool flag = node->erase(del_node[i]);
        if (!flag){
            AEX_PRINT("Erase error! erase key=" << static_cast<data_node_ptr>(del_node[i])->key[0]);
            return false;
        }
    }
    
    if (static_cast<size_t>(node->size) != n - batch){
        AEX_PRINT("Erase error! node size=" << node->size << ", real size=" << n - batch);
    }
    std::sort(del_pos.begin(), del_pos.end());
    vector<key_type> left_key;
    vector<node_ptr> left_node;
    for (size_t i = 0, j = 0; i < n; ++i){
        if (del_pos[j] == i){
            ++j;
        }
        else{
            left_key.push_back(data[i]);
            left_node.push_back(child_ptr[i]);
        }
    }

    size_type bit_cnt = 0;
    for (pos_type i = 0; i < node->slot_size; ++i){
        if (i > 1 && node->key_ptr[i] < node->key_ptr[i - 1]){
            AEX_PRINT("Key Error! slot[" << i << "]=" << node->key_ptr[i] << ", slot[" << i - 1 << "]=" << node->key_ptr[i - 1]);
            return false;
        }
        if (i < node->slot_size && bitmap_impl::at(node->bitmap_ptr, i)){
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
        if (bitmap_impl::at(node->bitmap_ptr, i)){
            if (left_key[bit_cnt] != node->key_ptr[i]){
                AEX_ERROR("Key Error, node key[" << i << "]=" << node->key_ptr[i] << ", real key=" << left_key[bit_cnt]);
                return false;
            }
        }
        bit_cnt += ((bitmap_impl::at(node->bitmap_ptr, i)) != 0);
    }

    const int ITER = 10;
    system_clock::time_point t1, t2;
    double delta = 0;
    size_type sum;
    for (int T = 0; T < ITER; ++T){
        sum = 0;
        node->construct(data.data(), child_ptr, n);
        t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch; ++i)
            sum += node->erase(del_node[i]);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("erase use " << delta << "ms, OPS=" << OPS);
    return true;
    
}

// test inner node find(key)
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_query_perf(vector<key_type> &data, size_t n, size_t batch){
    AEX_HINT("[test data node insertion performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;
    pos_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * traits::DATA_NODE_FULL_RATIO < n) slot_size <<= 1;
    inner_node_ptr node = tree.node_allocator.allocate_inner_node(slot_size);
    AEX_PRINT("node slot size" << node->slot_size);
    AEX_PRINT("prepare dataset...");
    node_ptr* child_ptr = tree.node_allocator.allocate_nodeptr_buffer(n);

    for (size_type i = 0; i < n; ++i){
        child_ptr[i] = tree.node_allocator.allocate_data_node(1);
        data_node_ptr now_node = static_cast<data_node_ptr>(child_ptr[i]);
        now_node->key[0] = data[i];
        now_node->size = 1;
    }

    vector<std::pair<key_type, node_ptr> > pack_data(n);
    vector<key_type> query;
    vector<node_ptr> answer;
    for (size_t i = 0; i < n; ++i)
        pack_data[i] = make_pair(data[i], child_ptr[i]);

    generate_query(pack_data, query, answer, batch);
    for (size_t i = 0; i < batch; ++i){
        pos_type pos = std::lower_bound(data.data(), data.data() + n, query[i]) - data.data();
        if (static_cast<size_type>(pos) > 0)
            query[i] -= (1.0 * (rand() % 65536) / 65536) * (data[pos] - data[pos - 1]);
    }
    
    node->construct(data.data(), child_ptr, n);
    AEX_PRINT("is ml node?(0 or 1): " << (((node->prop) & aex::node_property::ML_NODE) > 0));
    AEX_PRINT("construct finish.");
    for (size_t i = 0; i < batch; ++i){
        pos_type pos = node->find(query[i]);
        if (node->child_ptr[pos] != answer[i]){
            AEX_ERROR("Query Error! query=" << query[i] << ", get pos=" << pos << ", get child=" << node->child_ptr[pos] << ", real child=" << answer[i]);
            return false;
        }
    }

    const int ITER = 10;
    system_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    value_type sum;
    for (int T = 0; T < ITER; ++T){
        sum = 0;
        for (size_t i = 0; i < batch; ++i)
            sum += node->find(query[i]);
    }
    t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("code=" << sum << "query use " << delta << "ms, QPS=" << QPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_other(vector<key_type> &data, size_t n, size_t batch){
    return false;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node(vector<key_type> &data, size_t n){
    return false;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node(std::pair<key_type, value_type>* data, size_t n){
    return false;
}


template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_insert_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    AEX_PRINT("[test data node insertion performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;

    pos_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * traits::DATA_NODE_FULL_RATIO < n) slot_size <<= 1;
    data_node_ptr node = tree.node_allocator.allocate_data_node(slot_size);
    if (static_cast<size_t>(node->slot_size) < n) {
        AEX_PRINT("node slot size is few!");
        return false;
    }
    AEX_PRINT("node slot size=" << node->slot_size);
    vector<std::pair<key_type, value_type>> insert_data(batch);
    printf("prepare dataset...\n");
    vector<std::pair<key_type, value_type> > node_data(n - batch);
    random_shuffle(data, data + n);
    std::copy(data, data + n - batch, node_data.data());
    std::copy(data + n - batch, data + n, insert_data.data());
    std::sort(data, data + n);
    
    n -= batch;
    for (size_type i = 1; i < n; ++i)
        if (node_data[0] > node_data[i]) 
            std::swap(node_data[0], node_data[i]);
            
    for (size_type i = 0; i < batch; ++i)
        if (node_data[0] > insert_data[i]) 
            std::swap(node_data[0], insert_data[i]);
    std::sort(node_data.data(), node_data.data() + n);

    for (size_type i = 0; i < n; ++i)
        std::cout << node_data[i].first << " ";
    std::cout << std::endl;
    node->construct(node_data.data(), n);

    printf("construct finish.\n");

    AEX_PRINT("is ml node?(0 or 1): " << (((node->prop) & aex::node_property::ML_NODE) > 0));
    if (!(node->prop & aex::node_property::ML_NODE)) return false;
    for (size_t i = 0; i < batch; ++i){       
        if (tree.isfull(node)){
            AEX_PRINT("node slot full");
            return false;
        }

        node->insert(insert_data[i].first, insert_data[i].second);
    }

    for (pos_type i = 0; i < node->size; ++i){
        if (node->key[i] != data[i].first){
            AEX_PRINT("Key Error! key[" << i << "]="<< node->key[i] << ", real key=" << data[i].first);
            return false;
        }
        if (i > 1 && node->key[i] < node->key[i - 1]){
            AEX_PRINT("Key Error! slot[" << i << "]=" << node->key[i] << ", slot[" << i - 1 << "]=" << node->key[i - 1]);
            return false;
        }
    }

    printf("test insert data node performance");
    system_clock::time_point t1, t2;
    const int ITER = 1000;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        node->construct(node_data.data(), n);
        t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch; ++i){
            node->insert(insert_data[i].first, insert_data[i].second);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    AEX_SUCCESS("query used time=" << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_query_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    AEX_HINT("[test data node query performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename traits::pos_type pos_type;
    pos_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * traits::DATA_NODE_FULL_RATIO < n) slot_size <<= 1;
    data_node_ptr node = tree.node_allocator.allocate_data_node(slot_size);

    if (static_cast<size_type>(node->slot_size) < n) {
        printf("node slot size is few!\n");
        return false;
    }
    AEX_PRINT("node slot size=" << node->slot_size);
    vector<key_type> ori_data(n);
    for (size_t i = 0; i < n; ++i)
        ori_data[i] = data[i].first;

    vector<key_type> query;
    vector<value_type> answer;
    generate_query<key_type, value_type>(data, n, query, answer, batch);
    
    std::sort(data, data + n);

    AEX_PRINT("node slot size=" << node->slot_size);

    node->construct(data, n);

    AEX_PRINT("construct finish.\n");
    AEX_PRINT("is ml node?(0 or 1): " << (((node->prop) & aex::node_property::ML_NODE) > 0));
    if (!(node->prop & aex::node_property::ML_NODE)) return false;
    for (size_t i = 0; i < batch; ++i){
        pos_type pos = std::lower_bound(ori_data.data(), ori_data.data() + n, query[i]) - ori_data.data();
        if (pos > 0)
            query[i] -= (1.0 * (rand() % 65536) / 65536) * (ori_data[pos] - ori_data[pos - 1]);
    }

    for (size_t i = 0; i < batch; ++i){       
        [[maybe_unused]] pos_type pos = node->find_lower_pos(query[i]);
        if (answer[i] != node->data[pos]){
            AEX_PRINT("answer wrong! key=" << query[i] << ", answer=" << answer[i] << ", data=" << node->data[i]);
        }
    }

    AEX_PRINT("test query data node performance");
    system_clock::time_point t1, t2;
    value_type sum = 0;
    const int ITER = 1000;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch; ++i){
            sum += node->find_lower_pos(query[i]);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    //printf("msed time=%lld us\n", delta);
    double QPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("query used time=" << delta << " ms, QPS=" << QPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_erase_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    AEX_WARNING("no implement");
    return false;
}
