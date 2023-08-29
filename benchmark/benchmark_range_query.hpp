#include <bits/stdc++.h>
#include "aex/aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using namespace std::chrono;
using aex::aex_map;

template<typename key_type, typename value_type>
void aex_range_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    aex_map<key_type, value_type> index;
    index.bulk_load(data.data(), data.size());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("aex map query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        for (size_t i = 0; i < num_ops; ++i){
            const auto iter = index.find(query[i]);
            sum += iter.data();
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;

    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;

}

template<typename key_type, typename value_type>
void stlmap_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    std::map<key_type, value_type> index(data.begin(), data.end());
    vector<value_type> result(data.size());

    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t M = query.size();
    size_t times = 1;
    printf("stl map query test...\n");
    fflush(stdout);
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (const auto& x : query){
            const auto iter = index.find(x);
            sum += iter->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * M * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void stx_btree_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    stx::btree_map<key_type, value_type> index(data.begin(), data.end());
    vector<value_type> result(data.size());
    system_clock::time_point t1, t2;

    size_t num_ops = query.size();
    size_t times = 1;
    printf("stl map query test...");
    fflush(stdout);
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (const auto& x : query){
            const auto iter = index.find(x);
            sum += iter->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void alex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    printf("[alex query benchmark]\n");
    alex::Alex<key_type, value_type> index;
    std::cout << data.size() << std::endl;
    index.bulk_load(data.data(), data.size());

    [[maybe_unused]] size_t times = 1;
    std::cout << "alex query test..." << std::endl;
    [[maybe_unused]] value_type sum = 0;
    long long num_ops = query.size();
    auto t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        for (int i = 0; i < num_ops; ++i){
            auto iter = index.find(query[i]);
            sum += iter.payload();
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;

}

template<typename key_type, typename value_type>
void pgm_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        ++cnt;
        if (cnt % 10000 == 0) std::cout << "cnt=" << cnt << std::endl;
        index.insert_or_assign(x);
    }
    */
    
    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t num_ops = query.size();
    size_t times = 1;
    value_type sum = 0;
    printf("stl map query test...");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (const auto&x : query)
            sum += index.find(x)->second;
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void bineary_search_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t sz = data.size();
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("bineary search query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        for (size_t i = 0; i < num_ops; ++i){
            size_t pos = std::lower_bound(data.data(), data.data() + sz, std::pair<key_type, value_type>(query[i], -1)) - data.data();
            sum += data[pos].second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;

    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;

}