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
    typedef aex_tree<key_type, key_type, traits> Index;
    //[[maybe_unused]]typedef typename Index::node_ptr node_ptr;
    [[maybe_unused]]typedef typename Index::node_ptr node_ptr;
    typedef typename Index::HashTable HashTable;
    typedef typename HashTable::HashTableBlock HashTableBlock;
    typedef typename Index::EpochGuard EpochGuard;
    Index index;
    srand(0);
    
    std::cout << "sizeof(HashTableBlock)=" << sizeof(HashTableBlock) << std::endl;
    HashTable &hash_table = index.hash_table;
    auto node = index.allocator.allocate_hash_node(512, 1);
    ULL slot_size = 1;
    while (slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO < n) slot_size <<= 1;
    std::cout << slot_size;
    //hash_table.rescale(slot_size);
    //hash_table.slot_size = slot_size;
    //hash_table.real_slot_size = hash_table.get_real_slot_size(slot_size);
    //hash_table.table_ = new HashTableBlock[slot_size];
    std::vector<ULL> id(n);
    for (size_t i = 0; i < n; ++i) id[i] = 1 + (rand() % 64);
    for (size_t i = 1; i < n; ++i) id[i] += id[i - 1];
    std::random_shuffle(id.data(), id.data() + n);
    ThreadParam *params = new ThreadParam[thread_num];
    // test hash table insert performance
    system_clock::time_point t1, t2;
    double delta = 0, OPS = 0;
    t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < n; ++i){
        EpochGuard guard(&index);
        hash_table.insert(node, id[i], data[i], nullptr);
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "insert use time " << delta << "ms, OPS=" << OPS << std::endl;
    // test hash table lookup performance
    t1 = std::chrono::high_resolution_clock::now();
    //if constexpr(traits::AllowConcurrency){
    #pragma omp parallel num_threads(thread_num)
    {
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        #pragma omp barrier
        #pragma omp master
        t1 = std::chrono::high_resolution_clock::now();
        #pragma omp for schedule(dynamic, 10000)
        for (size_t i = 0; i < n; ++i){
            //std::pair<key_type, node_ptr> res;
            //hash_table.find(node, id[i], res);
            auto res = hash_table.find(node, id[i]);

            thread_param.sum += (uint64_t)res.first;
        }
    }
    //}
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    uint64_t sum = 0;
    for (int i = 0; i < thread_num; ++i)
        sum += params[i].sum;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "code=" << sum << ", lookup use time " << delta << "ms, OPS=" << OPS << std::endl;

    //#pragma omp parallel num_threads(thread_num)
    /*{
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        #pragma omp barrier
        #pragma omp master
        t1 = std::chrono::high_resolution_clock::now();
        #pragma omp for schedule(dynamic, 10000)
        for (size_t i = 0; i < n; ++i){
            auto res = hash_table.find(node, id[i]);
            if (res.first == data[i]) ++thread_param.success_read;
        }
    }
    size_t success_read = 0;
    for (int i = 0; i < thread_num; ++i)
        success_read += params[i].success_read;
    if (success_read != n){
        AEX_ERROR("test failed. success_read=" << success_read);
        return false;
    }*/
    //size_t success_read = 0;

    std::unordered_map<std::pair<uint64_t, key_type>, std::pair<uint64_t, uint64_t>, PairHash > mp;

    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i){
        mp.emplace(std::make_pair(node->id, id[i]), std::make_pair(data[i], 0));
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    std::cout << "insert use time " << delta << "ms, OPS=" << OPS << std::endl;
    // test hash table lookup performance
    t1 = std::chrono::high_resolution_clock::now();
    //if constexpr(traits::AllowConcurrency){
    #pragma omp parallel num_threads(thread_num)
    {
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        #pragma omp barrier
        #pragma omp master
        t1 = std::chrono::high_resolution_clock::now();
        #pragma omp for schedule(dynamic, 10000)
        for (size_t i = 0; i < n; ++i){
            //std::pair<key_type, node_ptr> res;
            //hash_table.find(node, id[i], res);
            auto res = mp.find(std::make_pair(node->id, id[i]));
            thread_param.sum += (*res).second.first;
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