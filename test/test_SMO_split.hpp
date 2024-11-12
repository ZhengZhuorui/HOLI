#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_data_split_with_exponential_probe_perf(key_type* key, value_type* data, size_t num_keys){
    if constexpr (!traits::AllowDynamicDataNode){
        AEX_ERROR("index not allow dynamic data node!");
        return false;
    }
    else{
        AEX_HINT("[test SMO--data split ]");
        [[maybe_unused]]typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
        typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        typedef typename traits::slot_type slot_type;

        aex_tree<key_type, value_type, traits> tree;
        std::vector<key_type> key_buf;
        std::vector<data_node_ptr> data_node_buf;
        std::sort(key, key + num_keys);
        tree.split_with_exponential_probe(key, data, num_keys, key_buf, data_node_buf);
        AEX_IMPORTANT("split data node size=" << data_node_buf.size());
        size_t cnt = 0, ml_node_cnt = 0;
        std::vector<value_type> node_data;
        for (size_t i = 0; i < data_node_buf.size(); ++i){
            cnt += data_node_buf[i]->size;
            ml_node_cnt += IS_ML_NODE(data_node_buf[i]);
        }
        if (cnt != static_cast<size_t>(num_keys)){
            AEX_ERROR("Key number is wrong. num_keys=" << num_keys << ", data node key=" << cnt);
            return false;
        }
        cnt = 0;
        for (size_t i = 0; i < data_node_buf.size(); ++i){
            data_node_ptr inode = data_node_buf[i];
            for (slot_type j = 0; j < inode->size; ++j, ++cnt){
                if (static_cast<data_node_ptr>(inode)->key[j] != key[cnt]){
                    AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << static_cast<data_node_ptr>(inode)->key[j]);
                    return false;
                }
            }
        }
        AEX_SUCCESS("mechine learing node rate=" << 1.0 * ml_node_cnt / data_node_buf.size());
        AEX_SUCCESS("test success! Next test performance...");
        std::chrono::system_clock::time_point t1, t2;
        const int ITER = 10;
        double delta = 0;
        t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITER; ++i){
            key_buf.clear();
            data_node_buf.clear();
            tree.split_with_exponential_probe(key, data, num_keys, key_buf, data_node_buf);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta = duration_cast<microseconds>(t2 - t1).count();
        double NPS = 1.0 * 1e6 * num_keys * ITER / delta;
        AEX_SUCCESS("split time=" << delta << "ms, NPS=" << NPS);
        return true;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_SMO_data_split_with_linear_probe_perf(key_type* key, value_type* data, size_t num_keys){
    if constexpr(!traits::AllowDynamicDataNode){
        AEX_ERROR("index not allow dynamic data node!");
        return false;
    }
    else{
        AEX_HINT("[test SMO--data split with linear probe]");
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr _data_node_ptr;
        [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
        typedef typename traits::slot_type slot_type;

        aex_tree<key_type, value_type, traits> tree;
        std::vector<key_type> key_buf;
        std::vector<data_node_ptr> data_node_buf;
        std::sort(key, key + num_keys);
        tree.split_with_linear_probe(key, data, num_keys, key_buf, data_node_buf);
        AEX_IMPORTANT("split data node size=" << data_node_buf.size());
        size_t cnt = 0, ml_node_cnt = 0;
        std::vector<value_type> node_data;
        for (size_t i = 0; i < data_node_buf.size(); ++i){
            cnt += data_node_buf[i]->size;
            ml_node_cnt += IS_ML_NODE(data_node_buf[i]);
        }
        if (cnt != num_keys){
            AEX_ERROR("Key number is wrong. num_keys=" << num_keys << "data node key=" << cnt);
            return false;
        }
        cnt = 0;
        for (size_t i = 0; i < data_node_buf.size(); ++i){
            data_node_ptr inode = data_node_buf[i];
            for (slot_type j = 0; j < inode->size; ++j, ++cnt){
                if (inode->key[j] != key[cnt]){
                    AEX_ERROR("Key Error! key[" << cnt << "]=" << key[cnt] << "data node key=" << inode->key[j]);
                    return false;
                }
            }
            //for (slot_type j = 0; j < inode->size; ++j)
            //if (IS_ML_NODE(inode)){
            //    if (std::abs(j - static_cast<data_node_ptr>(inode)->predict(static_cast<data_node_ptr>(inode)->key[j])) > traits::ERROR_BOUND){
            //        AEX_ERROR("Model Error! j=" << j <<  "pred_pos=" << static_cast<data_node_ptr>(inode)->predict(static_cast<data_node_ptr>(inode)->key[j]) << "key[" << j << "]=" << key[j] << "size=" << inode->size);
            //        return false;
            //    }
            //}
        }

        AEX_SUCCESS("mechine learing node rate=" << 1.0 * ml_node_cnt / data_node_buf.size());
        AEX_SUCCESS("test success! Next test performance...");
        std::chrono::system_clock::time_point t1, t2;
        const int ITER = 10;
        double delta = 0;
        t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITER; ++i){
            key_buf.clear();
            data_node_buf.clear();
            tree.split_with_linear_probe(key, data, num_keys, key_buf, data_node_buf);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta = duration_cast<microseconds>(t2 - t1).count();
        double NPS = 1.0 * 1e6 * num_keys * ITER / delta;
        AEX_SUCCESS("split time=" << delta << "ms, NPS=" << NPS);
        return true;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
bool test_SMO_node_split_perf(key_type* key, size_t num_keys){
    return false;
}