#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <vector>
#include <utility>
#include <chrono>
#include <random>
#include <map>

#include "aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

//#include "utils.h"

//const int N = 10000000, M = 10000000;
//const long long LONG_MAX = (1LL << 62) - 1, LONG_MIN = (1LL << 62) - 1;
typedef unsigned long long size_type;

using std::vector;
using std::pair;
using aex::aex_map;


template<typename key_type, typename value_type>
void aex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    aex_map<key_type, value_type> index;
    for (const auto& x : data){
        index.insert(x);
    }
    vector<value_type> result(data.size());
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type times = 10;
    size_type M = query.size();
    printf("aex map query test...");
    gettimeofday(&t1, &zone);

    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (const auto& x : query){
            const auto iter = index.find(x);
            result[cnt++] = iter.data();
        }
    }
    gettimeofday(&t2, &zone);
    float delta = (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    float QPS = 1000.0 * M * times / delta;

    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            printf("AEX Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            return;
        }
    }
}

template
void aex_query_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<long long> &query, vector<long long> &answer);

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
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    gettimeofday(&t1, &zone);
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (const auto& x : query){
            const auto iter = index.find(x);
            result[cnt++] = iter->second;
        }
    }
    gettimeofday(&t2, &zone);
    float delta = (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            printf("benchmark Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            fflush(stdout);
            return;
        }
    }
}

template 
void stlmap_query_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<long long> &query, vector<long long> &answer);

template<typename key_type, typename value_type>
void stx_btree_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    
    stx::btree_map<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        index.insert(x);
    }*/
    vector<value_type> result(data.size());
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...");
    fflush(stdout);
    gettimeofday(&t1, &zone);
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (const auto& x : query){
            const auto iter = index.find(x);
            result[cnt++] = iter->second;
        }
    }
    gettimeofday(&t2, &zone);
    float delta = (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            printf("benchmark Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            fflush(stdout);
            return;
        }
    }
}

template
void stx_btree_query_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<long long> &query, vector<long long> &answer);

template<typename key_type, typename value_type>
void alex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    alex::Alex<key_type, value_type> index(data.begin(), data.end());
    size_type cnt = 0;
    /*
    for (const auto& x : data){
        ++cnt;
        if (cnt % 10000 == 0) std::cout << "cnt=" << cnt << std::endl;
        //mp.insert(x);
        index.insert(std::abs(x.first), x.second);
    }*/
    vector<value_type> result(data.size());
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...");
    fflush(stdout);
    gettimeofday(&t1, &zone);
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (const auto& x : query){
            const auto iter = index.find(x);
            result[cnt++] = iter.payload();
        }
    }
    gettimeofday(&t2, &zone);
    float delta = (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            printf("benchmark Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            fflush(stdout);
            return;
        }
    }
}

template
void alex_query_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<long long> &query, vector<long long> &answer);

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
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...");
    fflush(stdout);
    gettimeofday(&t1, &zone);
    for (size_type i = 0; i < times; ++i){
        size_type cnt = 0;
        for (auto &x : query){
            size_type cnt = 0;
            result[cnt++] = index.find(x)->second;
        }
    }
    gettimeofday(&t2, &zone);
    float delta = (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);

    // test correctly
    for (size_type i = 0; i < answer.size(); ++i){
        if (answer[i] != result[i]) {
            printf("benchmark Error! query[%lld]: answer=%lld, result=%lld\n", i, answer[i], result[i]);
            fflush(stdout);
            return;
        }
    }
}

template
void pgm_query_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<long long> &query, vector<long long> &answer);


