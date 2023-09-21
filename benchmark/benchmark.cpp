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
//void benchmark(FILE* file, long long num_keys, long long num_ops, string &index_name, string &func, string &query_dis, ){
void benchmark(std::map<string, string> flags){
    auto input_files = flags["input_file"];
    FILE *file = fopen(input_files.c_str(), "rb");
    auto func = flags["function"];

    if (func == "lookup"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = 0;
        if (flags.find("num_ops") != flags.end())
            num_ops = stoll(flags["num_ops"]);
        string index_name = flags["index"];
        string query_dis = "uniform";
        if (flags.find("query_dis") != flags.end())
            query_dis = flags["query_dis"];
        benchmark_lookup<key_type, value_type>(file, num_keys, num_ops, index_name, query_dis);
    }
    else if (func == "insert"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = 0;
        if (flags.find("num_ops") != flags.end())
            num_ops = stoll(flags["num_ops"]);
        string index_name = flags["index"];
        benchmark_insert<key_type, value_type>(file, num_keys, num_ops, index_name);
    }
    else if (func == "mix"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = stoll(flags["num_ops"]);
        long long read_ratio = stod(flags["read_ratio"]);
        string index_name = flags["index"];
        string query_dis = flags["query_dis"];
        benchmark_mix<key_type, value_type>(file, num_keys, num_ops, read_ratio, index_name, query_dis);
    }
    else if (func == "build"){
        long long num_keys = stoll(flags["num_keys"]);
        string index_name = flags["index"];
        benchmark_build<key_type, value_type>(file, num_keys, index_name);
    }
    else if (func == "erase"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = stoll(flags["num_keys"]);
        string index_name = flags["index"];
        benchmark_erase<key_type, value_type>(file, num_keys, num_ops, index_name);
    }
    else if (func == "range_query"){
        long long num_keys = stoll(flags["num_keys"]);
        string index_name = flags["index"];
        long long length_ratio = stod(flags["length_ratio"]);
        long long num_ops = stoll(flags["num_ops"]);
        benchmark_range_query<key_type, value_type>(file, num_keys, num_ops, length_ratio, index_name);
    }
}

#include <sys/time.h>
#include <unistd.h>

int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];

    //double write_ratio = 0.5;
    //if (flags.find("write_ratio") != flags.end())
    //    write_ratio = stod(flags["write_ratio"]);

    if (key_type == "int") benchmark<unsigned long long, unsigned long long>(flags);
    else benchmark<double, double>(flags);
}