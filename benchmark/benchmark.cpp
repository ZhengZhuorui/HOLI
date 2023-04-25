#include <bits/stdc++.h>

#include "benchmark/benchmark.h"
#include "benchmark/utils.h"
#include "benchmark/generate_dataset.h"

#define TEST_ITER 10

using namespace std;
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
void benchmark_query(FILE* file, long long num_keys, long long num_ops, string index_name, string query_dis){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    read_bineary_file<key_type>(file, bin_data, num_keys);
    pack_KV_dataset(bin_data, data);
    vector<key_type> query;
    vector<value_type> answer;
    if (query_dis == "uniform")
        generate_query<key_type, value_type, std::uniform_int_distribution<long long> >(data, query, answer, num_ops);
    else
        generate_query_zipf<key_type, value_type>(data, query, answer, num_ops);

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
}

template<typename key_type,
        typename value_type>
void benchmark_insert(FILE* file, key_type num_keys, value_type num_ops, string index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    read_bineary_file<key_type>(file, bin_data, num_keys + num_ops);
    pack_KV_dataset(bin_data, data);

    vector<pair<key_type, value_type> > insert_data;
    insert_data.resize(num_ops);
    copy(data.begin() + data.size() - num_ops, data.end(), insert_data.begin());
    data.resize(num_ops);
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
}

template<typename key_type,
        typename value_type>
void benchmark(FILE* file, long long num_keys, long long num_ops, string index_name, string func, string query_dis){
    if (func == "query")
        benchmark_query<key_type, value_type>(file, num_keys, num_ops, index_name, query_dis);
    else if (func == "insert")
        benchmark_insert<key_type, value_type>(file, num_keys, num_ops, index_name);
}

int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    auto flags = parse_flags(argc, argv);
    auto data_type = flags["data_type"];
    auto input_files = flags["input_file"];
    FILE *file = fopen(input_files.c_str(), "rb");
    auto dataset = flags["dataset"];
    auto func = flags["function"];
    long long num_keys = stoll(flags["num_keys"]);
    long long num_ops = stoll(flags["num_ops"]);
    string index_name = flags["index"];
    string query_dis = "uniform";
    if (flags.find("query_dis") != flags.end())
        query_dis = flags["query_dis"];

    //double write_ratio = 0.5;
    //if (flags.find("write_ratio") != flags.end())
    //    write_ratio = stod(flags["write_ratio"]);
    

    if (data_type == "int") benchmark<long long, long long>(file, num_keys, num_ops, index_name, func, query_dis);
    else benchmark<double, double>(file, num_keys, num_ops, index_name, func, query_dis);
}