#include <bits/stdc++.h>

#ifndef AEX_DEBUG
#define AEX_DEBUG
#endif

#ifndef AEX_DEBUG_ASSERT
#define AEX_DEBUG_ASSERT
#endif

//using namespace aex;
#include "aex.h"
using namespace aex;
#include "benchmark/generate_dataset.h"
#include "test/test.h"
#include "benchmark/utils.h"
#include "aex_traits.h"

typedef long long LL;
typedef unsigned long long ULL;
using std::string;
using std::map;
const int N = 10000000, M = 10000;

template <typename T, bool AllowMultiKey, bool AllowConcurrency>
bool test(map<string, string> &flags){
    
    auto unit = flags["unit"];
    using default_traits = aex_default_traits<T, T, AllowMultiKey, void, AllowConcurrency>;
    AEX_HINT("unit test: AllowMultiKey=" << AllowMultiKey << ", AllowConcurrency=" << AllowConcurrency);

    if (unit == "avx"){
        auto func = flags["function"];
        if (func == "lower_bound_with_error_bound")
            return test_lower_bound_with_error_bound_avx<T>();
        return false;
    }
    
auto dataset = flags["dataset"];
    
    string file_name = flags["input_file"];
    FILE* file = fopen(file_name.c_str(), "rb");
    long long num_keys = stoll(flags["num_keys"]);

    vector<T> bin_data;
    //bool is_head = (file_name.find("fb_200M_uint64") != std::string::npos) | 
    //               (file_name.find("osm_cellids_200M_uint64") != std::string::npos) | 
    //               (file_name.find("wiki_ts_200M_uint64") != std::string::npos) | 
    //               (file_name.find("normal_200M_uint64") != std::string::npos) | 
    //               (file_name.find("lognormal_200M_uint64") != std::string::npos) | 
    //               (file_name.find("books_800M_uint64") != std::string::npos);
    bool is_head = !((file_name.find(".bin.data") != std::string::npos) || (file_name.find("generate_data") != std::string::npos));
    AEX_PRINT("is_head=" << is_head);
    size_t _ = read_bineary_file<T>(file, bin_data, num_keys, is_head);
    assert((long long)_ == num_keys);
    std::cout << "Data Example: " << bin_data[0] << ", " << bin_data[num_keys / 2] << ", " << bin_data[num_keys - 1] << std::endl;
    bool is_sorted_flag = is_sorted(bin_data.data(), num_keys);
    AEX_PRINT("Is sorted?: " << is_sorted_flag);
    long long unique_keys = std::unique(bin_data.data(), bin_data.data() + num_keys) - bin_data.data();
    //bool multikey_flag = (num_keys != unique_keys);
    std::cout << "Is unique? " << (num_keys == unique_keys ? "Yes" : "No") << ", unique_keys=" << unique_keys << std::endl;

#ifndef AEX_DEBUG_THREAD
#define AEX_DEBUG_THREAD
#endif

    if (unit == "sort"){
        std::string output_file = flags["output_file"];
        AEX_HINT("sort file: " << file_name << " to file:" << output_file);
        FILE* file = fopen(output_file.c_str(), "wb");
        std::sort(bin_data.data(), bin_data.data() + num_keys);
        write_bineary_file(file, bin_data, num_keys);
        AEX_SUCCESS("file is sorted");
        return true;
    }
    else if (unit == "function"){
        auto func = flags["function"];
        if (func == "exp_lower_bound")
            return test_exponential_search_lower_bound(bin_data.data(), num_keys);
        else if (func == "exp_upper_bound")
            return test_exponential_search_upper_bound(bin_data.data(), num_keys);
        else if (func == "search_perf")
            return test_search_perf(bin_data.data(), num_keys);
        else if (func == "search_with_error_bound_perf")
            return test_search_with_error_bound_perf(bin_data.data(), num_keys);
        //else if (func == "linear_probe")
        //    return test_linear_probe<T, T, default_traits >(bin_data.data(), num_keys);    
    }
    else if (unit == "model"){
        auto model_type = flags["model_type"];
        [[maybe_unused]]bool spec_flag = false;
        if (flags.find("spec")!= flags.end())
            if (flags["spec"] == "1") spec_flag = true;
        std::sort(bin_data.data(), bin_data.data() + num_keys);
        return test_model<T, default_traits>(bin_data.data(), num_keys);
        //if (model_type == "linear")
        //    return test_linear_model(bin_data.data(), num_keys, spec_flag);
        //else if (model_type == "exp")
        //    return test_exp_model(bin_data.data(), num_keys, spec_flag);
        //else if (model_type == "log")
        //    return test_log_model(bin_data.data(), num_keys, spec_flag);
        //else if (model_type == "quad")
        //    return test_quad_model(bin_data.data(), num_keys, spec_flag);
        //else if (model_type == "gap_linear")
        //    return test_gap_array_linear_model(bin_data.data(), num_keys, spec_flag);
        //else if (model_type == "piecewise_linear"){
        //    int level = stoi(flags["level"]);
        //    return test_model<T, piecewise_linear_model<T, default_traits > >(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "piecewise_linear_2"){
        //    int level = stoi(flags["level"]);
        //    return test_model<T, piecewise_linear_model_2<T, default_traits > >(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "piecewise_linear_3"){
        //    int level = stoi(flags["level"]);
        //    return test_model<T, piecewise_linear_model_3<T, default_traits > >(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "PDM"){
        //    int level = stoi(flags["level"]);
        //    return test_model<T, PDM<T, default_traits > >(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "PDM_avx"){
        //    int level = stoi(flags["level"]);
        //    using PDM_avx = PDM_AVX<T, PDM<T, default_traits>, default_traits>;
        //    return test_model<T, PDM_avx>(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "PDM_hash_table"){
        //    int level = stoi(flags["level"]);
        //    //using PDM_avx = PDM_AVX<T, PDM<T, default_traits>, default_traits>;
        //    return test_model_hash_table<T, PDM_hash_table<T, default_traits>>(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "linear_hash_table"){
        //    int level = stoi(flags["level"]);
        //    //using PDM_avx = PDM_AVX<T, PDM<T, default_traits>, default_traits>;
        //    return test_model_hash_table<T, gap_array_linear_model_hash_table<T, default_traits>>(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "PDM_hash_table_AVX"){
        //    int level = stoi(flags["level"]);
        //    using PDM_hash_table_avx = PDM_hash_table_AVX<T, PDM_hash_table<T, default_traits>, default_traits>;
        //    return test_model_hash_table<T, PDM_hash_table_avx>(bin_data.data(), num_keys, level);
        //}

        //else if (model_type == "PDM_avx"){
        //    int level = stoi(flags["level"]);
        //    //return test_PDM_AVX_perf(bin_data.data(), num_keys, batch, level);
        //    using PDM_avx = PDM_AVX<T, piecewise_linear_model<T, default_traits>, default_traits>;
        //    return test_model<T, PDM_avx>(bin_data.data(), num_keys, level);
        //}
        //else if (model_type == "all")
        //    return test_aex_model(bin_data.data(), num_keys, spec_flag);
    }
    else if (unit == "node"){
        auto node_type = flags["node_type"];
        auto func = flags["function"];
        long long batch = stoll(flags["batch"]);
        std::sort(bin_data.data(), bin_data.data() + num_keys);
        //vector<T> data(num_keys);
        if (node_type == "hash_node"){
            //int level = 1;
            //if (flags.find("level") != flags.end())
            //    level = stoi(flags["level"]);
            if (func == "insert")
                return test_hash_node_insert_perf<T, T, default_traits>(bin_data, num_keys, batch);
            if (func == "erase")
                return test_hash_node_erase_perf<T, T, default_traits>(bin_data, num_keys, batch);
            if (func == "query")
                return test_hash_node_query_perf<T, T, default_traits>(bin_data, num_keys, batch);
            //if (func == "other")
            //    return test_hash_node_other<T, T, default_traits>(bin_data, num_keys, batch);
        }
        else if (node_type == "dense_node"){

        }
        else if (node_type == "data_node"){
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            if (func == "insert")
                return test_data_node_insert_perf<T, T, default_traits>(data.data(), num_keys, batch);
            if (func == "erase")
                return test_data_node_erase_perf<T, T, default_traits>(data.data(), num_keys, batch);
            if (func == "query")
                return test_data_node_query_perf<T, T, default_traits>(data.data(), num_keys, batch);
            //if (func == "other")
            //    return test_data_node_other<T, T, aex::test_traits<T, T>>(bin_data, num_keys, batch);
        }

    }
    else if (unit == "balance"){
        auto func = flags["function"];
        if (func == "insert_merge"){}
        if (func == "split"){}
    }
    else if (unit == "SMO"){
        auto func = flags["function"];
        //if (func == "data_split_with_exponential_probe") {
        //    std::vector<T> value(num_keys);
        //    for (int i = 0; i < num_keys; ++i)
        //        value[i] = i;            
        //    return test_SMO_data_split_with_exponential_probe_perf<T, T>(bin_data.data(), value.data(), num_keys);
        //}
        //if (func == "data_split_with_linear_probe"){
        //    std::vector<T> value(num_keys);
        //    for (int i = 0; i < num_keys; ++i)
        //        value[i] = i;            
        //    return test_SMO_data_split_with_linear_probe_perf<T, T>(bin_data.data(), value.data(), num_keys);
        //}
        //if (func == "node_split") {
        //    return test_SMO_node_split_perf<T, T>(bin_data.data(), num_keys);
        //}
        //if (func == "node_rescale"){
        //    double ratio = stod(flags["ratio"]);
        //    int level = stoi(flags["level"]);
        //    return test_SMO_node_rescale_perf<T, T>(bin_data.data(), num_keys, ratio, level);
        //}
        //if (func == "insert_split"){
        //    return false;
        //}
        //if (func == "insert_ascend"){
        //    return false;
        //}

    }
    else if (unit == "index"){
        auto func = flags["function"];
        //bool multikey_flag = (flags.find("multikey") != flags.end());
        if (func == "bulk_load"){
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            bool res = test_index_bulk_load_perf<T, T, aex::aex_default_traits<T, T, AllowMultiKey, void, false>>(data.data(), num_keys);
            return res;
        }
        if (func == "insert"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_insert_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "lookup"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_lookup_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "delta_lookup"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_delta_lookup_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "insert_hotspot"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_insert_hotspot_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "range_query"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_range_query_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "erase"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_erase_perf<T, T, default_traits>(data.data(), num_keys, batch);
        }
        if (func == "mix"){
            long long batch = stoll(flags["batch"]);
            double rw_ratio = stod(flags["read_ratio"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index_mix_perf<T, T, default_traits>(data.data(), num_keys, batch, rw_ratio);
        }
        if (func == "demo"){
            vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            return test_index<T, T, default_traits>(data.data(), data.size());
        }
        if (func == "tot"){
            vector<std::pair<T, T>> data;
            pack_KV_dataset(bin_data, data, is_sorted_flag);
            long long read_nums = stoll(flags["read_nums"]);
            long long write_nums = stoll(flags["write_nums"]);
            long long erase_nums = stoll(flags["erase_nums"]);
            AEX_ASSERT(write_nums <= num_keys);
            AEX_ASSERT(erase_nums <= num_keys - write_nums);
            return test_index_total_perf<T, T, default_traits>(data.data(), num_keys, read_nums, write_nums, erase_nums);
        }
    }
    else if (unit == "hash_table"){
        long long thread_num = stoll(flags["thread_num"]);
        return test_hash_table_perf<T, default_traits>(bin_data.data(), num_keys, thread_num);
    }
#undef AEX_DEBUG_THREAD
    else if (unit == "index_con"){
        auto func = flags["function"];
        int thread_num = std::stoi(flags["thread_num"]);
        if (func == "tot"){
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            long long read_nums = stoll(flags["read_nums"]);
            long long write_nums = stoll(flags["write_nums"]);
            //long long erase_nums = stoll(flags["erase_nums"]);
            long long erase_nums = 0;
            AEX_ASSERT(write_nums <= num_keys);
            AEX_ASSERT(erase_nums <= num_keys - write_nums);
            return test_index_total_con_perf(data.data(), num_keys, thread_num, read_nums, write_nums, erase_nums);
        }
    }
    return false;
}

template<bool AllowMultiKey, bool AllowConcurrency>
void test_con(map<string, string> &flags){
    auto key_type = flags["key_type"];
    bool test_result = false;
    if (key_type == "uint64"){
        test_result = test<unsigned long long, AllowMultiKey, AllowConcurrency>(flags);
    }
    else if (key_type == "float64"){
        test_result = test<double, AllowMultiKey, AllowConcurrency>(flags);
    }
    else if (key_type == "int64"){
        test_result = test<long long, AllowMultiKey, AllowConcurrency>(flags);
    }
    else if (key_type == "float32"){
        test_result = test<float, AllowMultiKey, AllowConcurrency>(flags);
    }
    else if (key_type == "uint32"){
        test_result = test<unsigned int, AllowMultiKey, AllowConcurrency>(flags);
    }
    else if (key_type == "int32"){
        test_result = test<int, AllowMultiKey, AllowConcurrency>(flags);
    }

    if (test_result == false)
        AEX_ERROR("test failed.\n");
    else
        AEX_SUCCESS("test successed.\n");
}

template<bool AllowMultiKey>
void test_mul_key(map<string, string> &flags){
    bool allow_concurrency = (flags.find("con") != flags.end());
    if (allow_concurrency)
        test_con<AllowMultiKey, true>(flags);
    else
        test_con<AllowMultiKey, false>(flags);
}

/*
 * Required flags:
 * unit ("index", "function", "model", "SMO_xxx")
 * key_type: int or float
 * input_file
 * optional flags:
 * func: (if unit==function)
 * con: allow concurrency
 * 
 */

int main(int argc, char** argv){
    AEX_PRINT("Debug Mode On");
    #ifdef AEX_DEBUG_THREAD
    AEX_PRINT("Debug Lock Mode On");
    #endif
    srand(0);
    auto flags = parse_flags(argc, argv);
    if (flags.find("general") != flags.end()){
        AEX_HINT("<ULL, ULL> unconcurrency data node size=" << sizeof(aex::aex_static_data_node<ULL, ULL, aex_default_traits<ULL, ULL, false, void, false>>));
        AEX_HINT("<ULL, ULL> concurrency data node size=" << sizeof(aex::aex_static_data_node<ULL, ULL, aex_default_traits<ULL, ULL, false, void, true>>));
        AEX_HINT("<ULL, ULL> hash table block size=" << sizeof(aex::aex_hash_table_block<ULL, aex_default_traits<ULL, ULL, false, void, false>>));
        return 0;
    }
    else if (flags.find("rw_lock") != flags.end()){
        bool res = test_rw_lock();
        return res;
    }
    bool allow_multi_key = (flags.find("multikey") != flags.end());
    if (allow_multi_key)
        test_mul_key<true>(flags);
    else
        test_mul_key<false>(flags);
}