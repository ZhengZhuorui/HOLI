#pragma once
#include <bits/stdc++.h>
#include "benchmark/zipf.h"
#include "benchmark/utils.h"
#include "aex_utils.h"

template<typename T,
        typename Distribution,
        typename ArgsType>
void generate_data(vector<T> &data, const long long n, const ArgsType a, const ArgsType b){
    std::default_random_engine generator(seed);
    Distribution distribution(a, b);
    data.resize(n);
    for (long long i = 0; i < n; ++i){
        data[i] = static_cast<T>(distribution(generator));
    }
    std::random_shuffle(data.data(), data.data() + n);
}

//generate lookup and erase dataset
template<typename key_type, 
        typename value_type,
        typename Distribution=std::uniform_int_distribution<long long> >
void generate_query(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer, const long long m){
    query.resize(m);
    answer.resize(m);
    vector<long long> query_pos(m);
    generate_data<long long, Distribution, long long>(query_pos, m, 0, data.size() - 1);
    for (long long i = 0; i < m; ++i){
        long long pos = query_pos[i];
        query[i] = data[pos].first;
        answer[i] = data[pos].second;
    }
}

template<typename key_type, 
        typename value_type,
        typename Distribution=std::uniform_int_distribution<long long> >
void generate_query(std::pair<key_type, value_type> *data, const long long n, vector<key_type> &query, vector<value_type> &answer, const long long m){
    query.resize(m);
    answer.resize(m);
    vector<long long> query_pos(m);
    generate_data<long long, Distribution, long long>(query_pos, m, 0, n - 1);
    std::random_shuffle(query_pos.data(), query_pos.data() + m);
    for (long long i = 0; i < m; ++i){
        long long pos = query_pos[i];
        query[i] = data[pos].first;
        answer[i] = data[pos].second;
        //if (i == 0)
            //AEX_PRINT("pos=" << pos << ", k=" << query[i] << ", v=" << answer[i]);
    }
}

//generate lookup and erase dataset
template<typename key_type>
void split_dataset(vector<key_type> &data, vector<key_type> &split_data, const long long m){
    split_data.resize(m);
    std::random_shuffle(data.begin(), data.end());
    std::copy(data.data() + data.size() - m, data.data() + data.size(), split_data.data());
    data.resize(data.size() - m);
}

template<typename key_type,
        typename Distribution,
        typename ArgsType>
void generate_unique_dataset(vector<key_type> &data, const long long n, ArgsType a, ArgsType b){
    AEX_HINT("[generate unique data]");
    generate_data<key_type, Distribution>(data, n, a, b);
    
    std::sort(data.begin(), data.end());
    long long num = std::unique(data.data(), data.data() + n) - data.data();
    std::default_random_engine generator(seed);
    Distribution distribution(a, b);
    if (num < n){
        std::unordered_set<key_type> st;
        for (long long i = 0; i < num; ++i)
            st.insert(data[i]);
        for (long long i = num; i < n; ++i){
            key_type random_x = static_cast<key_type>(distribution(generator));;
            while(st.find(random_x) != st.end()){
                random_x = static_cast<key_type>(distribution(generator));
            }
            st.insert(random_x);
            data[i] = random_x;
        }
    }
    std::random_shuffle(data.data(), data.data() + n);
    std::random_shuffle(data.data(), data.data() + n);
}

template<typename key_type>
inline void generate_normal_unique_dataset(vector<key_type> &data, const long long n, double mean, double stddev){
    generate_unique_dataset<key_type, std::normal_distribution<double>, double>(data, n, mean, stddev);
}

template<typename key_type>
inline void generate_lognormal_unique_dataset(vector<key_type> &data, const long long n, double mean, double stddev){
    generate_unique_dataset<key_type, std::lognormal_distribution<double>, double>(data, n, mean, stddev);
}

template<typename key_type, 
        typename value_type>
void generate_query_zipf(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer, const long long m){
    query.resize(m);
    answer.resize(m);
    vector<long long> query_pos(m);
    {
        ScrambledZipfianGenerator zipf_gen(data.size());
        for (long long i = 0; i < m; i++) {
            query_pos[i] = zipf_gen.nextValue();
        }
    }
    for (long long i = 0; i < m; ++i){
        long long pos = query_pos[i];
        query[i] = data[pos].first;
        answer[i] = data[pos].second;
    }
}

template<int structure_size>
struct payload_structure{
public:
    char data[structure_size];
    payload_structure(){}

    template<typename key_type>
    payload_structure(key_type &x){
        key_type* ptr = reinterpret_cast<key_type>(this->data);
        *ptr = x;
    }
    
    template<typename key_type>
    key_type value(){
        return reinterpret_cast<key_type>(this->data);
    }
};

template<typename key_type,
        typename value_type>
inline void pack_KV_dataset(vector<key_type> &data, vector<std::pair<key_type, value_type> > &pack_data, bool is_sorted=false){
    pack_data.resize(data.size());
    if (!is_sorted)
        std::sort(data.begin(), data.end());
    value_type id = 0;
    pack_data[0] = std::make_pair(data[0], id);
    for (size_t i = 1; i < data.size(); ++i){
        id += (data[i - 1] != data[i]);
        pack_data[i] = std::make_pair(data[i], id);
    }
}
