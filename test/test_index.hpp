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
        AEX_PRINT("inner node num=" << st.inner_node() << ", data node num=" << st.data_node() << "size=" << st.size << "height=" << st.height);

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
    //tmp = index;
    
    {
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        index.print_detail();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != index.empty_leaf; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << ", index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        if (data_size != index.size()){
            AEX_ERROR("size error, index.size=" << index.size() << ", data size=" << data_size);
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

        for (long long i = 0; i < n; ++i){
            //AEX_PRINT("key[" << i << "]=" << data[i].first);
            auto x = data[i];
            auto y = index.find(x.first);
            if (y == index.end()){
                AEX_ERROR("query no exists! i=" << i << "key=" << x.first);
                return false;
            }
            if (y.key() != x.first || y.data() != x.second){
                AEX_ERROR("query error! query key=" << x.first << ", data=" << x.second << ", get key=" << y.key() << ", data=" << y.data());
                return false;
            }
        }
    }
    AEX_SUCCESS("bulk load finish...");
    index.clear();
    if (index.size() != 0){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 1;
    for (int T = 0; T < ITER; ++T){
        index.clear();
        t1 = std::chrono::high_resolution_clock::now();
        index.bulk_load(data, n);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("bulk load use time " << delta << "ms, OPS=" << OPS);
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
    std::sort(data, data + n);
    index.bulk_load(data, n);
    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    AEX_HINT("bulk load finish...");
    
    for (int i = 0; i < batch; ++i){
        //AEX_PRINT("key=" << query[i]);
        auto iter = index.find(query[i]);
        if (iter == index.end()){
            AEX_ERROR("Query no exists, i=" << i << ", key=" << query[i]);
            return false;
        }
        if (iter.key() != query[i]){
            AEX_ERROR("Key Error!, i=" << i << "query key=" << query[i] << ", node key=" << iter.key() <<  ", answer=" << answer[i] << ", find=" << iter.data());
        }
        if (iter.data() != answer[i]){
            AEX_ERROR("Answer Error!, i=" << i << "query key=" << query[i] << ", node key=" << iter.key() <<  ", answer=" << answer[i] << ", find=" << iter.data());
            return false;
        }
    }
    index.print_stats();
    index.print_detail();
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 1;
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

    double QPS = 1.0 * 1e6 * ITER * batch/ delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << "query use time " << delta << "ms, QPS=" << QPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_delta_lookup_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    vector<key_type> query;
    vector<value_type> answer;
    generate_query(data, n, query, answer, batch);
    for (long long i = 0; i < n; ++i){
        //if (i % 1000000 == 0)
        //    std::cout << "i=" << i << std::endl;
        typename tree::iterator iter;
        bool inserted;
        std::tie(iter, inserted) = index.insert(data[i]);
        if (inserted == false){
            AEX_ERROR("insert failed!");
            return false;
        }
        if (iter.key() != data[i].first || iter.data() != data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< data[i].first << "iter key=" << iter.key());
            return false;
        }
        iter = index.find(data[i].first);
        if (iter.key() != data[i].first || iter.data() != data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< data[i].first << "iter key=" << iter.key());
            return false;
        }
    }
    AEX_SUCCESS("insert finish..");

    {   
        for (long long i = 0; i < n; ++i){
            typename tree::iterator iter;
            iter = index.find(data[i].first);
            if (iter.key() != data[i].first || iter.data() != data[i].second){
                AEX_ERROR("return iterator is not equal item! i=" << i << "key="<< data[i].first << "iter key=" << iter.key());
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        index.print_detail();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != index.empty_leaf; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << ", index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        size_type i = 0;
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first << ", gap=" << iter.key() - data[i].first);
                auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }

    for (int i = 0; i < batch; ++i){
        auto iter = index.find(query[i]);
        if (iter == index.end()){
            AEX_ERROR("Query no exists, i=" << i);
            return false;
        }
        if (iter.key() != query[i]){
            AEX_ERROR("Key Error!, i=" << i << "query key=" << query[i] << ", node key=" << iter.key() <<  ", answer=" << answer[i] << ", find=" << iter.data());
        }
        if (iter.data() != answer[i]){
            AEX_ERROR("Answer Error!, i=" << i << "query key=" << query[i] << ", node key=" << iter.key() <<  ", answer=" << answer[i] << ", find=" << iter.data());
            return false;
        }
    }
    index.print_stats();
    index.print_detail();
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 1;
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

    double QPS = 1.0 * 1e6 * ITER * batch/ delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << "query use time " << delta << "ms, QPS=" << QPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_insert_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index insert perf]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index, index_bak;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    std::vector<std::pair<key_type, value_type> > insert_data(batch), node_data(n - batch + 1); 
    std::random_shuffle(data, data + n);
    std::copy(data, data + n - batch, node_data.data());
    std::copy(data + n - batch, data + n, insert_data.data());
    std::sort(data, data + n);
    std::sort(node_data.data(), node_data.data() + n - batch);
    index.bulk_load(node_data.data(), n - batch);
    AEX_PRINT("bulk load finish...");
    index.print_stats();
    index.print_detail();
    index_bak = index;
    for (long long i = 0; i < batch; ++i){
        //if (i % 1000000 == 0)
        //    std::cout << "i=" << i << std::endl;
        typename tree::iterator iter;
        bool inserted;
        std::tie(iter, inserted) = index.insert(insert_data[i]);
        if (inserted == false){
            AEX_ERROR("insert failed!");
            return false;
        }
        if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
            return false;
        }
        iter = index.find(insert_data[i].first);
        if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
            return false;
        }
    }
    AEX_SUCCESS("insert finish..");

    {   
        for (long long i = 0; i < n; ++i){
            typename tree::iterator iter;
            iter = index.find(data[i].first);
            if (iter.key() != data[i].first || iter.data() != data[i].second){
                AEX_ERROR("return iterator is not equal item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        index.print_detail();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != index.empty_leaf; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << ", index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        size_type i = 0;
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first << ", gap=" << iter.key() - data[i].first);
                auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    AEX_SUCCESS("Test success. Next test insert performance...");
    const int ITER = 1;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index = index_bak;
        //aex_tree<key_type, value_type, traits>::debug_level |= 1;
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < batch; ++i)
            index.insert(insert_data[i]);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("insert use time " << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_insert_hotspot_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index insert hotspot perf]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index, index_bak;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    std::vector<std::pair<key_type, value_type> > insert_data(batch), node_data(n - batch + 1); 
    long long pos = rand() % (n - batch);
    std::copy(data, data + pos, node_data.data());
    std::copy(data + pos, data + pos + batch, insert_data.data());
    std::copy(data + pos + batch, data + n, node_data.data());
    random_shuffle(insert_data.data(), insert_data.data() + batch);
    index.bulk_load(node_data.data(), n - batch);
    AEX_PRINT("bulk load finish...");
    index.print_stats();
    index.print_detail();
    index_bak = index;
    for (long long i = 0; i < batch; ++i){
        //if (i % 1000000 == 0)
        //    std::cout << "i=" << i << std::endl;
        typename tree::iterator iter;
        bool inserted;
        std::tie(iter, inserted) = index.insert(insert_data[i]);
        if (inserted == false){
            AEX_ERROR("insert failed!");
            return false;
        }
        if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
            return false;
        }
        iter = index.find(insert_data[i].first);
        if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
            return false;
        }
    }
    AEX_SUCCESS("insert finish..");

    {   
        for (long long i = 0; i < n; ++i){
            typename tree::iterator iter;
            iter = index.find(data[i].first);
            if (iter.key() != data[i].first || iter.data() != data[i].second){
                AEX_ERROR("return iterator is not equal item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        index.print_detail();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != index.empty_leaf; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << ", index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        size_type i = 0;
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first << ", gap=" << iter.key() - data[i].first);
                auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
            }
        }
    }

    AEX_SUCCESS("Test success. Next test insert performance...");
    const int ITER = 1;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index = index_bak;
        //aex_tree<key_type, value_type, traits>::debug_level |= 1;
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < batch; ++i)
            index.insert(insert_data[i]);
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double OPS = 1.0 * 1e6 * ITER * batch / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("insert use time " << delta << "ms, OPS=" << OPS);
    return true;
}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_erase_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index erase]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    mock_aex_tree<key_type, value_type, traits> index;
    typedef typename tree::node_ptr node_ptr;
    [[maybe_unused]] typedef typename traits::size_type size_type;
    std::vector<key_type> erase_key(batch);
    std::vector<std::pair<key_type, value_type> > left_data(n - batch);
    std::random_shuffle(data, data + n);
    for (long long i = 0; i < batch; ++i)
        erase_key[i] = data[i].first;
    std::copy(data + batch, data + n, left_data.data());
    std::sort(data, data + n);
    std::sort(left_data.data(), left_data.data() + n - batch);
    index.bulk_load(data, n);
    index.print_stats();
    index.print_detail();
    AEX_SUCCESS("bulk load finish...");
    for (long long i = 0; i < batch; ++i)
        index.erase(erase_key[i]);
    AEX_SUCCESS("erase finish...");

    {
        if (static_cast<long long>(index.size()) != n - batch){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n - batch);
            return false;
        }
        index.print_stats();
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr && inode != index.empty_leaf; inode = inode->next){
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        size_type i = 0;
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (left_data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << left_data[i].first);
                return false;
            }
            if (left_data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << left_data[i].second);
                return false;
            }
        }
        for (auto &x : left_data){
            auto y = index.find(x.first);
            if (y == index.end()){
                AEX_ERROR("query no exists!");
                return false;
            }
            if (y.key() != x.first || y.data() != x.second){
                AEX_ERROR("query error! query key=" << x.first << ", data=" << x.second << ", get key=" << y.key() << ", data=" << y.data());
                return false;
            }
        }
    }
    AEX_SUCCESS("Erasion test success! Next test erasion performance...");

    index.clear();
    const int ITER = 1;
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
        typename traits=aex_default_traits<key_type, value_type> >
bool test_index_range_query_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    mock_aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    std::vector<std::pair<key_type, key_type> > query(batch);
    std::vector<size_t> answer;
    std::sort(data, data + n);
    index.bulk_load(data, n);
    double length_ratio = 0.001;
    const int times = 1;
    query.resize(batch);
    vector<long long> query_pos(batch);
    long long length = n * length_ratio;
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, batch, 0, n - 1 - length);
    value_type sum = 0, valid = 0;
    for (long long i = 0; i < batch; ++i){
        long long pos = query_pos[i];
        query[i].first = data[pos].first;
        query[i].second = data[pos + length - 1].first;
        for (int j = pos; j < pos + length; ++j)
            valid += data[j].second;
    }
    
    for (auto x : query){
        auto iter = index.find(x.first);
        while (iter != index.end() && iter.key() <= x.second){
            sum += iter.data();
            ++iter;
        }
    }
    if (sum != valid){
        AEX_ERROR("sum=" << sum << ", valid=" << valid);
        return false;
    }
    
    const int ITER = 1;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < times; ++T){
        sum = 0;
        for (auto x : query){
            auto iter = index.find(x.first);
            while (iter != index.end() && iter.key() <= x.second){
                sum += iter.data();
                ++iter;
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("erase use time " << delta << "ms, OPS=" << OPS);
    return true;
}

