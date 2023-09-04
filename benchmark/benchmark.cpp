#include <bits/stdc++.h>

#include "benchmark/benchmark.h"
#include "benchmark/utils.h"
#include "benchmark/generate_dataset.h"

using std::string;
/*
 * Required flags:
 * input_file
 * key_type
 * num_keys
 * index
 * function
 * 
 * Optional flags:
 * query_dis(query distribution)
 * batch_size
 * write_ratio
 */

template<typename key_type,
        typename value_type>
void benchmark_lookup(FILE* file, long long num_keys, long long num_ops, string &index_name, string &query_dis){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    read_bineary_file<key_type>(file, bin_data, num_keys);
    pack_KV_dataset(bin_data, data);
    vector<key_type> query;
    vector<value_type> answer;
    if (query_dis == "uniform")
        generate_query<key_type, value_type, std::uniform_int_distribution<long long> >(data, query, answer, num_ops);
    else if (query_dis == "zipfian")
        generate_query_zipf<key_type, value_type>(data, query, answer, num_ops);

    std::sort(data.begin(), data.end(), [](auto const &a, auto const &b){return a.first < b.first;});

    //int unique_num = std::unique(data.data(), date.data() + data.size()) - data.data();
    //AEX_ASSERT(unique_num == num_keys);

    if (index_name == "aex"){
        aex_query_bench(data, query, answer);
    }
    else if (index_name == "stl_map"){
        stlmap_query_bench(data, query, answer);
    }
    else if (index_name == "stx_btree"){
        stx_btree_query_bench(data, query, answer);
    }
    else if (index_name == "alex"){
        alex_query_bench(data, query, answer);
    }
    else if (index_name == "pgm"){
        pgm_query_bench(data, query, answer);
    }
    else if (index_name == "lipp"){
        lipp_query_bench(data, query, answer);
    }
    else if (index_name == "search"){
        bineary_search_query_bench(data, query, answer);
    }
}

template<typename key_type,
        typename value_type>
void benchmark_insert(FILE* file, long long num_keys, long long num_ops, string &index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    read_bineary_file<key_type>(file, bin_data, num_keys + num_ops);
    pack_KV_dataset(bin_data, data);

    vector<pair<key_type, value_type> > insert_data(num_ops);
    std::random_shuffle(data.data(), data.data() + num_keys + num_ops);
    copy(data.data() + num_keys, data.data() + num_keys + num_ops, insert_data.data());
    data.resize(num_keys);
    std::sort(data.data(), data.data() + num_keys);
    if (index_name == "aex"){
        aex_insert_bench(data, insert_data);
    }
    else if (index_name == "stl_map"){
        stlmap_insert_bench(data, insert_data);
    }
    else if (index_name == "stx_btree"){
        stx_btree_insert_bench(data, insert_data);
    }
    else if (index_name == "alex"){
        alex_insert_bench(data, insert_data);
    }
    else if (index_name == "pgm"){
        pgm_insert_bench(data, insert_data);
    }
    else if (index_name == "lipp"){

    }
}

template<typename key_type,
        typename value_type>
void benchmark_depth(FILE* file, long long num_keys, string &index_name){
    vector<key_type> bin_data;
    read_bineary_file<key_type>(file, bin_data, num_keys);
    vector<pair<key_type, value_type> > data;
    pack_KV_dataset(bin_data, data);
    if (index_name == "alex")
        benchmark_alex_depth<key_type, value_type>(data);
    else if (index_name == "aex"){
        
    }
}

template<typename key_type,
        typename value_type>
void benchmark_build(FILE* file, long long num_keys, string &index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    read_bineary_file<key_type>(file, bin_data, num_keys);
    pack_KV_dataset(bin_data, data);
    std::sort(data.begin(), data.end());
    if (index_name == "aex"){
        aex_build_bench(data);
    }
    else if (index_name == "stl_map"){
        stlmap_build_bench(data);
    }
    else if (index_name == "stx_btree"){
        stx_btree_build_bench(data);
    }
    else if (index_name == "alex"){
        alex_build_bench(data);
    }
}

template<typename key_type,
        typename value_type>
void benchmark(FILE* file, long long num_keys, long long num_ops, string &index_name, string &func, string &query_dis){
    if (func == "lookup")
        benchmark_lookup<key_type, value_type>(file, num_keys, num_ops, index_name, query_dis);
    else if (func == "insert")
        benchmark_insert<key_type, value_type>(file, num_keys, num_ops, index_name);
    else if (func == "depth")
        benchmark_depth<key_type, value_type>(file, num_keys, index_name);

    else if (func == "build"){
        benchmark_build<key_type, value_type>(file, num_keys, index_name);
    }
    else if (func == "erase"){
        //benchmark_erase<key_type, value_type>(file, num_keys);
    }
}


#include <sys/time.h>
#include <unistd.h>

int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];
    auto input_files = flags["input_file"];
    FILE *file = fopen(input_files.c_str(), "rb");
    auto func = flags["function"];
    long long num_keys = stoll(flags["num_keys"]);
    long long num_ops = 0;
    if (flags.find("num_ops") != flags.end())
        num_ops = stoll(flags["num_ops"]);
    string index_name = flags["index"];
    string query_dis = "uniform";
    if (flags.find("query_dis") != flags.end())
        query_dis = flags["query_dis"];

    //double write_ratio = 0.5;
    //if (flags.find("write_ratio") != flags.end())
    //    write_ratio = stod(flags["write_ratio"]);

    if (key_type == "int") benchmark<unsigned long long, unsigned long long>(file, num_keys, num_ops, index_name, func, query_dis);
    else benchmark<double, double>(file, num_keys, num_ops, index_name, func, query_dis);
}