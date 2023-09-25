#include <bits/stdc++.h>

#ifndef AEX_DEBUG
#define AEX_DEBUG
#endif

#ifndef AEX_EXPERIMENT
#define AEX_EXPERIMENT
#endif
#include "aex/aex_map.h"
#include "benchmark/generate_dataset.h"
#include "test/test.h"
#include "benchmark/utils.h"
#include "aex/aex_traits.h"

typedef long long LL;
using std::string;
using std::map;
const int N = 10000000, M = 10000;

template <typename T>
bool test(map<string, string> &flags){
    auto unit = flags["unit"];
    auto dataset = flags["dataset"];
    
    string file_name = flags["input_file"];
    FILE* file = fopen(file_name.c_str(), "rb");
    long long num_keys = stoll(flags["num_keys"]);

    vector<T> bin_data;
    read_bineary_file<T>(file, bin_data, num_keys);
    if (unit == "function"){
        auto func = flags["function"];
        if (func == "exp_lower_bound")
            return test_exponential_search_lower_bound(bin_data.data(), num_keys);
        if (func == "exp_upper_bound")
            return test_exponential_search_upper_bound(bin_data.data(), num_keys);
        if (func == "search_perf")
            return test_search_perf(bin_data.data(), num_keys);
        if (func == "search_with_error_bound_perf")
            return test_search_with_error_bound_perf(bin_data.data(), num_keys);
        if (func == "linear_probe")
            return test_linear_probe<T, T, aex_default_traits<T, T> >(bin_data.data(), num_keys);    
    }
    else if (unit == "model"){
        auto model_type = flags["model_type"];
        bool spec_flag = false;
        if (flags.find("spec")!= flags.end())
            if (flags["spec"] == "1") spec_flag = true;
        std::sort(bin_data.data(), bin_data.data() + num_keys);
        if (model_type == "linear")
            return test_linear_model(bin_data.data(), num_keys, spec_flag);
        else if (model_type == "exp")
            return test_exp_model(bin_data.data(), num_keys, spec_flag);
        else if (model_type == "log")
            return test_log_model(bin_data.data(), num_keys, spec_flag);
        else if (model_type == "quad")
            return test_quad_model(bin_data.data(), num_keys, spec_flag);
        else if (model_type == "gap_linear")
            return test_gap_array_linear_model(bin_data.data(), num_keys, spec_flag);
        else if (model_type == "piecewise_linear")
            return test_piecewise_linear_model(bin_data.data(), num_keys);
        else if (model_type == "all")
            return test_aex_model(bin_data.data(), num_keys, spec_flag);
    }
    else if (unit == "node"){
        auto node_type = flags["node_type"];
        auto func = flags["function"];
        long long batch = stoll(flags["batch"]);
        std::sort(bin_data.data(), bin_data.data() + num_keys);
        //vector<T> data(num_keys);
        if (node_type == "inner_node"){
            int level = 1;
            if (flags.find("level") != flags.end())
                level = stoi(flags["level"]);
            if (func == "insert")
                return test_inner_node_insert_perf<T, T>(bin_data, num_keys, batch, level);
            if (func == "erase")
                return test_inner_node_erase_perf<T, T>(bin_data, num_keys, batch, level);
            if (func == "query")
                return test_inner_node_query_perf<T, T>(bin_data, num_keys, batch, level);
            if (func == "other")
                return test_inner_node_other<T, T>(bin_data, num_keys, batch, level);
        }
        else if (node_type == "data_node"){
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            if (func == "insert")
                return test_data_node_insert_perf<T, T>(data.data(), num_keys, batch);
            if (func == "erase")
                return test_data_node_erase_perf<T, T>(data.data(), num_keys, batch);
            if (func == "query")
                return test_data_node_query_perf<T, T>(data.data(), num_keys, batch);
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
        if (func == "data_split_with_exponential_probe") {
            std::vector<T> value(num_keys);
            for (int i = 0; i < num_keys; ++i)
                value[i] = i;            
            return test_SMO_data_split_with_exponential_probe_perf<T, T>(bin_data.data(), value.data(), num_keys);
        }
        if (func == "data_split_with_linear_probe"){
            std::vector<T> value(num_keys);
            for (int i = 0; i < num_keys; ++i)
                value[i] = i;            
            return test_SMO_data_split_with_linear_probe_perf<T, T>(bin_data.data(), value.data(), num_keys);
        }
        if (func == "node_split") {
            return test_SMO_node_split_perf<T, T>(bin_data.data(), num_keys);
        }
        if (func == "insert_split"){
            return false;
        }
        if (func == "insert_ascend"){
            return false;
        }
    }
    else if (unit == "index"){
        auto func = flags["function"];
        if (func == "bulk_load"){
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_bulk_load_perf<T, T>(data.data(), num_keys);
        }
        if (func == "insert"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_insert_perf<T, T>(data.data(), num_keys, batch);
        }
        if (func == "lookup"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_lookup_perf(data.data(), num_keys, batch);
        }
        if (func == "delta_lookup"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_delta_lookup_perf(data.data(), num_keys, batch);
        }
        if (func == "range_query"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_range_query_perf(data.data(), num_keys, batch);
        }
        if (func == "erase"){
            long long batch = stoll(flags["batch"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_erase_perf(data.data(), num_keys, batch);
        }
        if (func == "mix"){
            long long batch = stoll(flags["batch"]);
            double rw_ratio = stod(flags["read_ratio"]);
            std::vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index_mix_perf(data.data(), num_keys, batch, rw_ratio);
        }
        if (func == "demo"){
            vector<std::pair<T, T> > data;
            pack_KV_dataset(bin_data, data);
            return test_index(data.data(), data.size());
        }
        if (func == "tot"){
            vector<std::pair<T, T>> data;
            pack_KV_dataset(bin_data, data);
            double read_nums = stoll(flags["read_nums"]);
            double write_nums = stoll(flags["write_nums"]);
            double erase_nums = stoll(flags["erase_nums"]);
            AEX_ASSERT(write_nums <= num_keys);
            AEX_ASSERT(erase_nums <= num_keys - write_nums);
            return test_index_total_perf(data.data(), num_keys, read_nums, write_nums, erase_nums);
        }
    }
    else if (unit == "con_index"){
        
    }
    return false;
}

/*
 * Required flags:
 * unit ("index", "function", "model", "SMO_xxx")
 * key_type: int or float
 * input_file
 * optional flags:
 * func: (if unit==function)
 * 
 */

int main(int argc, char** argv){
    srand(0);
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];
    bool test_result = false;
    if (key_type == "int"){
        test_result = test<long long>(flags);
    }
    else if (key_type == "float"){
        test_result = test<double>(flags);
    }

    if (test_result == false)
        AEX_ERROR("test failed.\n");
    else
        AEX_SUCCESS("test successed.\n");
    
}