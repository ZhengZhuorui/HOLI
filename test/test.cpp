#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>

#include "aex_map.h"
using namespace std;
typedef long long LL;
const int N = 10000000, M = 10000;
void test_exponential_search_lower_bound(){
    
    vector<int> vec(N);
    
    for (int i = 0; i < N; ++i) vec[i] = random();
    sort(vec.begin(), vec.end());
    int min_value = vec[0], max_value = vec[N - 1];

    for (int i = 0; i < N; ++i){
        int x = random() % N;
        int predict = max((int)0, min((int)N - 1, (int)(1.0 * (x - min_value) / (max_value - min_value) * N)));
        int exp_search_pos = aex::exponential_search_lower_bound(vec.begin(), vec.end(), predict, x) - vec.begin();
        int real = lower_bound(vec.begin(), vec.end(), x) - vec.begin();
        if (predict != real){
            printf("Error!, item %d, real position %d, predict position %d, expenential search position %d", x, real, predict, exp_search_pos);
            break;
        }
    }

}

int main(){
    srand(0);
    test_exponential_search_lower_bound();
    aex::aex_map<LL, LL> mp;
    cout << "?" << endl;
    vector<LL> data, rank;
    //vector<pair<LL, LL> > rank;
    for (int i = 0; i < N; ++i) data.push_back(i);
    //rank.resize(N);
    random_shuffle(data.begin(), data.end());
    //for (int i = 0; i < N; ++i)
        //rank[i] = lower_bound(data.begin(), data.end(), data[i]) - data.begin();
    
    //cout << "data= ";
    //for (int i = 0; i < N; ++i) 
        //cout << data[i] << ", ";
    //cout << endl;
    //mp.set_debug_level(0);
    {
        // insert
        for (int i = 0; i < N; ++i){
            #ifdef AEX_DEBUG
            if (false) mp.set_debug_level(1);
            else mp.set_debug_level(0);
            #endif 
            auto y = mp.insert(std::pair<int, int>(data[i], i));
        }
        // find
        for (int i = 0; i < M; ++i){
            LL x = rand() % N;
            auto y = mp.find(x);
            if (data[y.data()] != x){
                printf("Error!");
            }
        }

        // erase
        random_shuffle(data.begin(), data.end());
        for (int i = 0; i < M; ++i){
            auto y = mp.erase(data[i]);
        }
        // bulk load
        std::sort(data.begin(), data.end());
        mp.bulk_load(data);
    }   

    //multi thread
    {
        
    }
    cout << "test finish" << endl;
    
}