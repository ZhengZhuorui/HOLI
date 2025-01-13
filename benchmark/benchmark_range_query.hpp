#include <bits/stdc++.h>
#include "aex.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using aex::aex_tree;
using namespace std::chrono;

template<typename key_type, typename value_type>
void aex_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type> > &query){
    aex_tree<key_type, value_type, aex::aex_default_traits<key_type, value_type, false, void, false, false> > index;
    index.bulk_load(data.data(), data.size());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    printf("aex map range query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        for (auto x : query){
            auto iter = index.lower_bound(x.first);
            //for (int i = 0; i < 100; ++i, ++iter)
            //    sum += iter.data();
            while (iter != index.end() && iter.key() <= x.second){
                sum += iter.data();
                ++iter;
            }
        }
    }

    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;

    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;

}

template<typename key_type, typename value_type>
void stlmap_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type>> &query){
    
    std::map<key_type, value_type> index(data.begin(), data.end());
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t M = query.size();
    size_t times = 1;
    printf("stl map range query test...\n");
    fflush(stdout);
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (auto& x : query){
            auto iter = index.find(x.first);
            while (iter != index.end() && iter->first <= x.second){
                sum += iter->second;
                ++iter;
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * M * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void stx_btree_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type> > &query){
    stx::btree_map<key_type, value_type> index(data.begin(), data.end());
    vector<value_type> result(data.size());
    system_clock::time_point t1, t2;

    size_t num_ops = query.size();
    size_t times = 1;
    printf("stx btree range query test...");
    fflush(stdout);
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (const auto& x : query){
            typename stx::btree_map<key_type, value_type>::const_iterator iter = index.lower_bound(x.first);
            while (iter != index.end() && iter.key() <= x.second){
                sum += iter->second;
                ++iter;
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void alex_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type> > &query){
    alex::Alex<key_type, value_type> index;
    std::cout << data.size() << std::endl;
    index.bulk_load(data.data(), data.size());

    [[maybe_unused]] size_t times = 1;
    [[maybe_unused]] value_type sum = 0;
    long long num_ops = query.size();
    printf("alex range query test...");
    fflush(stdout);
    auto t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        for (auto &x : query){
            auto iter = index.find(x.first);
            while (iter != index.end() && iter.key() <= x.second){
                sum += iter.payload();
                ++iter;
            }
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;

}

template<typename key_type, typename value_type>
void pgm_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type> > &query){
    //pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
    pgm::DynamicPGMIndex<key_type, value_type> index;
    for (auto &x : data)
        index.insert_or_assign(x.first, x.second);
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t num_ops = query.size();
    size_t times = 1;
    value_type sum = 0;
    printf("pgm range query test...");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (auto&x : query){
            auto result = index.range(x.first, x.second);
            for (auto[k, v] : result){
                sum += v;
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;
    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void dense_array_range_query_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, key_type> > &query){
    system_clock::time_point t1, t2;
    //size_t cnt = 0;
    size_t num_ops = query.size();
    size_t times = 1;
    value_type sum = 0;
    printf("pgm range query test...");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; ++i){
        for (auto&x : query){
            //auto iter = data.lower(x.first);
            auto iter = std::lower_bound(data.begin(), data.end(), x.first, [](auto x, auto y){return x.first < y;});
            while (iter != data.end() && iter->first < x.second){
                sum += iter->second;
                ++iter;
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;    
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void benchmark_range_query(FILE* file, long long num_keys, long long num_ops, int length, std::string index_name){ //file, num_keys, num_ops, length_ratio
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    assert((long long)_ == num_keys);
    std::sort(bin_data.data(), bin_data.data() + num_keys);
    num_keys = std::unique(bin_data.data(), bin_data.data() + num_keys) - bin_data.data();
    std::cout << "num_keys=" << num_keys << std::endl;
    pack_KV_dataset(bin_data, data);
    //std::sort(data.begin(), data.end(), [](auto const &a, auto const &b){return a.first < b.first;});
    vector<std::pair<key_type, key_type> > query;
    vector<value_type> answer;
    //generate_query<key_type, value_type, std::uniform_int_distribution<long long> >(data, query, answer, num_ops);
    query.resize(num_ops);
    vector<long long> query_pos(num_ops);
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, num_ops, 0, num_keys - 1 - length);
    for (long long i = 0; i < num_ops; ++i){
        long long pos = query_pos[i];
        query[i].first = data[pos].first;
        query[i].second = data[pos + length - 1].first;
    }
    
    if (index_name == "aex"){
        aex_range_query_bench(data, query);
    }
    else if (index_name == "stl_map"){
        stlmap_range_query_bench(data, query);
    }
    else if (index_name == "stx_btree"){
        stx_btree_range_query_bench(data, query);
    }
    else if (index_name == "alex"){
        alex_range_query_bench(data, query);
    }
    else if (index_name == "pgm"){
        pgm_range_query_bench(data, query);
    }
    //else if (index_name == "lipp"){
    //    lipp_range_query_bench(data, query);
    //}
    else if (index_name == "search"){
        dense_array_range_query_bench(data, query);
    }
}