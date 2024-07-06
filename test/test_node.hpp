#pragma once

//test inner node construction and insertion
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_insert_perf(vector<key_type> &data, size_t n, size_t batch, int level){
    
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
    AEX_PRINT("prepare dataset...");
    split_dataset(node_data, insert_data, batch);
    AEX_PRINT("prepare dataset target 0");
    n -= batch;
    for (size_type i = 1; i < n; ++i)
        if (node_data[0] < node_data[i]) 
            std::swap(node_data[0], node_data[i]);
            
    for (size_type i = 0; i < batch; ++i)
        if (node_data[0] < insert_data[i]) 
            std::swap(node_data[0], insert_data[i]);

    std::sort(node_data.begin(), node_data.end());
    node_ptr* child_ptr = new node_ptr[n];
    node_ptr* insert_node_ptr = new node_ptr[n];
    construct_data_node_array<key_type, value_type, node_ptr>(node_data.data(), node_data.size(), child_ptr);
    construct_data_node_array<key_type, value_type, node_ptr>(insert_data.data(), insert_data.size(), insert_node_ptr);
    std::vector<key_type> key_buf;
    std::vector<inner_node_ptr> child_buf;
    tree.split(node_data.data(), child_ptr, n, level, key_buf, child_buf);

    if (child_buf.size() > 1){
        AEX_ERROR("can't be construct in a inner node. nodes=" << child_buf.size());
        for (size_t i = 0; i < child_buf.size(); ++i)
            AEX_PRINT("slot size=" << static_cast<inner_node_ptr>(child_buf[i])->slot_size << ", size=" << child_buf[i]->size);
        return false;
    }
    if (IS_ML_NODE(child_buf[0]) == false)
        AEX_WARNING("node is not ml node");
    
    inner_node_ptr node = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), child_buf[0]->level, IS_ML_NODE(child_buf[0]));
    *node = *static_cast<inner_node_ptr>(child_buf[0]);
    AEX_PRINT("construct finish. node slot size=" << node->slot_size << ", size=" << node->size);

    size_type insert_failed = 0;
    vector<key_type> final_node_data(n);
    std::copy(node_data.data(), node_data.data() + n, final_node_data.data());

    for (size_t i = 0; i < batch; ++i){       
        if (tree.isfull(node)){
            AEX_ERROR("node slot full, slot size=" << node->slot_size << ", size=" << node->size << "full ratio=" << tree.inner_node_full_ratio[level]);
            insert_failed += batch - i;
            break;
        }
        if (node->insert(insert_data[i], insert_node_ptr[i]) != aex::NODE_INSERT_CODE::SUCCESS){
            ++insert_failed;
            AEX_PRINT("insert failed!");
        }
        else{
            if (node->find(insert_data[i]) != insert_node_ptr[i]){
                AEX_ASSERT("no find insert item");
                return false;
            }
            final_node_data.push_back(insert_data[i]);
        }
    }
    std::sort(final_node_data.begin(), final_node_data.end());
    //for (auto &x : node_data)
    //    std::cout << x << " ";
    //std::cout << std::endl;
    //for (auto &x : insert_data)
    //    std::cout << x << " ";
    //std::cout << std::endl;
    //for (slot_type i = 0; i < node->slot_size; ++i){
    //    std::cout << "(" << node->key_ptr[i] << ", " << node->child_ptr[i] << "), ";
    //}
    //std::cout << std::endl;
    std::vector<key_type> test_key_buf(node->size);
    std::vector<node_ptr> test_child_buf(node->size);
    tree.copy_to_buffer(node, test_key_buf.data(), test_child_buf.data());
    for (int i = 0; i < node->size - 1; ++i){
        if (test_key_buf[i] != final_node_data[i]){
            AEX_ERROR("i=" << i << ", node key=" << key_buf[i] << ", final_node_data=" << final_node_data[i]);
            return false;
        }
    }

    AEX_ASSERT(final_node_data.size() == n + batch - insert_failed);
    printf("insert failed=%lld, fail ratio=%.4f\n", insert_failed, 1.0 * insert_failed / batch);
    size_type bit_cnt = 0;
    if (IS_ML_NODE(node)){
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
            if (bitmap_impl::at(node->bitmap_ptr, i)){
                if (node->key_ptr[i] != final_node_data[bit_cnt]){
                    AEX_ERROR("Key error, node key[" << i<< "]=" << node->key_ptr[i] << ", real key=" << final_node_data[bit_cnt]);
                    return false;
                }
                bit_cnt++;
                std::pair<key_type, node_ptr> res;
                do{
                    res = node->hash_table.pop(i);
                    if (res.second != nullptr){
                        if (res.first != final_node_data[bit_cnt]){
                            AEX_ERROR("Key error, node key[" << i<< "]=" << res.first << ", real key=" << final_node_data[bit_cnt]);
                            return false;
                        }
                        bit_cnt++;
                    }
                }while (res.second != nullptr);
            }
            //bit_cnt += ((bitmap_impl::at(node->bitmap_ptr, i)) != 0);
        }
        if (bit_cnt + 1 != n + batch - insert_failed){
            AEX_ERROR("bit one cnt not equal items, 1 bits=" << bit_cnt << " n=" << n + batch - insert_failed);
            return false;
        }
    }

    AEX_SUCCESS("test insert node performance");
    system_clock::time_point t1, t2;
    const int ITER = 1000;
    double delta = 0;
    vector<inner_node_ptr> node_array(ITER);
    for (int i = 0; i < ITER; ++i){
        node_array[i] = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), child_buf[0]->level, IS_ML_NODE(child_buf[0]));
        *node_array[i] = *static_cast<inner_node_ptr>(child_buf[0]);
    }
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (size_t i = 0; i < batch; ++i){
            node_array[T]->insert(insert_data[i], insert_node_ptr[i]);
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("insert use " << delta << "ms, OPS=" << OPS);
    //tree.allocator.deallocate(child_ptr);
    AEX_PRINT("?");
    //tree.allocator.deallocate(insert_node_ptr);
    AEX_PRINT("??");
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_inner_node_erase_perf(vector<key_type> &data, size_t n, size_t batch, int level){
    
    AEX_HINT("[test data node erase performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    AEX_ASSERT(n > batch);
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;
    AEX_PRINT("prepare dataset...");
    node_ptr* child_ptr = new node_ptr[n];
    construct_data_node_array<key_type, value_type, node_ptr>(data.data(), data.size(), child_ptr);

    vector<size_type> del_pos;
    vector<node_ptr> del_node(batch);

    generate_unique_dataset<size_type, std::uniform_int_distribution<size_type>, size_type>(del_pos, batch, 0, n - 2);
    for (size_t i = 0; i < batch; ++i)
        del_node[i] = child_ptr[del_pos[i]];
    
    std::vector<key_type> key_buf;
    std::vector<inner_node_ptr> child_buf;
    tree.split(data.data(), child_ptr, n, level, key_buf, child_buf);
    AEX_PRINT("cosntruct finish.");

    if (child_buf.size() > 1){
        AEX_PRINT("can't be construct in a inner node.");
        return false;
    }
    
    if (IS_ML_NODE(child_buf[0]) == false){
        AEX_ERROR("node is not ml node");
    }
    
    inner_node_ptr node = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), child_buf[0]->level, IS_ML_NODE(child_buf[0]));
    *node = *static_cast<inner_node_ptr>(child_buf[0]);
    AEX_PRINT("size=" << node->size << ", slot size=" << node->slot_size);
    for (int i = 0; i < node->size; ++i)
        AEX_PRINT("key=" << node->key_ptr[i] << ", child=" << node->child_ptr[i]);
    
    for (size_t i = 0; i < batch; ++i){
        //AEX_PRINT("i=" << i << "del_node=" << del_node[i]);
        node->erase(del_node[i]);
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

    slot_type bit_cnt = 0;
    if (IS_ML_NODE(node)){
        for (slot_type i = 0; i < node->slot_size; ++i){
            if (i > 1 && node->key_ptr[i] < node->key_ptr[i - 1]){
                AEX_ERROR("Key Error! slot[" << i << "]=" << node->key_ptr[i] << ", slot[" << i - 1 << "]=" << node->key_ptr[i - 1]);
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
        if (bit_cnt + 1 != node->size){
            AEX_ERROR("bitmap error, bit cnt=" << bit_cnt << ", node->size=" << node->size);
        }
    }
    else{
        for (slot_type i = 0; i < node->size; ++i){
            if (i > 1 && node->key_ptr[i] < node->key_ptr[i - 1]){
                AEX_ERROR("Key Error! slot[" << i << "]=" << node->key_ptr[i] << ", slot[" << i - 1 << "]=" << node->key_ptr[i - 1]);
                return false;
            }
            if (left_key[i] != node->key_ptr[i]){
                AEX_ERROR("Key Error, node key[" << i << "]=" << node->key_ptr[i] << ", real key=" << left_key[bit_cnt]);
                return false;
            }
        }   
    }

    AEX_SUCCESS("Test inner node erase success. Next test erase performance");
    const int ITER = 1000;
    system_clock::time_point t1, t2;
    double delta = 0;
    //size_type sum;
    std::vector<inner_node_ptr> node_array(ITER);
    for (int i = 0; i < ITER; ++i){
        node_array[i] = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), child_buf[0]->level, IS_ML_NODE(child_buf[0]));
        *node_array[i] = *static_cast<inner_node_ptr>(child_buf[0]);
    }
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (size_t i = 0; i < batch; ++i)
            node_array[T]->erase(del_node[i]);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();

    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("erase use " << delta << "ms, OPS=" << OPS);
    //tree.allocator.deallocate(child_ptr);
    return true;
}

// test inner node find(key)
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_query_perf(vector<key_type> &data, size_t n, size_t batch, int level){
    
    AEX_HINT("[test data node query performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;

    AEX_PRINT("prepare dataset...");
    node_ptr* child_ptr = new node_ptr[n];
    construct_data_node_array<key_type, value_type, node_ptr>(data.data(), data.size(), child_ptr);

    vector<std::pair<key_type, node_ptr> > pack_data(n);
    vector<key_type> query;
    vector<node_ptr> answer;
    for (size_t i = 0; i < n; ++i)
        pack_data[i] = std::make_pair(data[i], child_ptr[i]);

    generate_query(pack_data, query, answer, batch);
    for (size_t i = 0; i < batch; ++i){
        slot_type pos = std::lower_bound(data.data(), data.data() + n, query[i]) - data.data();
        if (static_cast<size_type>(pos) > 0 && !std::is_integral<key_type>::value){
            query[i] -= (1.0 * (rand() % 65536) / 65536) * (data[pos] - data[pos - 1]);
        }
    }
    
    std::vector<key_type> key_buf;
    std::vector<inner_node_ptr> child_buf;
    tree.split(data.data(), child_ptr, n, level, key_buf, child_buf);
    AEX_PRINT("construct finish. ");

    if (child_buf.size() > 1){
        AEX_PRINT("can't be construct in a inner node.");
        return false;
    }
    
    AEX_IMPORTANT("IS_ML_NODE?:" << (aex::IS_ML_NODE(child_buf[0]) ? 'Y' : 'N'));
    
    inner_node_ptr node = tree.allocator.allocate_inner_node(static_cast<inner_node_ptr>(child_buf[0])->real_slot_size(), child_buf[0]->level, IS_ML_NODE(child_buf[0]));
    *node = *static_cast<inner_node_ptr>(child_buf[0]);
    AEX_PRINT("node->slot size=" << node->slot_size << ", size=" << node->size);
    for (size_t i = 0; i < batch; ++i){
        data_node_ptr res = static_cast<data_node_ptr>(node->find(query[i]));
        if (res != answer[i]){
            AEX_ERROR("Query Error! query=" << query[i] << ", get=" << res->key[0] << ", get child=" << node->find(query[i]) << ", real child=" << answer[i]);
            return false;
        }
    }

    const int ITER = 10000;
    system_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    value_type sum;
    for (int T = 0; T < ITER; ++T){
        sum = 0;
        for (size_t i = 0; i < batch; ++i)
            sum += static_cast<data_node_ptr>(node->find(query[i]))->key[0];
    }
    t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    AEX_SUCCESS("code=" << sum << ", query use " << delta << "ms, QPS=" << QPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_inner_node_other(vector<key_type> &data, size_t n, size_t batch, int level){
    return false;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_insert_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    
    if constexpr (!traits::AllowDynamicDataNode){
        AEX_ERROR("index not allow dynamic data node!");
        return false;
    }
    else{
    AEX_PRINT("[test data node insertion performance]");
    mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename traits::slot_type slot_type;

    vector<std::pair<key_type, value_type>> insert_data(batch);
    printf("prepare dataset...\n");
    vector<std::pair<key_type, value_type> > node_data(n - batch);
    random_shuffle(data, data + n);
    std::copy(data, data + n - batch, node_data.data());
    std::copy(data + n - batch, data + n, insert_data.data());
    std::sort(data, data + n);
    for (size_type i = 0; i < n; ++i)
        std::cout << data[i].first << " ";

    std::cout << std::endl;    
    n -= batch;
    for (size_type i = 1; i < n; ++i)
        if (node_data[0] < node_data[i]) 
            std::swap(node_data[0], node_data[i]);
            
    for (size_type i = 0; i < batch; ++i)
        if (node_data[0] < insert_data[i]) 
            std::swap(node_data[0], insert_data[i]);
    std::sort(node_data.data(), node_data.data() + n);

    std::vector<key_type> key_buf, node_key(n);
    std::vector<data_node_ptr> child_buf;
    std::vector<value_type> node_value(n);

    for (size_t i = 0; i < n; ++i){
        node_key[i] = node_data[i].first;
        node_value[i] = node_data[i].second;
    }
    
    tree.split_with_linear_probe(node_key.data(), node_value.data(), n, key_buf, child_buf);
    AEX_PRINT("construct finish. data node size=" << child_buf.size());
    if (child_buf.size() > 1){
        AEX_ERROR("can't be construct in a data node, data node nums=" << child_buf.size());
        return false;
    }
    data_node_ptr node = tree.allocator.allocate_data_node(child_buf[0]->slot_size, true);
    *node = *static_cast<data_node_ptr>(child_buf[0]);
    for (size_t i = 0; i < batch; ++i){       
        if (tree.isfull(node)){
            AEX_ERROR("node slot full");
            return false;
        }
        node->insert(insert_data[i].first, insert_data[i].second);
    }   

    if (static_cast<size_t>(node->size) != n + batch){
        AEX_ERROR("Size error! node size=" << node->size << ", true size=" << n + batch);
        return false;
    }

    for (slot_type i = 0; i < node->size; ++i){
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
    std::vector<data_node_ptr> node_array(ITER);
    for (int i = 0; i < ITER; ++i){
        *node_array[i] = *static_cast<data_node_ptr>(child_buf[0]);
    }
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (size_t i = 0; i < batch; ++i){
            node_array[i]->insert(insert_data[i].first, insert_data[i].second);
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta += duration_cast<microseconds>(t2 - t1).count();
    std::cout << std::scientific;
    std::cout << std::setprecision(3);   
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    AEX_SUCCESS("query used time=" << delta << "ms, OPS=" << OPS);
    return true;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_query_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    
    if constexpr (!traits::AllowDynamicDataNode){
        AEX_ERROR("index not allow dynamic data node!");
        return false;
    }
    else{
        AEX_HINT("[test data node query performance]");
        mock_aex_tree<key_type, value_type, traits> tree;
        typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
        [[maybe_unused]] typedef typename traits::size_type size_type;
        typedef typename traits::slot_type slot_type;

        vector<key_type> ori_data(n);
        for (size_t i = 0; i < n; ++i)
            ori_data[i] = data[i].first;

        vector<key_type> query;
        vector<value_type> answer;
        generate_query<key_type, value_type>(data, n, query, answer, batch);
        
        std::sort(data, data + n);

        std::vector<key_type> key_buf, node_key(n);
        std::vector<data_node_ptr> child_buf;
        std::vector<value_type> node_value(n);
        for (size_t i = 0; i < n; ++i){
            node_key[i] = data[i].first;
            node_value[i] = data[i].second;
        }
        tree.split_with_linear_probe(node_key.data(), node_value.data(), n, key_buf, child_buf);
        AEX_PRINT("construct finish. data node size=" << child_buf.size() );
        if (child_buf.size() > 1){
            AEX_PRINT("can't be construct in a data node.");
            return false;
        }
        data_node_ptr node = tree.allocator.allocate_data_node(child_buf[0]->slot_size, true);
        *node = *child_buf[0];
        AEX_PRINT("node slot size=" << node->slot_size);

        AEX_PRINT("is ml node?(0 or 1): " << IS_ML_NODE(node));

        if (!IS_ML_NODE(node)) return false;
        for (size_t i = 0; i < batch; ++i){
            slot_type pos = std::lower_bound(ori_data.data(), ori_data.data() + n, query[i]) - ori_data.data();
            if (pos > 0)
                query[i] -= (1.0 * (rand() % 65536) / 65536) * (ori_data[pos] - ori_data[pos - 1]);
        }

        for (size_t i = 0; i < batch; ++i){       
            [[maybe_unused]] slot_type pos = node->find_lower_pos(query[i]);
            if (answer[i] != node->data[pos]){
                AEX_PRINT("answer wrong! key=" << query[i] << ", answer=" << answer[i] << ", data=" << node->data[i]);
            }
        }

        AEX_PRINT("test query data node performance");
        system_clock::time_point t1, t2;
        value_type sum = 0;
        const int ITER = 1000;
        double delta = 0;
        t1 = std::chrono::high_resolution_clock::now();
        for (int T = 0; T < ITER; ++T){
            for (size_t i = 0; i < batch; ++i){
                sum += node->find_lower_pos(query[i]);
            }
            delta += duration_cast<microseconds>(t2 - t1).count();
        }
        t2 = std::chrono::high_resolution_clock::now();

        //printf("msed time=%lld us\n", delta);
        double QPS = 1.0 * 1e6 * ITER * batch / delta;
        std::cout << std::scientific;
        std::cout << std::setprecision(3);   
        AEX_SUCCESS("query used time=" << delta << " ms, QPS=" << QPS);
        return true;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_data_node_erase_perf(std::pair<key_type, value_type>* data, size_t n, size_t batch){
    
    AEX_PRINT("[test data node erase performance]");
    if constexpr (!traits::AllowDynamicDataNode){
        AEX_ERROR("index not allow dynamic data node!");
        return false;
    }
    else{
        mock_aex_tree<key_type, value_type, traits> tree;
        [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
        [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
        typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        [[maybe_unused]] typedef typename traits::size_type size_type;
        typedef typename traits::slot_type slot_type;

        std::vector<size_type> del_pos;
        std::vector<key_type> del_key(batch);
        generate_unique_dataset<size_type, std::uniform_int_distribution<size_type>, size_type>(del_pos, batch, 0, n - 2);

        std::sort(del_pos.data(), del_pos.data() + batch);
        for (size_t i = 0; i < batch; ++i){
            AEX_PRINT("del pos=" << del_pos[i]);
            del_key[i] = data[del_pos[i]].first;
        }

        std::vector<key_type> key_buf, node_key(n);
        std::vector<data_node_ptr> child_buf;
        std::vector<value_type> node_value(n);
        for (size_t i = 0; i < n; ++i){
            node_key[i] = data[i].first;
            node_value[i] = data[i].second;
            std::cout << node_key[i] << " ";
        }
        std::cout << std::endl;

        tree.split_with_linear_probe(node_key.data(), node_value.data(), n, key_buf, child_buf);
        AEX_PRINT("construct finish. data node size=" << child_buf.size());
        if (child_buf.size() > 1){
            AEX_PRINT("can't be construct in a data node.");
            return false;
        }
        data_node_ptr node = tree.allocator.allocate_data_node(child_buf[0]->slot_size, true);
        *node = *child_buf[0];
        for (size_t i = 0; i < batch; ++i){       
            if (tree.isfew(node)){
                AEX_PRINT("node slot few");
            }
            slot_type pos = node->find_lower_pos(del_key[i]);
            node->erase(pos);
        }

        std::vector<key_type> left_key;
        for (size_t i = 0, j = 0; i < n; ++i){
            if (del_pos[j] == i)
                ++j;
            else{
                left_key.push_back(data[i].first);
            }
        }

        if (static_cast<size_t>(node->size) != n - batch){
            AEX_ERROR("key size=" << node->size << ", real size=" << n - batch);
        }

        for (slot_type i = 0; i < node->size; ++i){
            if (node->key[i] != left_key[i]){
                AEX_PRINT("Key Error! key[" << i << "]="<< node->key[i] << ", real key=" << left_key[i]);
                return false;
            }
            if (i > 1 && node->key[i] < node->key[i - 1]){
                AEX_PRINT("Key Error! slot[" << i << "]=" << node->key[i] << ", slot[" << i - 1 << "]=" << node->key[i - 1]);
                return false;
            }
        }

        AEX_SUCCESS("Test inner node erase success. Next test erase performance");

        AEX_HINT("test insert data node erase performance");
        system_clock::time_point t1, t2;
        const int ITER = 1000;
        double delta = 0;
        std::vector<data_node_ptr> node_array(ITER);
        for (int i  = 0; i < ITER; ++i){
            node_array[i] = tree.allocator.allocate_data_node(static_cast<data_node_ptr>(child_buf[0])->slot_size, IS_ML_NODE(child_buf[0]));
            *node_array[i] = *static_cast<data_node_ptr>(child_buf[0]);
        }
        t1 = std::chrono::high_resolution_clock::now();
        for (int T = 0; T < ITER; ++T){
            for (size_t i = 0; i < batch; ++i){
                slot_type pos = node_array[T]->find_lower_pos(del_key[i]);
                node_array[T]->erase(pos);
            }
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta = duration_cast<microseconds>(t2 - t1).count();

        std::cout << std::scientific;
        std::cout << std::setprecision(3);   
        double OPS = 1.0 * 1e6 * ITER * batch / delta;
        AEX_SUCCESS("query used time=" << delta << "ms, OPS=" << OPS);
        return true;
    }
}
