#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>

#include "aex_map.h"
using namespace std;
typedef long long LL;
const int N = 10000000, TEST_N = 10000;
int main(){
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
    
    for (int i = 0; i < N; ++i){
        if (i % 100 == 0) 
            cout << "insert: " << i << ":" << data[i] << endl;
        #ifdef AEX_DEBUG
        if (false) mp.set_debug_level(1);
        else mp.set_debug_level(0);
        #endif 
        auto y = mp.insert(std::pair<int, int>(data[i], i));
        /*
        if (i % 10000 == 0) {
            bool res = mp.debug_error();
            if (res == false){
                std::cout << "error!" << std::endl;
                return 0;
            }
        }*/
        /*
        for (aex::aex_map<LL, LL>::iterator x = mp.begin(); x != mp.end() &&  j < N; ++x, ++j){
            std::cout << x.key() << ": " << x.data() << ", ";
        }*/
    }
    cout << "test finish" << endl;
    
}