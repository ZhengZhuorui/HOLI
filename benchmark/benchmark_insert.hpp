#include <bits/stdc++.h>

#include "aex/aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"
#include "utils.h"
#include "generate_dataset.h"

using namespace std;
using namespace chrono;
typedef unsigned long long size_type;


template<typename key_type, typename value_type>
void aex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type M = insert_data.size();
    size_type times = 10;
    printf("aex insert test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_type i = 0; i < times; ++i){
        t1 = system_clock::now();
        aex::aex_map<key_type, value_type> index(data.begin(), data.end());
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = system_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    
    
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}


template<typename key_type, typename value_type>
void stlmap_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > & insert_data){
    
    system_clock::time_point t1, t2;

    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_type i = 0; i < times; ++i){
        std::map<key_type, value_type> index(data.begin(), data.end());
        t1 = system_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = system_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void stlmap_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);


template<typename key_type, typename value_type>
void stx_btree_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_type i = 0; i < times; ++i){
        stx::btree_map<key_type, value_type> index(data.begin(), data.end());
        t1 = system_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = system_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void stx_btree_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);

template<typename key_type, typename value_type>
void alex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_type i = 0; i < times; ++i){
        alex::Alex<key_type, value_type> index(data.begin(), data.end());
        t1 = system_clock::now();    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        t2 = system_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void alex_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);

template<typename key_type, typename value_type>
void pgm_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    
    system_clock::time_point t1, t2;

    //size_type cnt = 0;
    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    long long delta = 0;
    for (size_type i = 0; i < times; ++i){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        t1 = system_clock::now();    
        for (const auto& x : insert_data){
            index.insert_or_assign(x.first, x.second);
        }
        t2 = system_clock::now();
        delta += duration_cast<microseconds>(t2 - t1).count();
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%lld us, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

