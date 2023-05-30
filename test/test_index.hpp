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
        printf("inner node=%lld, data node=%lld size=%lld height=%lld", st.inner_node, st.data_node, st.size, st.height);

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
    typedef typename aex::aex_map<key_type, value_type, traits> Index;
    typedef typename traits::size_type size_type;
    Index index;
    index.bulk_load(data, n);
    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    index.print_stats();
    
    size_type i = 0;
    for (auto iter = index.begin(); iter != index.end(); ++iter){
        if (data[i].first != iter.key()){
            AEX_ERROR("size error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
        }
        if (data[i].second != iter.data()){
            AEX_ERROR("size error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
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
    AEX_SUCCESS("bulk load use time " << delta << "ms, OPS=" << OPS);
    return true;
}