#include <bits/stdc++.h>
#ifdef DEBUG
#define AEX_DEBUG
#endif
//#define AEX_DEBUG

bool file_is_head;
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
    else if (func == "construct"){
        long long num_keys = stoll(flags["num_keys"]);
        string index_name = flags["index"];
        std::cout << "?" << std::endl;
        benchmark_build<key_type, value_type>(file, num_keys, index_name);
    }
    else if (func == "erase"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = stoll(flags["num_ops"]);
        string index_name = flags["index"];
        benchmark_erase<key_type, value_type>(file, num_keys, num_ops, index_name);
    }
    else if (func == "range_query"){
        long long num_keys = stoll(flags["num_keys"]);
        string index_name = flags["index"];
        double length_ratio = stod(flags["length"]);
        long long num_ops = stoll(flags["num_ops"]);
        benchmark_range_query<key_type, value_type>(file, num_keys, num_ops, length_ratio, index_name);
    }
    else if (func == "delta_lookup"){
        long long num_keys = stoll(flags["num_keys"]);
        long long num_ops = 0;
        if (flags.find("num_ops") != flags.end())
            num_ops = stoll(flags["num_ops"]);
        string index_name = flags["index"];
        string query_dis = "uniform";
        if (flags.find("query_dis") != flags.end())
            query_dis = flags["query_dis"];
        benchmark_delta_lookup<key_type, value_type>(file, num_keys, num_ops, index_name, query_dis);
    }
}

#include <sys/time.h>
#include <unistd.h>

int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    AEX_PRINT("[Debug Mode]");
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];

    //double write_ratio = 0.5;
    //if (flags.find("write_ratio") != flags.end())
    //    write_ratio = stod(flags["write_ratio"]);
    std::string file_name = flags["input_file"];
    file_is_head = !((file_name.find(".bin.data") != std::string::npos) || (file_name.find("generate_data") != std::string::npos));
    if (key_type == "uint64") benchmark<unsigned long long, unsigned long long>(flags);
    //else if (key_type == "uint") benchmark<unsigned long long, unsigned long long>(flags);
    else if (key_type == "float64") benchmark<double, double>(flags);
    else if (key_type == "uint32") benchmark<unsigned int, unsigned int>(flags);
    else if (key_type == "int64") benchmark<long long, long long>(flags);
    else if (key_type == "float32") benchmark<float, float>(flags);
    else if (key_type == "int32") benchmark<int, int>(flags);
    std::cout << "benchmark finish..." << std::endl;
}