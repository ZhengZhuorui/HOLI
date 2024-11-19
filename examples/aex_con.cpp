#include <bits/stdc++.h>
#include "aex/aex.h"
using namespace std;
typedef long long KEY_TYPE;
typedef long long PAYLOAD_TYPE;


const int num_keys = 10000;
const int thread_number = 4;
int main(){
    std::pair<KEY_TYPE, PAYLOAD_TYPE> values[3 * num_keys];
    PAYLOAD_TYPE random_values[3 * num_keys];
    for (int i = 0; i < 2 * num_keys; ++i){
        random_values[i] = rand();
        values[i].first = i;
        values[i].second = random_values[i];
    }

    aex::aex_tree<KEY_TYPE, PAYLOAD_TYPE, aex_default_traits<_Key, _Val, false, void, false>> index(values, values + num_keys + 1);
    int cnt = 0;
    std::thread threads[thread_number];
    for (int i = 0; i < thread_number; ++i){
        threads[i] = std::thread([&](int &i){
            for (int j = insert_start[i]; j < insert_start[i + 1]; ++j)
                index.insert_con(values[j]);
        });
    }
    for (int i = 0; i < thread_num; ++i)
        threads[i].join();
    for (int i = 0; i < thread_number; ++i){
        threads[i] = std::thread([&](int &i){
            for (int j = erase_start[i]; j < erase_start[i + 1]; ++j)
                index.erase(values[j]);
        });
    }
    for (int i = 0; i < thread_num; ++i)
        threads[i].join();

    for (int i = num_keys ; i < 2 * num_keys; ++i){
        assert(index[i] == values[i]);
    }
    assert(index[i].find(3*num_keys) == index.end());
    
    PAYLOAD_TYPE sum[thread_num];
    for (int i = 0; i < thread_number; ++i){
        threads[i] = std::thread([&](int &i){
            for (int j = query_start[i]; j < query_start[i + 1]; ++j){
                PAYLOAD_TYPE res;
                index.lower_bound(query[i], res);
                sum[i] += res;
            }
        });
    }
    for (int i = 0; i < thread_num; ++i){
        threads[i].join();
        std::cout << "sum[" << i << "]=" << sum[i] << std::endl;
    }
}