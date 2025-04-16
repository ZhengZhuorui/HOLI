#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
template<typename K, typename V, typename traits>
bool test_index(std::pair<K, V>* data, size_t n){
    aex::aex_tree<K , V, traits> mp;
    random_shuffle(data, data + n);
    // insert
    for (size_t i = 0; i < n; ++i)
        //mp.insert_con(std::make_pair(data[i].first, data[i].second));
        mp.insert(data[i].first, data[i].second);
    
    // find
    int M = std::min(n, (size_t)100);
    V y;
    for (int i = 0; i < M; ++i){
        size_t x = rand() % n;
        bool res = mp.find(data[x].first, y);
        if (res || y != data[x].second){
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
        aex::aex_tree<K, V> mp;
        mp.bulk_load(data, n);
        mp.print_stats();
        //typename aex::aex_tree<K, V>::stats st = mp.get_stats();
        //printf("inner node=%lld, data node=%lld size=%lld height=%lld", st.inner_node, st.data_node, st.size, st.height);
        //AEX_PRINT("inner node num=" << st.inner_node() << ", data node num=" << st.data_node() << "size=" << st.size << "height=" << st.height);

        // find
        for (int i = 0; i < M; ++i){
            size_t x = rand() % n;
            V y;
            bool z = mp.find(data[x].first, y);
            if (!z || y != data[x].second){
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

    aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]]typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    std::sort(data, data + n);
    index.bulk_load(data, n);
    //tmp = index;
    
    {
        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_t i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        for (auto node = index.head_leaf; node != nullptr; node = node->next){

            for (int j = 0; j < node->size; ++j, ++i){
                if (data[i].first != node->key[j]){
                    AEX_ERROR("key error, key[" << i << "]=" << node->key[j] <<", real key=" << data[i].first << ", gap=" << node->key[j] - data[i].first);
                    //auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                    //AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                    return false;
                }
                if (data[i].second != node->data[j]){
                    AEX_ERROR("data error, data[" << i << "]=" << node->data[j] <<", real data=" << data[i].second);
                    return false;
                }
            }
        }

        for (long long i = 0; i < n; ++i){
            //AEX_PRINT("key[" << i << "]=" << data[i].first);
            auto x = data[i];
            value_type y;
            bool z = index.find(x.first, y);
            if (!z){
                AEX_ERROR("query no exists! i=" << i << "key=" << x.first);
                return false;
            }
            if (y != x.second){
                AEX_ERROR("query error! query key=" << x.first << ", data=" << x.second << ", get data=" << y);
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
    aex_tree<key_type, value_type, traits> index;
    vector<key_type> query;
    vector<value_type> answer;
    generate_query(data, n, query, answer, batch);
    index.bulk_load(data, n);
    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }
    AEX_HINT("bulk load finish...");
    index.print_stats();

    for (int i = 0; i < batch; ++i){
        //AEX_PRINT("key=" << query[i]);
        if (i % 1000000 == 0)
            AEX_PRINT("i=" << i);
        value_type res;
        //auto iter = index.find(query[i]);
        bool find_flag = index.find(query[i], res);
        //AEX_ASSERT(iter.data() == res);
        if (!find_flag){
            AEX_ERROR("Query no exists, i=" << i << ", key=" << query[i]);
            return false;
        }
        if (res != answer[i]){
            AEX_ERROR("Answer Error!, i=" << i << "query key=" << query[i] <<  ", answer=" << answer[i] << ", get=" << res);
            return false;
        }        
    }
    
    index.print_stats();
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 1;
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (long long i = 0; i < batch; ++i){
            value_type res;
            if (index.find(query[i], res))
                sum += res;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1.0 * 1e9 * ITER * batch/ delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << "query use time " << delta << "ms, QPS=" << QPS);
    return true;
}

/*
template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_lookup_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    //typedef typename aex::aex_map<key_type, value_type, traits> Index;
    aex_tree<key_type, value_type, traits> index;
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
*/

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_index_delta_lookup_perf(std::pair<key_type, value_type>* data, long long n, long long batch){
    AEX_HINT("[test index lookup]");
    typedef aex_tree<key_type, value_type, traits> tree;
    aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    vector<key_type> query;
    vector<value_type> answer;
    generate_query(data, n, query, answer, batch);
    for (long long i = 0; i < n; ++i){
        //if (i % 1000000 == 0)
            //std::cout << "i=" << i << std::endl;
            AEX_PRINT("i=" << i);
        typename tree::iterator iter;
        bool inserted = index.insert(data[i]);
        if (!inserted){
            AEX_ERROR("insert failed!");
            return false;
        }
        value_type res;
        bool find_flag = index.find(data[i].first, res);
        if (!find_flag){
            AEX_ERROR("query no exists!");
            return false;
        }
        if (res != data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< data[i].second << ", res =" << res);
            return false;
        }
    }
    
    AEX_SUCCESS("insert finish..");

    {   
        for (long long i = 0; i < n; ++i){
            value_type res;
            bool find_flag = index.find(data[i].first, res);
            if (!find_flag){
                AEX_ERROR("query no exists!");
                return false;
            }
            if (res != data[i].second){
                AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< data[i].second << ", res =" << res);
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_t i = 0;
        for (auto node = index.head_leaf; node != nullptr; node = node->next){

            for (int j = 0; j < node->size; ++j, ++i){
                if (data[i].first != node->key[j]){
                    AEX_ERROR("key error, key[" << i << "]=" << node->key[j] <<", real key=" << data[i].first << ", gap=" << node->key[j] - data[i].first);
                    //auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                    //AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                    return false;
                }
                if (data[i].second != node->data[j]){
                    AEX_ERROR("data error, data[" << i << "]=" << node->data[j] <<", real data=" << data[i].second);
                    return false;
                }
            }
        }
    }

    if (static_cast<long long>(index.size()) != n){
        AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
        return false;
    }

    for (int i = 0; i < batch; ++i){
        value_type res;
        bool find_flag = index.find(query[i], res);
        if (!find_flag){
            AEX_ERROR("query no exists!");
            return false;
        }
        if (res != answer[i]){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< query[i] << ", answer=" << answer[i] << ", res =" << res);
            return false;
        }
    }
    index.print_stats();
    
    system_clock::time_point t1, t2;
    double delta = 0;
    const int ITER = 1;
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < ITER; ++T){
        for (long long i = 0; i < batch; ++i){
            value_type res;
            index.find(query[i], res);
            sum += res;
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
    typedef aex_tree<key_type, value_type, traits> tree;
    aex_tree<key_type, value_type, traits> index, index_bak;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    std::vector<std::pair<key_type, value_type> > insert_data(batch), node_data(n - batch + 1), tmp_data(n); 
    std::copy(data, data + n, tmp_data.data());
    std::random_shuffle(tmp_data.data(), tmp_data.data() + n);
    std::copy(tmp_data.data(), tmp_data.data() + n - batch, node_data.data());
    std::copy(tmp_data.data() + n - batch, tmp_data.data() + n, insert_data.data());
    std::sort(node_data.data(), node_data.data() + n - batch);
    AEX_SUCCESS("dataset prepare finish...");
    index.bulk_load(node_data.data(), n - batch);
    AEX_SUCCESS("bulk load finish...");
    index.print_stats();
    index_bak = index;
    for (long long i = 0; i < batch; ++i){
        if (i % 1000000 == 0)
            AEX_PRINT("i=" << i << ", key=" << insert_data[i].first);
        typename tree::iterator iter;
        bool inserted = index.insert(insert_data[i]);
        if (inserted == false){
            AEX_ERROR("insert failed!");
            return false;
        }
        //if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
        //    AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
        //    return false;
        //}
        //iter = index.find(insert_data[i].first);
        value_type res;
        bool find_flag = index.find(insert_data[i].first, res);
        if (!find_flag){
            AEX_ERROR("query " << i << " no exists!");
            index.print_stats();
            return false;
        }
        if (res != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].second << ", res =" << res);
            return false;
        }
    }
    AEX_SUCCESS("insert finish..");
    index.print_stats();

    {   
        for (long long i = 0; i < n; ++i){
            value_type res;
            bool find_flag = index.find(data[i].first, res);
            if (!find_flag){
                AEX_ERROR("query no exists! i=" << i << ", key=" << data[i].first);
                return false;
            }
            if (res != data[i].second){
                AEX_ERROR("return iterator is not equal insert item! i=" << i << "answer="<< data[i].second << ", res =" << res);
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_t i = 0;
        for (auto node = index.head_leaf; node != nullptr; node = node->next){

            for (int j = 0; j < node->size; ++j, ++i){
                if (data[i].first != node->key[j]){
                    AEX_ERROR("key error, key[" << i << "]=" << node->key[j] <<", real key=" << data[i].first << ", gap=" << node->key[j] - data[i].first);
                    //auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                    //AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                    return false;
                }
                if (data[i].second != node->data[j]){
                    AEX_ERROR("data error, data[" << i << "]=" << node->data[j] <<", real data=" << data[i].second);
                    return false;
                }
            }
        }
    }

    AEX_SUCCESS("Test success. Next test insert performance...");
    const int ITER = 1;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index = index_bak;
        t1 = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < batch; ++i){
            if (i % 1000000 == 0)
                AEX_PRINT("i=" << i << ", key=" << insert_data[i].first);
            index.insert(insert_data[i]);
        }
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
    typedef aex_tree<key_type, value_type, traits> tree;
    aex_tree<key_type, value_type, traits> index, index_bak;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    std::sort(data, data + n);
    std::vector<std::pair<key_type, value_type> > insert_data(batch), node_data(n - batch + 1); 
    //long long pos = rand() % (n - batch);
    long long pos = n - batch;
    std::copy(data, data + pos, node_data.data());
    std::copy(data + pos, data + pos + batch, insert_data.data());
    std::copy(data + pos + batch, data + n, node_data.data() + pos);
    //random_shuffle(insert_data.data(), insert_data.data() + batch);
    index.bulk_load(node_data.data(), n - batch);
    AEX_PRINT("bulk load finish...");
    index.print_stats();
    index_bak = index;
    for (long long i = 0; i < batch; ++i){
        if (i % 1000000 == 0)
            std::cout << "i=" << i << std::endl;
        typename tree::iterator iter;
        bool inserted = index.insert(insert_data[i]);
        if (inserted == false){
            AEX_ERROR("insert failed!");
            return false;
        }
        //if (iter.key() != insert_data[i].first || iter.data() != insert_data[i].second){
        //    AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].first << "iter key=" << iter.key());
        //    return false;
        //
        value_type res;
        bool find_flag = index.find(insert_data[i].first, res);
        if (!find_flag){
            AEX_ERROR("query no exists!");
            return false;
        }
        if (res != insert_data[i].second){
            AEX_ERROR("return iterator is not equal insert item! i=" << i << ", answer="<< insert_data[i].second << ", res =" << res);
            return false;
        }
    }
    AEX_SUCCESS("insert finish..");

    {   
        for (long long i = 0; i < n; ++i){
            value_type res;
            bool find_flag = index.find(insert_data[i].first, res);
            if (!find_flag){
                AEX_ERROR("query no exists!");
                return false;
            }
            if (res != insert_data[i].second){
                AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< insert_data[i].second << ", res =" << res);
                return false;
            }
        }

        if (static_cast<long long>(index.size()) != n){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n);
            return false;
        }
        index.print_stats();
        size_t i = 0;
        for (auto node = index.head_leaf; node != nullptr; node = node->next){

            for (int j = 0; j < node->size; ++j, ++i){
                if (data[i].first != node->key[j]){
                    AEX_ERROR("key error, key[" << i << "]=" << node->key[j] <<", real key=" << data[i].first << ", gap=" << node->key[j] - data[i].first);
                    //auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                    //AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                    return false;
                }
                if (data[i].second != node->data[j]){
                    AEX_ERROR("data error, data[" << i << "]=" << node->data[j] <<", real data=" << data[i].second);
                    return false;
                }
            }
        }
    }

    AEX_SUCCESS("Test success. Next test insert performance...");
    const int ITER = 1;
    std::chrono::high_resolution_clock::time_point t1, t2;
    double delta = 0;
    for (int T = 0; T < ITER; ++T){
        index = index_bak;
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
    typedef aex_tree<key_type, value_type, traits> tree;
    aex_tree<key_type, value_type, traits> index;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
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
    AEX_SUCCESS("bulk load finish...");
    for (long long i = 0; i < batch; ++i){
        if (i % 1000000 == 0)
            AEX_PRINT("i=" << i << ", key=" << erase_key[i]);
        index.erase(erase_key[i]);
    }
    AEX_SUCCESS("erase finish...");

    {
        if (static_cast<long long>(index.size()) != n - batch){
            AEX_ERROR("size error, index.size=" << index.size() << ", n=" << n - batch);
            return false;
        }
        index.print_stats();

        size_t i = 0;
        for (auto node = index.head_leaf; node != nullptr; node = node->next){
            for (int j = 0; j < node->size; ++j, ++i){
                if (data[i].first != node->key[j]){
                    AEX_ERROR("key error, key[" << i << "]=" << node->key[j] <<", real key=" << data[i].first << ", gap=" << node->key[j] - data[i].first);
                    //auto _ = std::lower_bound(data, data + n, std::make_pair(iter.key(), iter.data())) - data;
                    //AEX_ERROR("iter_key position=" << _ << ", now position=" << i);
                    return false;
                }
                if (data[i].second != node->data[j]){
                    AEX_ERROR("data error, data[" << i << "]=" << node->data[j] <<", real data=" << data[i].second);
                    return false;
                }
            }
        }
        for (auto &x : left_data){
            value_type res;
            bool find_flag = index.find(x.first, res);
            if (!find_flag){
                AEX_ERROR("query no exists!");
                return false;
            }
            if (res != left_data[i].second){
                AEX_ERROR("return iterator is not equal insert item! i=" << i << "key="<< x.second << ", res =" << res);
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
    aex_tree<key_type, value_type, traits> index;
    std::vector<std::pair<key_type, key_type> > query(batch);
    std::vector<size_t> answer;
    std::sort(data, data + n);
    index.bulk_load(data, n);
    const int times = 1;
    query.resize(batch);
    vector<long long> query_pos(batch);
    long long length = 100;
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, batch, 0, n - 1 - length);
    value_type sum = 0, valid = 0;
    for (long long i = 0; i < batch; ++i){
        long long pos = query_pos[i];
        query[i].first = data[pos].first;
        query[i].second = data[pos + length - 1].first;
        for (int j = pos; j < pos + length; ++j)
            valid += data[j].second;
    }
    std::vector<std::pair<key_type, value_type> > results;
    
    for (auto x : query){
        results.clear();
        index.range_query(x.first, x.second, results);
        for (auto &y : results)
            sum += y.second;
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
            results.clear();
            index.range_query(x.first, x.second, results);
            for (auto &y : results)
                sum += y.second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    double OPS = 1.0 * 1e6 * ITER / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("sum=" << sum << "range_query use time " << delta << "ms, OPS=" << OPS);
    return true;
}

#pragma GCC diagnostic pop