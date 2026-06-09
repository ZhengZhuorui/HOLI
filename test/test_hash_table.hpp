#pragma once

struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1);
    }
};

template<typename key_type, typename traits>
bool test_hash_table_perf(key_type *data, size_t n, int thread_num){
    AEX_HINT("[test hash table performance]");
    typedef aex_tree<key_type, key_type, traits> Index;
    //[[maybe_unused]]typedef typename Index::node_ptr node_ptr;
    //[[maybe_unused]]typedef typename Index::node_ptr node_ptr;
    using node_ptr = typename Index::node_ptr;
    //using HashTable = aex::aex_hash_table<LL, std::pair<key_type, node_ptr>, traits>;
    using HashTable = typename Index::HashTable;

    Index index;
    srand(0);
    AEX_PRINT("n=" << n << ", thread_num=" << thread_num);
    ULL slot_size = 1;
    while (slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO < n) slot_size <<= 1;
    AEX_PRINT("slot_size=" << slot_size);
    //auto node = new hash_node(slot_size, m);
    HashTable table(slot_size);
    std::vector<LL> id(n);
    for (size_t i = 0; i < n; ++i) id[i] = 1 + (rand() % 64);
    for (size_t i = 1; i < n; ++i) id[i] += id[i - 1];
    std::random_shuffle(id.data(), id.data() + n);
    ThreadParam *params = new ThreadParam[thread_num];

    AEX_PRINT("begin insert");
    // test hash table insert performance
    system_clock::time_point t1, t2;
    double delta = 0, OPS = 0;
    t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < n; ++i){
        table.insert(id[i], std::make_pair(data[i], nullptr));
    }
    t2 = std::chrono::high_resolution_clock::now();
    AEX_PRINT("finish insert");
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "insert use time " << delta << "ms, OPS=" << OPS << std::endl;

    AEX_PRINT("begin lookup");
    // test hash table lookup performance
    for (int i = 0; i < thread_num; ++i)
        params[i].sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel num_threads(thread_num)
    {
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        #pragma omp barrier
        #pragma omp master
        t1 = std::chrono::high_resolution_clock::now();
        #pragma omp for schedule(dynamic, 10000)
        for (size_t i = 0; i < n; ++i){
            thread_param.sum += table.find(id[i]).first;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    AEX_PRINT("finish lookup");
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    uint64_t sum = 0;
    for (int i = 0; i < thread_num; ++i)
        sum += params[i].sum;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "code=" << sum << ", lookup use time " << delta << "ms, OPS=" << OPS << std::endl;

    //size_t success_read = 0;

    //std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> mp;
    std::unordered_map<uint64_t, std::pair<key_type, node_ptr>> mp;

    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i){
        //mp.emplace(id[i], std::make_pair(data[i], 0));
        mp.emplace(id[i], std::make_pair(data[i], nullptr));
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "insert use time " << delta << "ms, OPS=" << OPS << std::endl;
    // test hash table lookup performance
    
    for (int i = 0; i < thread_num; ++i)
        params[i].sum = 0;
    
    
    t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel num_threads(thread_num)
    {
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        #pragma omp barrier
        #pragma omp master
        t1 = std::chrono::high_resolution_clock::now();
        #pragma omp for schedule(dynamic, 10000)
        for (size_t i = 0; i < n; ++i){
            thread_param.sum += mp.find(id[i])->second.first;
        }
    }
    //}
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    sum = 0;
    for (int i = 0; i < thread_num; ++i)
        sum += params[i].sum;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "code=" << sum << ", lookup use time " << delta << "ms, OPS=" << OPS << std::endl;

    //if (flag) AEX_SUCCESS("test success.");
    //return flag;
    return true;    
}