template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_index_RW_perf(std::pair<key_type, value_type>* data, long long n, long long batch, double rw_ratio){
    typedef mock_aex_tree<key_type, value_type, traits> tree;
    [[maybe_unused]]typedef typename mock_aex_tree<key_type, value_type, traits>::size_type size_type;
    typedef typename tree::node_ptr node_ptr;
    mock_aex_tree<key_type, value_type, traits> index;
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
                    auto x = index.find(query[qn]);
                    if (x != index.end()){
                        if (x.data() != answer[qn]){
                            AEX_ERROR("lookup data error!");
                        }
                    }
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i]);
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
        size_type leaf_num = 0, data_size = 0;
        for (node_ptr inode = index.head_leaf; inode != nullptr; inode = inode->next){
            //std::cout << "inode=" << inode << ", key[0]=" << static_cast<data_node_ptr>(inode)->key[0] << ;
            ++leaf_num;
            data_size += inode->size;
        }
        if (leaf_num != index.m_stats.data_node()){
            AEX_ERROR("leaf num error! leaf_num=" << leaf_num << "index.leaf_num=" << index.m_stats.data_node());
            return false;
        }
        
        size_type i = 0;
        //AEX_PRINT("slot_size=" << index.begin()._M_node->slot_size);
        std::sort(data, data + n);
        for (auto iter = index.begin(); iter != index.end(); ++iter, ++i){
            if (data[i].first != iter.key()){
                AEX_ERROR("key error, key[" << i << "]=" << iter.key() <<", real key=" << data[i].first);
                return false;
            }
            if (data[i].second != iter.data()){
                AEX_ERROR("data error, data[" << i << "]=" << iter.data() <<", real data=" << data[i].second);
                return false;
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
                    auto x = index.find(query[qn]);
                    if (x != index.end())
                        sum += x.data();
                    qn++;
                    break;
                }
                case OperationType::Insert:{
                    index.insert(insert_data[i]);
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