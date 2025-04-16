#pragma once

//test hash node construction and insertion
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_hash_node_insert_perf(vector<key_type> &data, size_t n, size_t batch){
    AEX_PRINT("[test data node insertion performance]");
    aex_tree<key_type, value_type, traits> tree;
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]]typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::hash_node_ptr hash_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;
    //tree.hash_table.rescale(min_slot_size(n * 2, traits::HASH_TABLE_FULL_RATIO, traits::MIN_HASH_TABLE_SIZE));
    vector<key_type> node_data(n);
    std::copy(data.begin(), data.end(), node_data.begin());
    vector<key_type> insert_data(batch);
    AEX_PRINT("prepare dataset...");
    split_dataset(node_data, insert_data, batch);
    AEX_PRINT("prepare dataset target 0");
    n -= batch;
    //for (size_t i = 1; i < n; ++i)
    //    if (node_data[0] > node_data[i]) 
    //        std::swap(node_data[0], node_data[i]);
    //        
    //for (size_t i = 0; i < batch; ++i)
    //    if (node_data[0] > insert_data[i]) 
    //        std::swap(node_data[0], insert_data[i]);

    std::sort(node_data.begin(), node_data.end());
    data_node_ptr* child_ptr = new data_node_ptr[n];
    data_node_ptr* insert_node_ptr = new data_node_ptr[n];
    construct_data_node_array<key_type, value_type, traits>(node_data.data(), node_data.size(), child_ptr);
    construct_data_node_array<key_type, value_type, traits>(insert_data.data(), insert_data.size(), insert_node_ptr);
    hash_node_ptr node = static_cast<hash_node_ptr>(tree.construct(node_data.data(), reinterpret_cast<node_ptr*>(child_ptr), n));
    if (node->type != NodeType::HashNode){
        AEX_ERROR("Node type is not hash node");
        return false;
    }

    size_t insert_failed = 0;

    for (size_t i = 0; i < batch; ++i){       
        if (tree.isfull(node)){
            AEX_ERROR("node slot full, slot size=" << node->slot_size << ", size=" << node->size);
            insert_failed += batch - i + 1;
            break;
        }
        slot_type pos = node->predict(insert_data[i]);
        if (!node->is_occupied(pos))
            tree.insert_no_collision(node, pos, insert_data[i], insert_node_ptr[i]);
        else
            ++insert_failed;
    }
    AEX_HINT("node size=" << node->size << ", slot_size=" << node->slot_size << ", insert failed=" << insert_failed);
    AEX_SUCCESS("test insert node performance");
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_hash_node_erase_perf(vector<key_type> &data, size_t n, size_t batch){
        AEX_PRINT("[test data node insertion performance]");
    aex_tree<key_type, value_type, traits> tree;
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::hash_node_ptr hash_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;
    //tree.hash_table.rescale(min_slot_size(n * 2, traits::MIN_HASH_TABLE_SIZE));
    
    AEX_PRINT("prepare dataset...");
    data_node_ptr* child_ptr = new data_node_ptr[n];
    construct_data_node_array<key_type, value_type, traits>(data.data(), data.size(), child_ptr);

    vector<size_t> del_pos;
    vector<key_type> del_key(batch);
    vector<node_ptr> del_node(batch);

    generate_unique_dataset<size_t, std::uniform_int_distribution<size_t>, size_t>(del_pos, batch, 0, n - 2);
    for (size_t i = 0; i < batch; ++i){
        del_key[i] = data[del_pos[i]];
        del_node[i] = child_ptr[del_pos[i]];
    }

    construct_data_node_array<key_type, value_type, traits>(data.data(), data.size(), child_ptr);
    hash_node_ptr node = static_cast<hash_node_ptr>(tree.construct(data.data(), reinterpret_cast<node_ptr*>(child_ptr), n));
    if (node->type != aex::NodeType::HashNode){
        AEX_ERROR("Node type is not hash node");
        return false;
    }

    size_t insert_failed = 0;

    for (size_t i = 0; i < batch; ++i){       
        if (tree.isfew(node)){
            AEX_ERROR("node slot few, slot size=" << node->slot_size << ", size=" << node->size);
            insert_failed += batch - i + 1;
            break;
        }
        slot_type pos = node->predict(del_key[i]);
        slot_type next_pos = node->next_item(pos + 1);
        if (pos != 0 && !node->is_occupied(pos))
            tree.erase(node, pos, next_pos, del_node[i]);
        else
            ++insert_failed;
    }
    AEX_HINT("node size=" << node->size << ", insert failed=" << insert_failed);
    AEX_SUCCESS("test erase node performance");
    return true;
}

// test inner node find(key)
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_hash_node_query_perf(vector<key_type> &data, size_t n, size_t batch){
    
    AEX_PRINT("[test data node query performance]");
    aex_tree<key_type, value_type, traits> tree;
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]]typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::hash_node_ptr hash_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;
    AEX_PRINT("1");
    //tree.hash_table.rescale(min_slot_size(n * 2, traits::HASH_TABLE_FEW_RATIO, traits::MIN_HASH_TABLE_SIZE));
    AEX_PRINT("2");
    data_node_ptr* child_ptr = new data_node_ptr[n];
    AEX_PRINT("3");
    construct_data_node_array<key_type, value_type, traits>(data.data(), data.size(), child_ptr);
    vector<std::pair<key_type, data_node_ptr> > pack_data(n);
    vector<key_type> query;
    vector<data_node_ptr> answer;
    node_ptr child;
    for (size_t i = 0; i < n; ++i)
        pack_data[i] = std::make_pair(data[i], child_ptr[i]);
    AEX_PRINT("4");
    generate_query(pack_data, query, answer, batch);
    for (size_t i = 0; i < batch; ++i){
        slot_type pos = std::lower_bound(data.data(), data.data() + n, query[i]) - data.data();
        if (static_cast<size_t>(pos) < n - 1 && !std::is_integral<key_type>::value){
            query[i] += (1.0 * (rand() % 65536) / 65536) * (data[pos + 1] - data[pos]);
        }
    }
    AEX_PRINT("5");
    hash_node_ptr node = static_cast<hash_node_ptr>(tree.construct(data.data(), reinterpret_cast<node_ptr*>(child_ptr), n));
    if (node->type != NodeType::HashNode){
        AEX_ERROR("Node type is not hash node");
        return false;
    }
    AEX_PRINT("6");
    for (size_t i = 0; i < batch; ++i){       
        [[maybe_unused]]key_type key;
        node_ptr child;
        child = tree.find(node, query[i]);
        if (child->type == aex::NodeType::LeafNode && child != answer[i]){
            AEX_ERROR("Query Error! query=" << query[i] << ", get=" << static_cast<data_node_ptr>(child)->key[0] << ", real key=" << answer[i]->key[0]);
            return false;
        }
    }

    AEX_HINT("node size=" << node->size << ", node slot size=" << node->slot_size);

    const int ITER = 10000;
    system_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    value_type sum;
    for (int T = 0; T < ITER; ++T){
        sum = 0;
        for (size_t i = 0; i < batch; ++i){
            child = tree.find(node, query[i]);
            sum += child->size;
        }
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
    aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;

    vector<std::pair<key_type, value_type>> insert_data(batch);
    printf("prepare dataset...\n");
    vector<std::pair<key_type, value_type> > node_data(n - batch);
    random_shuffle(data, data + n);
    std::copy(data, data + n - batch, node_data.data());
    std::copy(data + n - batch, data + n, insert_data.data());
    std::sort(data, data + n);
    for (size_t i = 0; i < n; ++i)
        std::cout << data[i].first << " ";

    std::cout << std::endl;    
    n -= batch;
    for (size_t i = 1; i < n; ++i)
        if (node_data[0] < node_data[i]) 
            std::swap(node_data[0], node_data[i]);
            
    for (size_t i = 0; i < batch; ++i)
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
            AEX_PRINT("Key Error! key[" << i << "]=" << node->key[i] << ", real key=" << data[i].first);
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
        aex_tree<key_type, value_type, traits> tree;
        typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
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
        aex_tree<key_type, value_type, traits> tree;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
        typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        typedef typename traits::slot_type slot_type;

        std::vector<size_t> del_pos;
        std::vector<key_type> del_key(batch);
        generate_unique_dataset<size_t, std::uniform_int_distribution<size_t>, size_t>(del_pos, batch, 0, n - 2);

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
                left_key.emplace_back(data[i].first);
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
