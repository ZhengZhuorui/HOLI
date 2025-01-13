#pragma once
#include <bits/stdc++.h>

#include "aex.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"
#include "utils.h"
#include "generate_dataset.h"

using namespace std::chrono;

template<typename key_type, typename value_type>
void aex_erase_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &erase_data){
    
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t M = erase_data.size();
    size_t times = 1;
    printf("aex erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        aex::aex_tree<key_type, value_type> index;
        index.bulk_load(data.data(), data.size());
        t1 = std::chrono::high_resolution_clock::now();
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    
    double QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void stlmap_erase_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &erase_data){
    
    system_clock::time_point t1, t2;

    size_t M = erase_data.size();
    size_t times = 1;
    printf("stl map erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        std::map<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }

    double QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void stx_btree_erase_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &erase_data){
    
    system_clock::time_point t1, t2;

    size_t M = erase_data.size();
    size_t times = 1;
    printf("stx btree erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        stx::btree_map<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double QPS = 1.0 * 1e6 * M * times / delta;
    std::cout << "M=" << M << "" << std::endl;
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void alex_erase_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &erase_data){
    
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t M = erase_data.size();
    size_t times = 1;
    printf("alex erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        alex::Alex<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void pgm_erase_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &erase_data){
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = erase_data.size();
    size_t times = 1;
    printf("pgm erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    double QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void lipp_erase_bench(vector<pair<key_type, value_type>> &data, vector<key_type> &erase_data){
    // system_clock::time_point t1, t2;
    // //size_t cnt = 0;
    // size_t M = erase_data.size();
    // size_t times = 1;
    // printf("lipp erase test...\n");
    // fflush(stdout);
    // long long delta = 0;
    // for (size_t i = 0; i < times; ++i){
    //     //pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
    //     LIPP<key_type, value_type> index;
    //     index.bulk_load(data.data(), data.size());
    //     t1 = std::chrono::high_resolution_clock::now();    
    //     for (const auto& x : erase_data){
    //         index.erase(x);
    //     }
    //     t2 = std::chrono::high_resolution_clock::now();
    //     delta += duration_cast<microseconds>(t2 - t1).count();
    // }
    // float QPS = 1.0 * 1e6 * M * times / delta;
    // 
    // printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    // fflush(stdout);
}

template<typename key_type, typename value_type>
void hash_erase_bench(vector<pair<key_type, value_type>> &data, vector<key_type> &erase_data){
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = erase_data.size();
    size_t times = 1;
    printf("hash erase test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        //pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        //LIPP<key_type, value_type> index
        std::unordered_map<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : erase_data){
            index.erase(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type,
        typename value_type>
void benchmark_erase(FILE* file, long long num_keys, long long num_ops, std::string &index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    assert((long long)_ == num_keys);
    std::sort(bin_data.data(), bin_data.data() + num_keys);
    num_keys = std::unique(bin_data.data(), bin_data.data() + num_keys) - bin_data.data();
    std::cout << "unique num_keys=" << num_keys << std::endl;
    pack_KV_dataset(bin_data, data);
    //vector<pair<key_type, value_type> > erase_data(num_ops);
    std::vector<key_type> erase_data(num_ops);
    std::random_shuffle(data.data(), data.data() + num_keys);
    for (long long i = 0; i < num_ops; ++i)
        erase_data[i] = data[i].first;
    std::cout << erase_data.size() << std::endl;
    std::sort(data.data(), data.data() + num_keys);
    if (index_name == "aex"){
        aex_erase_bench(data, erase_data);
    }
    else if (index_name == "stl_map"){
        stlmap_erase_bench(data, erase_data);
    }
    else if (index_name == "stx_btree"){
        stx_btree_erase_bench(data, erase_data);
    }
    else if (index_name == "alex"){
        alex_erase_bench(data, erase_data);
    }
    else if (index_name == "pgm"){
        pgm_erase_bench(data, erase_data);
    }
    else if (index_name == "lipp"){
        lipp_erase_bench(data, erase_data);
    }
    else if (index_name == "hash"){
        hash_erase_bench(data, erase_data);
    }
}