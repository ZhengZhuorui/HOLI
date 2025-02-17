#pragma once

struct alignas(64)
ThreadParam {
    std::vector<std::pair<uint64_t, uint64_t>> latency;
    uint64_t success_insert = 0;
    uint64_t success_read = 0;
    uint64_t success_update = 0;
    uint64_t success_remove = 0;
    uint64_t scan_not_enough = 0;
    uint64_t sum = 0;
};

template<typename key_type, typename traits>
bool test_hash_table_perf(key_type *data, size_t n, int thread_num){
    typedef aex_tree<key_type, key_type, traits> Index;
    Index index;
    typedef typename Index::HashTable HashTable;
    typedef typename HashTable::HashTableBlock HashTableBlock;
    std::cout << "sizeof(HashTableBlock)=" << sizeof(HashTableBlock) << std::endl;
    auto node = index.allocator.allocate_hash_node(512, 1);
    HashTable hash_table;
    ULL slot_size = 1;
    while (slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO < n) slot_size <<= 1;
    std::cout << slot_size;
    //hash_table.rescale(slot_size);
    hash_table.slot_size = slot_size;
    hash_table.real_slot_size = hash_table.get_real_slot_size(slot_size);
    hash_table.table_ = new HashTableBlock[slot_size];
    std::vector<ULL> id(n);
    for (size_t i = 0; i < n; ++i) id[i] = rand() % 64;
    for (size_t i = 1; i < n; ++i) id[i] += id[i - 1];
    std::random_shuffle(id.data(), id.data() + n);
    ThreadParam *params = new ThreadParam[thread_num];
    // test hash table insert performance
    system_clock::time_point t1, t2;
    double delta = 0, OPS = 0;
    t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < n; ++i)
        hash_table.insert(node, id[i], data[i], nullptr);
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("insert use time " << delta << "ms, OPS=" << OPS);
    // test hash table lookup performance
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
            auto res = hash_table.find(node, id[i]);
            thread_param.sum += (uint64_t)res.first;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    OPS = 1.0 * 1e6 * n / delta;
    uint64_t sum = 0;
    for (int i = 0; i < thread_num; ++i)
        sum += params[i].sum;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);  
    AEX_SUCCESS("code=" << sum << ", lookup use time " << delta << "ms, OPS=" << OPS);
    return true;
}