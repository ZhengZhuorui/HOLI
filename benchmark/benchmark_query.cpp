#include <bits/stdc++.h>
#include "aex_map.h"
#include "stx/btree_map.h"
#include "alex_map.h"
#include "alex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"

using namespace std;
typedef unsigned long long size_type;

//using std::vector;
//using std::pair;
using aex::aex_map;

template<typename key_type, typename value_type>
void aex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer){
    aex_map<key_type, value_type> index(data.begin(), data.end());
    /*
    for (const auto& x : data){
        index.insert(x);
    }*/
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
    long long num_keys = stoll(flags["num_keys"]);
    auto index_name = flags["index"];
    long long batch_size = 10000000;
    if (flags.find("batch_size") != flags.end()) 
        batch_size = stoll(flags["batch_size"]);

    if (key_type == "int"){
        vector<pair<long long, long long> > data;
        // query
        read_bineary_file<pair<long long, long long>>(file, data, num_keys);
        vector<long long> query;
        vector<long long> answer;
        //generate_uniform_unique_dataset<long long, long long>(data, query, answer, num_keys, batch_size, LONG_MIN_VALUE, LONG_MAX_VALUE);

        generate_query(data, query, answer, batch_size, num_keys);
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
        
    }
    else if (key_type == "float"){
        
    }

}