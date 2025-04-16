#include <bits/stdc++.h>
#include "aex.h"
using namespace std;
typedef unsigned long long KEY_TYPE;
typedef unsigned long long PAYLOAD_TYPE;


const int num_keys = 10000;
int main(){
    srand(0);
    std::pair<KEY_TYPE, PAYLOAD_TYPE> values[3 * num_keys];
    KEY_TYPE random_values[3 * num_keys];
    for (int i = 0; i < 2 * num_keys; ++i){
        random_values[i] = 128 * i + rand() % 128;
        values[i].first = random_values[i];
        values[i].second = i;
    }
    std::sort(values, values + num_keys, [&](auto &x, auto &y)->bool{return x.first < y.first ;});

    aex::aex_tree<KEY_TYPE, PAYLOAD_TYPE, aex::aex_default_traits<KEY_TYPE, PAYLOAD_TYPE, false, void, false, false>> index;
    index.bulk_load(values, num_keys);

    for (int i = num_keys; i < 2 * num_keys; ++i){
        index.insert(values[i]);
        //cout << i << ", " <<  values[i].first << ", " << values[i].second << "," << index[values[i].first]  << endl;
        assert(index[values[i].first] == values[i].second);
    }
    
    for (int i = 0; i < num_keys; ++i){
        index.erase(values[i].first);
    }

    for (int i = num_keys ; i < 2 * num_keys; ++i){
        assert(index[random_values[i]] == i);
    }
    
    //assert(index.find(3*num_keys) == index.end());
    //std::pair<KEY_TYPE, PAYLOAD_TYPE> it;
    auto it = index.lower_bound(150);
    std::cout << it.key() << ": " << it.data() << endl;

    PAYLOAD_TYPE sum = 0;
    for (auto it = index.begin(); it != index.end(); ++it)
        sum += it.data();
    std::cout << "sum=" << sum << std::endl;
}