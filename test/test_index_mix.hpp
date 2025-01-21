template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_mix_perf(std::pair<key_type, value_type>* data, long long n, long long batch, double rw_ratio){
    typedef aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    aex_tree<key_type, value_type, traits> index;
    std::map<key_type, value_type> mp;
    long long insert_num = static_cast<long long>(rw_ratio * batch);
    long long query_num = batch - insert_num;
    std::vector<std::pair<key_type, value_type> > insert_data(insert_num), bulk_load_data(n);
    std::vector<key_type> query(query_num);
    std::vector<value_type> answer(query_num), index_answer(query_num);
    std::random_shuffle(data, data + n);
    generate_query(data, n, query, answer, query_num);
    assert(n > insert_num);
    std::copy(data + n - insert_num, data + n, insert_data.data());
    std::copy(data, data + n - insert_num, bulk_load_data.data());
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
                    value_type y;
                    bool x = index.find(query[qn], y);
                    if (x == false || y != answer[qn]){
                        AEX_ERROR("lookup data error!");
                    }
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i].first, insert_data[i].second);
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
        size_t i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        std::sort(data, data + n);
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
                    value_type y;
                    bool x = index.find(query[qn], y);
                    if (x)
                        sum += y;
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i].first, insert_data[i].second);
                    break;
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
    return true;
}


template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_total_perf(std::pair<key_type, value_type>* data, long long n, long long read_nums, long long write_nums, long long erase_nums){
    
    AEX_HINT("[test index all interface]");
    typedef aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    aex_tree<key_type, value_type, traits> index;
    long long init_nums = n - write_nums, tot_nums = read_nums + write_nums + erase_nums;
    std::vector<std::pair<key_type, value_type>> init_data(init_nums), index_data(init_nums);
    std::vector<bool> is_delete(n);
    std::vector<std::pair<key_type, value_type>> insert_data(write_nums);
    std::random_shuffle(data, data + n);
    std::copy(data, data + init_nums, init_data.data());
    std::copy(data, data + init_nums, index_data.data());
    std::copy(data + init_nums, data + n, insert_data.data());
    std::sort(init_data.data(), init_data.data() + init_nums);
    index.bulk_load(init_data.data(), init_nums);
    AEX_PRINT("bulk_load finish...");
    std::vector<OperationType> opt(read_nums + write_nums + erase_nums);
    std::fill(opt.data(), opt.data() + read_nums, OperationType::Lookup);
    std::fill(opt.data() + read_nums, opt.data() + read_nums + write_nums, OperationType::Insert);
    std::fill(opt.data() + read_nums + write_nums, opt.data() + tot_nums, OperationType::Erase);
    std::random_shuffle(opt.data(), opt.data() + tot_nums);
    size_t insert_cnt = 0;
    AEX_PRINT("prepare finish...");
    
    index.print_stats();
    
    for (long long i = 0; i < tot_nums; ++i){
        switch (opt[i]){
            case OperationType::Lookup:{
                //AEX_PRINT("i=" << i << "Lookup:");
                if (index.size() < 100)
                    continue;
                size_t pos = rand() % index_data.size();
                while (is_delete[pos] == true) 
                    pos = rand() % index_data.size();
                value_type x;
                bool y = index.find(index_data[pos].first, x);
                if (!y){
                    AEX_ERROR("i=" << i << ", query error, pos=" << pos << ", key=" << index_data[pos].first << ", query no exists");
                    index.print_stats();
                    return false;
                }
                if (x != index_data[pos].second){
                    AEX_ERROR("i=" << i << ", query error, query key=" << index_data[pos].first << ", data=" << index_data[pos].second << ", get=" << x);
                    return false;
                } 
                break;
            }
            case OperationType::Insert:{
                //AEX_PRINT("i=" << i << "Insert:");
                index_data.emplace_back(insert_data[insert_cnt]);
                typename tree::iterator iter;
                bool _ = index.insert(insert_data[insert_cnt].first, insert_data[insert_cnt].second);
                if (_ == false){
                    AEX_ERROR("i=" << i << ", insert error, insert_key=" << insert_data[insert_cnt].first);
                    return false;
                }
                value_type x;
                bool y = index.find(insert_data[insert_cnt].first, x);
                if (!y){
                    AEX_ERROR("i=" << i << ", insert error, insert_key=" << insert_data[insert_cnt].first << " not found");
                    return false;
                }
                insert_cnt++;
                //index.debug(index.root);
                break;
            }
            case OperationType::Erase:{
                //AEX_PRINT("i=" << i << ", Erase:");
                size_t pos = rand() % index_data.size();
                while (is_delete[pos] == true) 
                    pos = rand() % index_data.size();
                auto _ = index.erase(index_data[pos].first);
                is_delete[pos] = true;
                if (_ == 0){
                    AEX_ERROR("i=" << i << "erase error!");
                    return false;
                }
                break;
            }
            default:
                break;
        }
    }

    {
        index.print_stats();
    }

    return true;
}