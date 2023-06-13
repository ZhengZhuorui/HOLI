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

    index.bulk_load(data, n);
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
    typedef typename aex::aex_map<key_type, value_type, traits> Index;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    vector<key_type> query;
    vector<value_type> answer;
    generate_query(data, n, query, answer, batch);
    std::sort(data, data + n);
    Index index;
    index.bulk_load(data, n);
    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    index.print_stats();
    for (int i = 0; i < batch; ++i){
        auto iter = index.find(query[i]);
        if (iter.data() != answer[i]){
            AEX_ERROR("Answer Error!, key=" << query[i] << ", answer=" << answer[i] << ", find=" << iter.data());
            return false;
        }
    }
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 10;
    value_type sum = 0;
    for (int T = 0; T < ITER; ++T){
        index.clear();
        t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < batch; ++i){
            auto iter = index.find(query[i]);
            sum += iter.data();
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    double OPS = 1.0 * 1e6 * ITER * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << "query use time " << delta << "ms, NPS=" << OPS);
    return true;
}