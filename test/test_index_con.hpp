#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_total_con_perf(std::pair<key_type, value_type>* data, long long n, long long read_nums, long long write_nums, long long erase_nums){
    AEX_HINT("[test index all interface]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_total_con_perf(std::pair<key_type, value_type>* data, long long n, long long read_nums, long long write_nums, long long erase_nums){
    AEX_HINT("[test index all interface]");
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    [[maybe_unused]]typedef typename tree::node_ptr node_ptr;
    mock_aex_tree<key_type, value_type, traits> index;
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
    ThreadPool tp;
    for (long long i = 0; i < tot_nums; ++i){
        switch (opt[i]){
            case OperationType::Lookup:{
                tp.add_task(test_lookup_con_unit, index, index_data[pos], id);

                size_type pos = rand() % index_data.size();
                while (is_delete[pos] == true) 
                    pos = rand() % index_data.size();

                auto x = index.find(index_data[pos].first);
                if (x == index.end()){
                    AEX_ERROR("i=" << i << ", query error, pos=" << pos << ", key=" << index_data[pos].first << ", query no exists");
                    index.print_stats();
                    index.print_detail();
                    return false;
                }
                if (x.key() != index_data[pos].first || x.data() != index_data[pos].second){
                    AEX_ERROR("i=" << i << ", query error, query key=" << index_data[pos].first << ", data=" << index_data[pos].second << ", get key=" << x.key() << ", data=" << x.data());
                    return false;
                } 
                break;
            }
            case OperationType::Insert:{
                index_data.push_back(insert_data[insert_cnt]);
                std::thread t2(test_insert_con_unit, index, index_data[insert_data[insert_cnt]]);
                insert_cnt++;
                break;
            }
            case OperationType::Erase:{                
                //AEX_PRINT("i=" << i << ", Erase:");
                size_type pos = rand() % index_data.size();
                while (is_delete[pos] == true) 
                    pos = rand() % index_data.size();
                tp.add_task(test_index_erase_unit, index, index_data[pos].first, id);
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
        tp.synchronize();
        if (index.size() != n - erase_nums){
            AEX_ERROR("CONCURRENCY ERROR!");
            return false;
        }
    }

    return true;
}