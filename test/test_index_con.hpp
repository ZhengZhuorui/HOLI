#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
bool test_index_total_con_perf(std::pair<key_type, value_type>* data, long long n, long long read_nums, long long write_nums, long long erase_nums){
    
    AEX_HINT("[test index concurrency with all interface]");
    typedef mock_aex_tree_con<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    mock_aex_tree_con<key_type, value_type, traits> index;
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
    size_type insert_cnt = 0;
    AEX_PRINT("prepare finish...");
    ThreadPool tp(std::thread::hardware_concurrency());
    for (long long i = 0; i < tot_nums; ++i){
        switch (opt[i]){
            case OperationType::Lookup:{
                size_type pos = rand() % index_data.size();
                std::function<void()> t = std::bind(test_lookup_con_unit<key_type, value_type>, index, index_data[pos], i);
                //std::function<void()> t = std::bind(test_lookup_con_unit<key_type, value_type>, index, index_data[pos], i);
                tp.enqueue(t);
                break;
            }
            case OperationType::Insert:{
                index_data.push_back(insert_data[insert_cnt]);
                //std::thread t2(test_insert_con_unit, index, insert_data[insert_cnt]);
                std::function<void()> t = std::bind(test_insert_con_unit<key_type, value_type>, index, insert_data[insert_cnt], i);
                tp.enqueue(t);
                insert_cnt++;
                break;
            }
            case OperationType::Erase:{                
                //AEX_PRINT("i=" << i << ", Erase:");
                size_type pos = rand() % index_data.size();
                while (is_delete[pos] == true) 
                    pos = rand() % index_data.size();
                std::function<void()> t = std::bind(test_erase_con_unit<key_type, value_type>, index, index_data[pos].first, i);
                tp.enqueue(t);
                is_delete[pos] = true;
                break;
            }
            default:
                break;
        }
        //tp.synchronize();
        if (static_cast<long long>(index.size()) != n - erase_nums){
            AEX_ERROR("CONCURRENCY ERROR!");
            return false;
        }
    }

    return true;
}