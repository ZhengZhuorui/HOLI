#include <bits/stdc++.h>
#include "aex.h"
using namespace std;
typedef long long KEY_TYPE;
typedef long long PAYLOAD_TYPE;
const int num_keys = 1000;
const int thread_num = 4;

int main(){
    srand(0);
    aex::aex_tree<KEY_TYPE, PAYLOAD_TYPE, aex::aex_default_traits<KEY_TYPE, PAYLOAD_TYPE, false, void, true>> index;

    std::pair<KEY_TYPE, PAYLOAD_TYPE> values[3 * num_keys];
    int query_start[thread_num + 1], insert_start[thread_num + 1];
    //int erase_start[thread_num + 1];
    for (int i = 0; i < thread_num; ++i){
        query_start[i] = i * num_keys / thread_num;
        //erase_start[i] = num_keys + num_keys / thread_num;
        insert_start[i] = num_keys + i * num_keys / thread_num;
    }
    query_start[thread_num] = 2 * num_keys;
    //erase_start[thread_num] = 2 * num_keys;
    insert_start[thread_num] = 2 * num_keys;
    
    PAYLOAD_TYPE random_values[3 * num_keys];
    for (int i = 0; i < 3 * num_keys; ++i){
        random_values[i] = 128 * i + rand() % 128;
        values[i].first = random_values[i];
        values[i].second = i;
    }
    std::sort(values, values + num_keys, [&](auto &x, auto &y)->bool{return x.first < y.first ;});
    //std::sort(values, values + num_keys);

    index.bulk_load(values, num_keys);
    
    std::thread threads[thread_num];
    for (int i = 0; i < thread_num; ++i){
        threads[i] = std::thread([](auto index, auto *start, auto* end){
            for (auto j = start; j < end; ++j)
                index->insert(*j);
        }, &index, values + insert_start[i], values + insert_start[i + 1]);
    }
    for (int i = 0; i < thread_num; ++i)
        threads[i].join();
    //for (int i = 0; i < thread_num; ++i){
    //    threads[i] = std::thread([](auto index, auto *start, auto *end){
    //        for (auto j = start; j < end; ++j)
    //            index->erase((*j).first);
    //    }, &index, values + erase_start[i], values + erase_start[i + 1]);
    //}
    //for (int i = 0; i < thread_num; ++i)
    //    threads[i].join();

    for (int i = 0; i < 2 * num_keys; ++i){
        PAYLOAD_TYPE y;
        bool x = index.find(values[i].first, y);
        assert(x && y == values[i].second);
    }
    
    PAYLOAD_TYPE sum[thread_num];

    for (int i = 0; i < thread_num; ++i){
        sum[i] = 0;
        threads[i] = std::thread([](auto index, auto *start, auto* end, PAYLOAD_TYPE &sum){
            for (auto j = start; j < end; ++j){
                PAYLOAD_TYPE y;
                if (index->find((*j).first, y))
                    sum += y;
            }
        }, &index, values + query_start[i], values + query_start[i + 1], std::ref(sum[i]));
        
    }
    for (int i = 0; i < thread_num; ++i){
        threads[i].join();
        std::cout << "sum[" << i << "]=" << sum[i] << std::endl;
    }
}