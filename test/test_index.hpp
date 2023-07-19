#pragma once

template<typename K, typename V>
bool test_index(std::pair<K, V>* data, size_t n){
    aex::aex_map<K , V> mp;
    random_shuffle(data, data + n);
    // insert
    for (size_t i = 0; i < n; ++i){
        #ifdef AEX_DEBUG
            if (false) mp.set_debug_level(1);
            else mp.set_debug_level(0);
        #endif 
        mp.insert(std::make_pair(data[i].first, data[i].second));
    }

    // find
    int M = std::min(n, (size_t)100);
    for (int i = 0; i < M; ++i){
        size_t x = rand() % n;
        auto y = mp.find(data[x].first);
        if (y.data() != data[x].second){
            printf("Error!");
        }
    }

    // erase
    random_shuffle(data, data + n);

    for (int i = 0; i < M; ++i){
        mp.erase(data[i].first);
    }

    //bulk load
    {
        std::sort(data, data + n);
        aex::aex_map<K, V> mp;
        mp.bulk_load(data, n);
        typename aex::aex_map<K, V>::stats st = mp.get_stats();
        //printf("inner node=%lld, data node=%lld size=%lld height=%lld", st.inner_node, st.data_node, st.size, st.height);
        AEX_PRINT("inner node num=" << st.inner_node << ", data node num=" << st.data_node << "size=" << st.size << "height=" << st.height);

        // find
        for (int i = 0; i < M; ++i){
            size_t x = rand() % n;
            auto y = mp.find(data[x].first);
            if (y.data() != data[x].second){
                printf("Error!");
            }
        }

        // erase
        random_shuffle(data, data + n);
        for (int i = 0; i < M; ++i){
            mp.erase(data[i].first);
        }
    }

    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_bulk_load_perf(std::pair<key_type, value_type>* data, long long n){
    AEX_HINT("[test index bulk load]");

    mock_aex_tree<key_type, value_type, traits> index;
    typedef typename mock_aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;
    std::sort(data, data + n);
    index.bulk_load(data, n);
    
    {
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr; inode = inode->next){
            //std::cout << "inode=" << inode << ", key[0]=" << static_cast<data_node_ptr>(inode)->key[0] << ;
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node);
            return false;
        }
        
        size_type i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    index.clear();
    index.print_stats();
    if (index.size() != 0){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 10;
    for (int T = 0; T < ITER; ++T){
        index.clear();
        t1 = std::chrono::high_resolution_clock::now();
        index.bulk_load(data, n);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    double OPS = 1.0 * 1e6 * ITER * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("bulk load use time " << delta << "ms, NPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_lookup_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    //typedef typename aex::aex_map<key_type, value_type, traits> Index;
    mock_aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    vector<key_type> query;
    vector<value_type> answer;
    generate_query(data, n, query, answer, batch);
    //AEX_PRINT("query[0]=" << query[0] << "answer[0]=" << answer[0]);
    std::sort(data, data + n);
    index.bulk_load(data, n);
    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    //for (int i = 0; i < index.head_leaf->size; ++i)
    //    AEX_PRINT("i=" << i << ", k=" << index.head_leaf->key[i] << ", v=" << index.head_leaf->data[i]);

    index.print_stats();
    for (int i = 0; i < batch; ++i){
        auto iter = index.find(query[i]);
        if (iter.data() != answer[i]){
            AEX_ERROR("Answer Error!, query key=" << query[i] << ", node key=" << iter.key() <<  ", answer=" << answer[i] << ", find=" << iter.data());
            return false;
        }
    }
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 10;
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (long long i = 0; i < batch; ++i){
            auto iter = index.find(query[i]);
            sum += iter.data();
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();

    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << "query use time " << delta << "ms, NPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_insert_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    std::vector<std::pair<key_type, value_type> > insert_data(batch); 
    std::random_shuffle(data, data + n);
    std::sort(data, data + n - batch);
    index.bulk_load(data, n - batch);
    for (long long i = n - batch; i < n; ++batch)
        index.insert(data[i]);
    {
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr; inode = inode->next){
            //std::cout << "inode=" << inode << ", key[0]=" << static_cast<data_node_ptr>(inode)->key[0] << ;
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node);
            return false;
        }
        
        size_type i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    index.clear();
    const int ITER = 10;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index.bulk_load(data, n - batch);
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = n - batch; i < n; ++i)
            index.insert(data[i]);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("insert use time " << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_erase_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index;
    typedef typename tree::node_ptr node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    std::vector<key_type> erase_key(batch);
    std::vector<std::pair<key_type, value_type> > left_data(n - batch);
    std::random_shuffle(data, data + n);
    std::copy(data + n - batch, data + n, erase_key.data());
    std::copy(data, data + n, left_data.data());
    std::sort(data, data + n);
    index.bulk_load(data, n);
    for (long long i = 0; i < batch; ++i)
        index.erase(erase_key[i]);

    {
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node);
            return false;
        }
        
        size_type i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (left_data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
                return false;
            }
            if (left_data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    index.clear();
    const int ITER = 10;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index.bulk_load(data, n);
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < batch; ++i)
            index.erase(erase_key[i]);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("erase use time " << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_index_range_query_perf(std::pair<key_type, value_type>* data, long long n, long long batch, double range_length){
    mock_aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;

    return false;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_rw_balance_traits<key_type, value_type>>
bool test_index_RW_perf(std::pair<key_type, value_type>* data, long long n, long long batch, double rw_ratio){
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    mock_aex_tree<key_type, value_type, traits> index;
    long long insert_num = static_cast<long long>(rw_ratio * batch);
    long long query_num = batch - insert_num;
    std::vector<std::pair<key_type, value_type> > insert_data(insert_num), bulk_load_data(n);
    std::vector<key_type> query(query_num);
    std::vector<value_type> answer(query_num), index_answer(query_num);
    std::random_shuffle(data, data + n);
    generate_query(data, n, query, answer, query_num);
    assert(n > insert_num);
    std::copy(data + n - insert_num, data + n, insert_data.data());
    std::copy(data, data + n - insert_num, bulk_load_data);
    std::vector<OperationType> operation_list(batch);

    for (long long i = 0; i < query_num; ++i) operation_list[i] = OperationType::Lookup;
    for (long long i = query_num; i < batch; ++i) operation_list[i] = OperationType::Insert;
    std::random_shuffle(operation_list.begin(), operation_list.end());

    {
        std::sort(bulk_load_data.begin(), bulk_load_data.end());
        index.bulk_load(bulk_load_data.data(), bulk_load_data.size());
        for (long long i = 0, qn = 0; i < batch; ++i){
            switch (operation_list[i]){
                case OperationType::Lookup:{
                    auto x = index.find(query[qn]);
                    if (x != index.end()){
                        if (x.data() != answer[qn]){
                            AEX_ERROR("lookup data error!");
                        }
                    }
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i]);
                }
                default:
                    break;
            }
        }
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr; inode = inode->next){
            //std::cout << "inode=" << inode << ", key[0]=" << static_cast<data_node_ptr>(inode)->key[0] << ;
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node);
            return false;
        }
        
        size_type i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        std::sort(data, data + n);
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    const int ITER = 10;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        value_type sum = 0;
        index.clear();
        index.bulk_load(bulk_load_data.data(), bulk_load_data.size());
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = 0, qn = 0; i < batch; ++i){
            switch (operation_list[i]){
                case OperationType::Lookup:{
                    auto x = index.find(query[qn]);
                    if (x != index.end())
                        sum += x.data();
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i]);
                }
                default:
                    break;
            }
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta +=  duration_cast<microseconds>(t2 - t1).count();
    }

    double OPS = 1.0 * 1e6 * ITER * batch/ delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("mix operation use time " << delta << "ms, OPS=" << OPS);

}
