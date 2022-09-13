#pragma once
//#include <random>
//#include <cstdio>
//#include <vector>
#include <bits/stdc++.h>

using namespace std;
template<typename T>
void generate_uniform_data(vector<T> &data, const size_type n, const T a, const T b){
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<T> distribution(a, b);
    data.resize(n);
    for (size_type i = 0; i < n; ++i){
        data[i] = distribution(generator);
    }
}

template<typename T>
void generate_normal_data(vector<T> &data, const size_type n, const T a, const T b){
    std::default_random_engine generator(seed);
    std::normal_distribution<T> distribution(a, b);
    data.resize(n);
    for (size_type i = 0; i < n; ++i){
        data[i] = distribution(generator);
    }
}

template<typename key_type, typename value_type>
void generate_query(vector<pair<key_type, value_type>> &data, vector<key_type> &query, vector<value_type> &answer, const size_type m, const size_type n){
    query.resize(m);
    answer.resize(m);
    vector<size_type> query_pos(m);
    generate_uniform_data<size_type>(query_pos, m, 0LL, n - 1);
    for (size_type i = 0; i < m; ++i){
        size_type pos = query_pos[i];
        query[i] = data[pos].first;
        answer[i] = data[pos].second;
    }
}

template<typename key_type,
        typename value_type>
void generate_uniform_unique_dataset
    (vector<pair<key_type, value_type> > &data, const size_type n, key_type L, key_type R){}

template<>
void generate_uniform_unique_dataset<long long, long long>(vector<pair<long long, long long> > &data, const size_type n, long long L, long long R){
    //std::cout << "generate uniform unique data" << std::endl;
    printf("generate uniform unique data\n");
    fflush(stdout);
    typedef long long key_type;
    typedef long long value_type;
    vector<key_type> key;
    vector<value_type> value;
    generate_uniform_data<key_type>(key, n, L, R);
    //generate_uniform_int_data<value_type>(value, N, LONG_MIN, LONG_MAX);
    value.resize(n);
    for (size_type i = 0; i < n; ++i) value[i] = i;
    data.resize(n);
    for (size_type i = 0; i < n; ++i){
        data[i] = std::pair<key_type, value_type>(key[i], value[i]);
    }
    std::sort(data.data(), data.data() + n);
    for (size_type i = 0; i < n; ++i) data[i].first += i;
    std::random_shuffle(data.data(), data.data() + n);
}