#include <bits/stdc++.h>
#include "aex/aex.h"
using namespace std;
typedef long long KEY_TYPE;
typedef long long PAYLOAD_TYPE;


const int num_keys = 100;
int main(){
    srand(0);
    std::pair<KEY_TYPE, PAYLOAD_TYPE> values[3 * num_keys];
    PAYLOAD_TYPE random_values[3 * num_keys];
    for (int i = 0; i < 2 * num_keys; ++i){
        random_values[i] = rand();
        values[i].first = random_values[i];
        values[i].second = i;
    }
    std::sort(values, values + num_keys, [&](auto &x, auto &y)->bool{return x.first < y.first ;});

    aex::aex_tree<KEY_TYPE, PAYLOAD_TYPE> index;
    index.bulk_load(values, num_keys);
    
    for (int i = num_keys; i < 2 * num_keys; ++i){
        index.insert(values[i]);
    }
    
    for (int i = 0; i < num_keys; ++i){
        index.erase(values[i].first);
    }

    for (int i = num_keys ; i < 2 * num_keys; ++i){
        assert(index[random_values[i]] == i);
    }
    assert(index.find(3*num_keys) == index.end());

    aex::aex_tree<KEY_TYPE, PAYLOAD_TYPE>::iterator it = index.lower_bound(150);
    std::cout << it.key() << " " << it.data() << endl;
}