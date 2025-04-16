

template<typename key_type, typename value_type>
void aex_mix_bench(vector<pair<key_type, value_type> > &init_data, vector<int> &opt, vector<pair<key_type, value_type>> &insert_data, vector<key_type> &query, vector<value_type> &answer){
    aex_tree<key_type, value_type, aex::aex_default_traits<key_type, value_type, false, void, false, false>> index;
    index.bulk_load(init_data.data(), init_data.size());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("aex map query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        int j = 0, k = 0;
        for (size_t i = 0; i < num_ops; ++i){
            if (opt[i] == 0){
                const auto iter = index.find(query[j++]);
                if (iter != index.end())
                sum += iter.data();
            }
            else{
                typename aex_tree<key_type, value_type>::iterator iter;
                index.insert(insert_data[k++]);
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void stlmap_mix_bench(vector<pair<key_type, value_type> > &init_data, vector<int> &opt, vector<pair<key_type, value_type>> &insert_data, vector<key_type> &query, vector<value_type> &answer){
    std::map<key_type, value_type> index(init_data.begin(), init_data.end());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("stl map query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        int j = 0, k = 0;
        for (size_t i = 0; i < num_ops; ++i){
            if (opt[i] == 0){
                const auto iter = index.find(query[j++]);
                if (iter != index.end())
                    sum += iter->second;
            }
            else{
                typename std::map<key_type, value_type>::iterator iter;
                bool inserted;
                std::tie(iter, inserted) = index.insert(insert_data[k++]);
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void stx_btree_mix_bench(vector<pair<key_type, value_type> > &init_data, vector<int> &opt, vector<pair<key_type, value_type>> &insert_data, vector<key_type> &query, vector<value_type> &answer){
    std::map<key_type, value_type> index(init_data.begin(), init_data.end());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("stx btree query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        int j = 0, k = 0;
        for (size_t i = 0; i < num_ops; ++i){
            if (opt[i] == 0){
                const auto iter = index.find(query[j++]);
                if (iter != index.end())
                    sum += iter->second;
            }
            else{
                typename std::map<key_type, value_type>::iterator iter;
                bool inserted;
                std::tie(iter, inserted) = index.insert(insert_data[k++]);
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void alex_mix_bench(vector<pair<key_type, value_type> > &init_data, vector<int> &opt, vector<pair<key_type, value_type>> &insert_data, vector<key_type> &query, vector<value_type> &answer){
    alex::Alex<key_type, value_type> index(init_data.begin(), init_data.end());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("alex query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        int j = 0, k = 0;
        for (size_t i = 0; i < num_ops; ++i){
            if (opt[i] == 0){
                const auto iter = index.find(query[j++]);
                if (iter != index.end())
                    sum += iter.payload();
            }
            else{
                typename alex::Alex<key_type, value_type>::Iterator iter;
                bool inserted;
                std::tie(iter, inserted) = index.insert(insert_data[k++]);
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void lipp_mix_bench(vector<pair<key_type, value_type> > &init_data, vector<int> &opt, vector<pair<key_type, value_type>> &insert_data, vector<key_type> &query, vector<value_type> &answer){
    alex::Alex<key_type, value_type> index(init_data.begin(), init_data.end());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("lipp query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        int j = 0, k = 0;
        for (size_t i = 0; i < num_ops; ++i){
            if (opt[i] == 0){
                const auto iter = index.find(query[j++]);
                if (iter != index.end())
                    sum += iter.data();
            }
            else{
                typename alex::Alex<key_type, value_type>::iterator iter;
                bool inserted;
                std::tie(iter, inserted) = index.insert(insert_data[k++]);
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type,
        typename value_type>
void benchmark_mix(FILE* file, long long num_keys, long long num_ops, double read_ratio, std::string &index_name, std::string &query_dis){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    assert((long long)_ == num_keys);
    pack_KV_dataset(bin_data, data);
    std::random_shuffle(data.begin(), data.end());
    long long read_nums = num_ops * read_ratio; 
    long long insert_nums = num_ops - read_nums;
    long long init_nums = num_keys - insert_nums;
    assert(insert_nums > num_keys);
    std::vector<std::pair<key_type, value_type> > init_data(init_nums), insert_data(insert_nums);
    std::vector<key_type> query;
    std::vector<value_type> answer;
    std::vector<int> opt(num_ops);
    std::fill(opt.data(), opt.data() + insert_nums, 1);
    std::random_shuffle(opt.begin(), opt.end());
    std::copy(data.data(), data.data() + init_nums, init_data.data());
    std::copy(data.data() + init_nums, data.data() + num_keys, insert_data.data());
    if (query_dis == "uniform")
        generate_query<key_type, value_type, std::uniform_int_distribution<long long> >(data, query, answer, num_ops);
    else if (query_dis == "zipfian")
        generate_query_zipf<key_type, value_type>(data, query, answer, num_ops);
    
    if (index_name == "aex"){
        aex_mix_bench(init_data, opt, insert_data, query, answer);
    }
    else if (index_name == "stl_map"){
        stlmap_mix_bench(init_data, opt, insert_data, query, answer);
    }
    else if (index_name == "stx_btree"){
        stx_btree_mix_bench(init_data, opt, insert_data, query, answer);
    }
    else if (index_name == "alex"){
        alex_mix_bench(init_data, opt, insert_data, query, answer);
    }
    //else if (index_name == "lipp"){
    //    lipp_mix_bench(init_data, opt, insert_data, query, answer);
    //}
}