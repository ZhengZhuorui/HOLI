#include <bits/stdc++.h>
#include "aex/aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using namespace std::chrono;
typedef unsigned long long size_type;

using aex::aex_map;

template<typename key_type, typename value_type>
void aex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    aex_map<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        index.insert(x);
    }*/
    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type times = 10;
    size_type num_ops = query.size();
    value_type sum = 0;
    printf("aex map query test...");
    t1 = std::chrono::high_resolution_clock::now();

    for (size_type i = 0; i < times; ++i){
        for (const auto& x : query){
            const auto iter = index.find(x);
            sum += iter.data();
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    float QPS = 1000000.0 * num_ops * times / delta;

    std::cout << std::scientific;
    std::cout << std::setprecision(3);    
    std::cout << "used time=" << delta <<  ", QPS=" << QPS;

    // test correctly
    /*
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            //printf("AEX Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            AEX_PRINT("AEX Error! query[" << i << "]: answer=" << answer[i] << ", result=" << result[i] << "\n");
            return;
        }
    }
    */
}

template<typename key_type, typename value_type>
void stlmap_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    std::map<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        if (mp.find(x.first) != index.end()){
            //printf("error! %lld exists\n", x);
            //fflush(stdout);
            exit(0);
        }
        index.insert(x);
    }*/
    vector<value_type> result(data.size());

    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (const auto& x : query){
            const auto iter = index.find(x);
            result[cnt++] = iter->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    float QPS = 1000000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            AEX_PRINT("AEX Error! query[" << i << "]: answer=" << answer[i] << ", result=" << result[i] << "\n");
            fflush(stdout);
            return;
        }
    }
}

template<typename key_type, typename value_type>
void stx_btree_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    stx::btree_map<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        index.insert(x);
    }*/
    vector<value_type> result(data.size());
    system_clock::time_point t1, t2;

    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...");
    fflush(stdout);
    value_type sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_type i = 0; i < times; ++i){
        for (const auto& x : query){
            const auto iter = index.find(x);
            sum += iter->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    float QPS = 1000000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            AEX_PRINT("AEX Error! query[" << i << "]: answer=" << answer[i] << ", result=" << result[i] << "\n");
            fflush(stdout);
            return;
        }
    }
}

template<typename key_type, typename value_type>
void alex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    printf("[alex query benchmark]\n");
    alex::Alex<key_type, value_type> index;
    std::cout << data.size() << std::endl;
    index.bulk_load(data.data(), data.size());
    std::cout << std::scientific;
    std::cout << std::setprecision(3);    
    //key_type* query_key = get_search_keys(ori_data.data(), ori_data.size(), query.size());
    //system_clock::time_point t1, t2;
    [[maybe_unused]] size_type times = 1;
    std::cout << "alex query test..." << std::endl;
    [[maybe_unused]] value_type sum = 0;
    long long num_ops = query.size();
    auto t1 = std::chrono::high_resolution_clock::now();
    //for (size_type T = 0; T < times; ++T){
        for (int i = 0; i < num_ops; ++i){
            value_type *value = index.get_payload(query[i]);
            //if (value)
                sum += *value;
        }
    //}
    auto t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<nanoseconds>(t2 - t1).count();
    std::cout << "alex test end, sum=" << sum << std::endl;
    //float QPS = 1e9 * times * M / delta;
    double QPS = 1e9 * num_ops / delta;
    
    //printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    std::cout << "used time=" << delta << " us, QPS=" << QPS << "\n";

    // test correctly

    //for (size_type i = 0; i < answer.size(); ++i){
    //    if (answer[i] != result[i]) {
    //        AEX_PRINT("AEX Error! query[" << i << "]: answer=" << answer[i] << ", result=" << result[i] << "\n");
    //        return;
    //    }
    //}
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
    vector<value_type> result(data.size());
    
    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...");
    fflush(stdout);
    t1 = std::chrono::high_resolution_clock::now();
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (auto &x : query){
            result[cnt++] = index.find(x)->second;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    float QPS = 1000000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            AEX_PRINT("AEX Error! query[" << i << "]: answer=" << answer[i] << ", result=" << result[i] << "\n");
            fflush(stdout);
            return;
        }
    }
}
