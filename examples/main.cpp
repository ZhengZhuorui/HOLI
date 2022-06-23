#include <bits/stdc++.h>
#include "aex/aex.h"
using namespace std;
typedef int KEY_TYPE;
typedef int PAYLOAD_TYPE;


const int num_keys = 100;
int main(){
    std::pair<KEY_TYPE, PAYLOAD_TYPE> values[3 * num_keys];
    PAYLOAD_TYPE random_values[3 * num_keys];
    for (int i = 0; i < 2 * num_keys; ++i){
        random_values[i] = rand();
        values[i].first = i;
        values[i].second = random_values[i];
    }
    for (int i = 0; i < 3*)
    //for (int i = 0; i < num_keys; ++i) 

    aex::aex_map<KEY_TYPE, PAYLOAD_TYPE> index(values, values + num_keys + 1);
    
    for (int i = num_keys; i < 2 * num_keys; ++i){
        index.insert(values[i]);
    }
    
    for (int i = 0; i < num_keys; ++i){
        index.erase(values[i]);
    }

    for (int i = num_keys ; i < 2 * num_keys; ++i){
        assert(index[i] == values[i]);
    }
    assert(index[i].find(3*num_keys) == index.end());

    aex::aex<KEY_TYPE, PAYLOAD_TYPE>::iterator it = index.lower_bound(150);
    std::cout << it->key() << " " << it->payload() << endl;

}