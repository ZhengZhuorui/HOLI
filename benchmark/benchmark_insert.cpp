#include <bits/stdc++.h>

#include "aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"
#include "utils.h"
#include "generate_dataset.h"

using namespace std;
typedef unsigned long long size_type;


template<typename key_type, typename value_type>
void aex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data){
    struct timezone zone;
    struct timeval t1, t2;

    //size_type cnt = 0;
    size_type M = insert_data.size();
    size_type times = 10;
    printf("aex insert test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        aex::aex_map<key_type, value_type> index(data.begin(), data.end());
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
    struct timezone zone;
    struct timeval t1, t2;

    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        std::map<key_type, value_type> index(data.begin(), data.end());
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

    size_type M = insert_data.size();
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
    size_type M = insert_data.size();
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
    size_type M = insert_data.size();
    size_type times = 10;
    printf("stl map query test...\n");
    fflush(stdout);
    float delta = 0;
    for (size_type i = 0; i < times; ++i){
        pgm::DynamicPGMIndex<key_type, value_type> index(data.begin(), data.end());
        gettimeofday(&t1, &zone);    
        for (const auto& x : insert_data){
            index.insert_or_assign(x.first, x.second);
        }
        gettimeofday(&t2, &zone);
        delta += (t2.tv_sec - t1.tv_sec) * 1000.0 + 1.0 * (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    float QPS = 1000.0 * M * times / delta;
    
    printf("used time=%.2f ms, QPS=%.2f\n", delta, QPS);
    fflush(stdout);
}

/*
 * Required flags:
 * generate_dataset
 * input_files
 * key_type
 * num_keys
 * index
 * Optional flags:
 * batch_size
 *
 */
int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];
    auto input_files = flags["input_files"];
    FILE *file = fopen(input_files.c_str(), "rb");
    auto dataset = flags["dataset"];
    //auto func = flags["func"];
    long long num_keys = stoll(flags["num_keys"]);
    auto index_name = flags["index"];
    long long batch_size = 10000000;
    if (flags.find("batch_size") != flags.end()) 
        batch_size = stoll(flags["batch_size"]);

    if (key_type == "int"){
        vector<pair<long long, long long> > data;
        read_bineary_file<pair<long long, long long>>(file, data, num_keys + batch_size);
        vector<pair<long long, long long> > insert_data;
        //read_bineary_file<pair<long long, long long>>(file, data, batch_size);
        insert_data.resize(batch_size);
        memcpy(insert_data.data(), data.data() + num_keys, batch_size * sizeof(pair<long long, long long>));
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
    }
    else if (key_type == "float"){
        
    }

}