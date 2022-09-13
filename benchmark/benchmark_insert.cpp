#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

#include "aex/aex_map.h"


template<typename key_type, typename value_type>
void aex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    aex::aex_map<key_type, value_type> index;
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type times = 10;
    printf("aex insert test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void aex_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);

template<typename key_type, typename value_type>
void stlmap_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > & insert_data){
    aex::aex_map<key_type, value_type> index;
    struct timezone zone;
    struct timeval t1, t2;

    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void stlmap_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);


template<typename key_type, typename value_type>
void stx_btree_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        stx::btree_map<key_type, value_type> index(data.begin(), data.end());
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void stx_btree_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);

template<typename key_type, typename value_type>
void alex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        alex::Alex<key_type, value_type> index(data.begin(), data.end());
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void alex_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);

template<typename key_type, typename value_type>
void pgm_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = query.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert(x);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

template
void pgm_insert_bench<long long, long long>(vector<pair<long long, long long> > &data, vector<pair<long long, long long> > &insert_data);