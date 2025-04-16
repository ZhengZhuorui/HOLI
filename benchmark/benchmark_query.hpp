#include <bits/stdc++.h>
#include "aex.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using namespace std::chrono;

template<typename key_type, typename value_type>
void aex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    aex::aex_tree<key_type, value_type, aex::aex_default_traits<key_type, value_type, false, void, false> > index;
    //aex::aex_tree<key_type, value_type> index;
    std::cout << "[AEX]" << std::endl;
    index.bulk_load(data.data(), data.size());
    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    std::cout << "aex map query test..." << std::endl;
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        for (auto &x : query){
            value_type y;
            index.find(x, y);
            sum += y;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << "e6" << std::endl;
}

template<typename key_type, typename value_type>
void stlmap_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    std::map<key_type, value_type> index(data.begin(), data.end());
    vector<value_type> result(data.size());

    system_clock::time_point t1, t2;

    //size_t cnt = 0;
    size_t num_ops = query.size();
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
    long long delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << "e6" << std::endl;
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
    long long delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << "e6" << std::endl;
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
        for (auto &x : query){
            auto iter = index.find(x);
            //if (it != index.cend())
                sum += iter.payload();
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << "e6" << std::endl;

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
    printf("pgm query test...");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t T = 0; T < times; ++T){
        for (auto &x : query){
            sum += index.find(x)->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << "e6" << std::endl;
}

template<typename key_type, typename value_type>
void lipp_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    LIPP<key_type, value_type> index;
    index.bulk_load(data.data(), data.size());

    //size_t cnt = 0;
    size_t num_ops = query.size();
    size_t times = 1;
    value_type _ = 0;
    std::cout << "lipp query test..." << std::endl;
    
    const auto t1 = std::chrono::high_resolution_clock::now();
    //for (size_t T = 0; T < times; ++T){
        for (size_t i = 0; i < num_ops; ++i)
            index.find(query[i], _);
    //}
    const auto t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<nanoseconds>(t2 - t1).count();
    double QPS = 1e3 * num_ops * times / delta;
    
    std::cout << "code=" << _ << ", used time=" << delta <<  " ns, QPS=" << QPS << "e6" << std::endl;
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
        for (auto &x : query){
            size_t pos = std::lower_bound(data.data(), data.data() + sz, std::pair<key_type, value_type>(x, -1)) - data.data();
            sum += data[pos].second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;

    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type, typename value_type>
void hash_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    printf("hash map query test...");
    std::unordered_map<key_type, value_type> index(data.begin(), data.end());

    system_clock::time_point t1, t2;
    size_t times = 1;
    size_t num_ops = query.size();
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();

    for (size_t T = 0; T < times; ++T){
        for (auto &x : query){
            auto iter = index.find(x);
            sum += iter->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    double QPS = 1000000.0 * num_ops * times / delta;

    std::cout << "code=" << sum << ", used time=" << delta <<  " ms, QPS=" << QPS << std::endl;
}

template<typename key_type,
        typename value_type>
void benchmark_lookup(FILE* file, long long num_keys, long long num_ops, std::string &index_name, std::string &query_dis){
    std::cout << "benchmark lookup: " << index_name << std::endl;
    vector<key_type> bin_data;
    vector<pair<key_type, value_type> > data;
    size_t _ = read_bineary_file<key_type>(file, bin_data, num_keys, file_is_head);
    std::cout << _ << ", " << num_keys << std::endl;
    assert((long long)_ == num_keys);
    pack_KV_dataset(bin_data, data);
    vector<key_type> query;
    vector<value_type> answer;
    if (query_dis == "uniform")
        generate_query<key_type, value_type, std::uniform_int_distribution<long long> >(data, query, answer, num_ops);
    else if (query_dis == "zipfian")
        generate_query_zipf<key_type, value_type>(data, query, answer, num_ops);

    std::sort(data.begin(), data.end(), [](auto const &a, auto const &b){return a.first < b.first;});
    std::cout << "query_num=" << query.size() << ", num_ops=" << num_ops << std::endl;
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
    else if (index_name == "hash"){
        hash_query_bench(data, query, answer);
    }
    std::cout << "benchmark query finish..." << std::endl;
}