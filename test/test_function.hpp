#include <bits/stdc++.h>
#include "aex/aex_utils.h"
#include "test/test.h"
using namespace std;

template<typename T>
bool test_exponential_search_lower_bound(T* data, size_t n){
    cout << "[test_exponential_search_lower_bound]" << endl;
    sort(data, data + n);
    T min_value = data[0], max_value = data[n - 1];
    cout << min_value << " " << max_value << endl;

    for (size_t i = 0; i < n; ++i){
        T x = data[i];
        int predict = max((size_t)0, min((size_t)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
        size_t exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict, x) - data;
        size_t real = lower_bound(data, data + n, x) - data;
        if (predict != real){
            printf("Error!, item %d, real position %d, predict position %d, expenential search position %d\n", x, real, predict, exp_search_pos);
            return false;
        }
    }
    return true;
}