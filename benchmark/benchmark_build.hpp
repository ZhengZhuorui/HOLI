#include <bits/stdc++.h>
#include "aex.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using namespace std::chrono;

template<typename key_type, typename value_type>
void aex_build_bench(vector<pair<key_type, value_type> > &data){
    /*
    for (const auto& x : data){
        index.insert(x);
    }*/
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t times = 1;
    size_t sum = 0;
    printf("aex map construct test...");
    t1 = std::chrono::high_resolution_clock::now();
    aex::aex_tree<key_type, value_type> index;
    for (size_t i = 0; i < times; ++i){
        index.bulk_load(data.data(), data.size());
        sum += index.size();
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1000000.0 * times / delta;

    std::cout << "code=" << sum << "used time=" << delta <<  " ms, QPS=" << OPS << std::endl;
    index.print_stats();
}

template<typename key_type, typename value_type>
void stlmap_build_bench(vector<pair<key_type, value_type> > &data){
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t times = 1;
    size_t sum = 0;
    printf("aex map construct test...");
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        std::map<key_type, value_type> mp(data.begin(), data.end());
        sum += mp.size();
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1000000.0 * times / delta;

    std::cout << "code=" << sum << "used time=" << delta <<  " ms, QPS=" << OPS << std::endl;
}

template<typename key_type, typename value_type>
void stx_btree_build_bench(vector<pair<key_type, value_type> > &data){
    
    stx::btree_map<key_type, value_type> index(data.begin(), data.end());
    vector<value_type> result(data.size());
    system_clock::time_point t1, t2;

    size_t times = 1;
    printf("stx btree build test...");
    fflush(stdout);
    size_t sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        stx::btree_map<key_type, value_type> index;
        index.bulk_load(data.begin(), data.end());
        sum += index.size();
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void alex_build_bench(vector<pair<key_type, value_type> > &data){
    printf("[alex build benchmark]\n");
    [[maybe_unused]] size_t times = 1;
    size_t sum = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        alex::Alex<key_type, value_type> index;
        index.bulk_load(data.data(), data.size());
        sum += index.size();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void pgm_build_bench(vector<pair<key_type, value_type> > &data){
    printf("[pgm build benchmark]\n");
    [[maybe_unused]] size_t times = 1;
    size_t sum = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        sum += index.size();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1000000.0 * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void lipp_build_bench(vector<pair<key_type, value_type> > &data){
    printf("[lipp build benchmark]\n");
    [[maybe_unused]] size_t times = 1;
    size_t sum = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        alex::Alex<key_type, value_type> index;
        index.bulk_load(data.data(), data.size());
        sum += index.size();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1000000.0 * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}


template<typename key_type,
        typename value_type>
void benchmark_build(FILE* file, long long num_keys, std::string &index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    assert((long long)_ == num_keys);
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
    else if (index_name == "pgm"){
        pgm_build_bench(data);
    }
}