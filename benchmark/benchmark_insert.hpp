#pragma once
#include <bits/stdc++.h>

#include "aex.h"
#include "stx/btree_map.h"
//#include "alex_map.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"
#include "utils.h"
#include "generate_dataset.h"

using namespace std::chrono;

template<typename key_type, typename value_type>
void aex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    aex::aex_tree<key_type, value_type> index;
    size_t M = insert_data.size();
    size_t times = 1;
    printf("aex insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t T = 0; T < times; ++T){
        index.bulk_load(data.data(), data.size());
        t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < insert_data.size(); ++i){
            index.insert(insert_data[i]);
        }
        //for (const auto& x : insert_data){
        //    index.insert(x);
        //}
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void stlmap_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > & insert_data){
    
    system_clock::time_point t1, t2;

    size_t M = insert_data.size();
    size_t times = 1;
    printf("stl map insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        std::map<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1000000.0 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void stx_btree_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    size_t M = insert_data.size();
    size_t times = 1;
    printf("stx btree insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        stx::btree_map<key_type, value_type> index;
        index.bulk_load(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void alex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t M = insert_data.size();
    size_t times = 1;
    alex::Alex<key_type, value_type> index;
    index.bulk_load(data.data(), data.size());
    printf("alex insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void pgm_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    assert(std::is_integral<key_type>::value==true);
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = insert_data.size();
    size_t times = 1;
    printf("pgm insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert_or_assign(x.first, x.second);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void lipp_insert_bench(vector<pair<key_type, value_type>> &data, vector<pair<key_type, value_type>> &insert_data){
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = insert_data.size();
    size_t times = 1;
    printf("lipp insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        //pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        LIPP<key_type, value_type> index;
        index.bulk_load(data.data(), data.size());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x.first, x.second);
        }
        t2 = std::chrono::high_resolution_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1.0 * 1e6 * M * times / delta;
    
    printf("used time=%lld ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template<typename key_type, typename value_type>
void hash_insert_bench(vector<pair<key_type, value_type>> &data, vector<pair<key_type, value_type>> &insert_data){
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = insert_data.size();
    size_t times = 1;
    printf("hash insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_t i = 0; i < times; ++i){
        //pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        //LIPP<key_type, value_type> index
        std::unordered_map<key_type, value_type> index(data.begin(), data.end());
        t1 = std::chrono::high_resolution_clock::now();    
        for (const auto& x : insert_data){
            index.insert(std::make_pair(x.first, x.second));
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
void benchmark_insert(FILE* file, long long num_keys, long long num_ops, std::string &index_name){
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    assert((long long)_ == num_keys);
    pack_KV_dataset(bin_data, data);

    vector<pair<key_type, value_type> > insert_data(num_ops);
    std::random_shuffle(data.data(), data.data() + num_keys);
    copy(data.data() + num_keys - num_ops, data.data() + num_keys, insert_data.data());
    data.resize(num_keys - num_ops);
    std::sort(data.data(), data.data() + num_keys - num_ops);
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
        lipp_insert_bench(data, insert_data);
    }
    else if (index_name == "hash"){
        hash_insert_bench(data, insert_data);
    }
}