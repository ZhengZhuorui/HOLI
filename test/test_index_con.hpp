#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
struct OperationUnit{
    OperationType type;
    key_type key;
    value_type value;
};

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
bool test_index_total_con_perf(std::pair<key_type, value_type>* data, long long n, long long thread_num, long long read_nums, long long write_nums, long long erase_nums){
    AEX_HINT("[test index concurrency with all interface]");
    // TODO: need to finish erase
    AEX_ASSERT(erase_nums == 0);
    AEX_ASSERT(n >= write_nums);
    typedef OperationUnit<key_type, value_type, traits> Unit;
    typedef aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    aex_tree<key_type, value_type, traits> index;
    long long init_nums = n - write_nums, tot_nums = read_nums + write_nums + erase_nums;
    std::vector<std::pair<key_type, value_type>> init_data(init_nums);
    std::random_shuffle(data, data + n);
    std::copy(data, data + init_nums, init_data.data());
    std::sort(init_data.data(), init_data.data() + init_nums);
    index.bulk_load(init_data.data(), init_nums);
    AEX_PRINT("bulk_load finish...");
    std::vector<Unit> opt(read_nums + write_nums + erase_nums);
    for (int i = 0; i < read_nums; ++i){
        size_t pos = rand() % init_data.size();
        opt[i].type = OperationType::Lookup;
        opt[i].key = init_data[pos].first;
        opt[i].value = init_data[pos].second;

    }
    for (int i = read_nums; i < read_nums + write_nums; ++i){
        opt[i].type = OperationType::Insert;
        opt[i].key = data[i].first;
        opt[i].value = data[i].second;
    }
    std::random_shuffle(opt.data(), opt.data() + tot_nums);
    ThreadParam *params = new ThreadParam[thread_num];
    AEX_PRINT("prepare finish...");

    #pragma omp parallel num_threads(thread_num)
    {
        auto thread_id = omp_get_thread_num();
        ThreadParam &thread_param = params[thread_id];
        //#pragma omp barrier
        //#pragma omp master

        #pragma omp for schedule(dynamic, 10000)
        for (long long i = 0; i < tot_nums; ++i){
            if (i % 1000000 == 0) 
                AEX_PRINT("i=" << i);
            switch (opt[i].type){
            case OperationType::Lookup:{
                value_type _val;
                thread_param.success_read += index.find(opt[i].key, _val);
                AEX_ASSERT(_val == opt[i].value);
                break;
            }
            case OperationType::Insert:{
                thread_param.success_insert += index.insert(opt[i].key, opt[i].value);

                value_type _val;
                [[maybe_unused]]bool _ = index.find(opt[i].key, _val);
                AEX_ASSERT(_ == true);
                if (_val != opt[i].value)
                    AEX_PRINT(_val << ", " << opt[i].value);
                AEX_ASSERT(_val == opt[i].value);

                break;
            }
            default:
                break;
            }
        }
    }

    for (long long i = 0; i < tot_nums; ++i){
        if (i % 1000000 == 0) 
            AEX_PRINT("i=" << i);
        switch (opt[i].type){
            case OperationType::Lookup:{
                value_type _val;
                [[maybe_unused]]bool _ = index.find(opt[i].key, _val);
                AEX_ASSERT(_ == true);
                AEX_ASSERT(_val == opt[i].value);
                break;
            }
            case OperationType::Insert:{
                value_type _val;
                [[maybe_unused]]bool _ = index.find(opt[i].key, _val);
                AEX_ASSERT(_ == true);
                if (_val != opt[i].value)
                    AEX_PRINT(_val << ", " << opt[i].value);
                AEX_ASSERT(_val == opt[i].value);
                break;
            }
            default:
                break;
        }
    }

    index.print_stats();
    ThreadParam tot;
    for (int i = 0; i < thread_num; ++i){
        tot.success_read += params[i].success_read;
        tot.success_insert += params[i].success_insert;
    }
    if ((long long)tot.success_read != read_nums){
        AEX_ERROR("test failed. success_read=" << tot.success_read);
        return false;
    }
    if ((long long)tot.success_insert != write_nums){
        AEX_ERROR("test failed. success_insert=" << tot.success_insert);
        return false;
    }
    return true;
}