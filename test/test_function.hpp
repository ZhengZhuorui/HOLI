#include <bits/stdc++.h>
#include "aex_utils.h"
#include "test/test.h"

using namespace std::chrono;

template<typename _Tp>
bool test_exponential_search_lower_bound(_Tp* data, size_t n){
    
    AEX_PRINT("[test_exponential_search_lower_bound]");
    std::sort(data, data + n);

    long long real = n >> 1;
    _Tp x = data[real];
    for (size_t i = 0; i < n; ++i){    
        long long predict = i;
        long long exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict, x) - data;
        //AEX_PRINT("predict=" << i << ", exp_search_pos=" << exp_search_pos << ", real=" << real);
        if (exp_search_pos != real){
            AEX_ERROR("Error!, item " << x << ", real position " << real << ", predict position " << predict <<  ", expenential search position " << exp_search_pos);
            return false; 
        }
    }
    return true;
}

template<typename _Tp>
bool test_exponential_search_upper_bound(_Tp* data, size_t n){
    
    AEX_PRINT("[test_exponential_search_lower_bound]");
    std::sort(data, data + n);

    long long real = n >> 1;
    _Tp x = data[real];
    for (size_t i = 0; i < n; ++i){    
        long long predict = i;
        long long exp_search_pos = aex::exponential_search_upper_bound(data, data + n, data + predict, x) - data;
        if (exp_search_pos != real + 1){
            AEX_ERROR("Error!, item " << x << ", real position " << real << ", predict position " << predict <<  ", expenential search position " << exp_search_pos);
            return false; 
        }
    }
    return true;
}

template <class K>
int alex_binary_search_lower_bound(K* data, int l, int r, const K& key) {
    while (l < r) {
      int mid = l + (r - l) / 2;
      if (data[mid] >= key) {
        r = mid;
      } else {
        l = mid + 1;
      }
    }
    return l;
  }

template <class K>
inline int alex_exponential_search_lower_bound(K* data, int n, int m, const K& key) {
    // Continue doubling the bound until it contains the lower bound. Then use
    // binary search.
    int bound = 1;
    int l, r;  // will do binary search in range [l, r)
    if (data[m] >= key) {
        int size = m;
        while (bound < size &&
                data[m - bound] >= key) {
        bound *= 2;
        }
        l = m - std::min<int>(bound, size);
        r = m - bound / 2;
    } else {
        int size = n - m;
        while (bound < size && data[m + bound] < key) {
        bound *= 2;
        }
        l = m + bound / 2;
        r = m + std::min<int>(bound, size);
    }
    return alex_binary_search_lower_bound(data, l, r, key);
}

template<typename _Tp>
bool test_search_perf(_Tp* data, size_t n){
    AEX_HINT("[test_search_perf]");
    std::sort(data, data + n);
    _Tp min_value = data[0], max_value = data[n - 1];
    //cout << min_value << " " << max_value << std::endl;

    const int iter = 10;
    const int M = 1000000;
    vector<size_t> query(M);
    for (int i = 0; i < M ; ++i){
        query[i] = rand() % n;
        //predict[i] = std::max(0ULL, std::min(n - 1, query[i] + (rand() % 17) - 8));
    }

    system_clock::time_point t1, t2;
    long long sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long predict = std::max((long long)0, std::min((long long)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
            long long exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict, x) - data;
            sum += exp_search_pos;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    printf("self exp lower bound used time=%lld us\n", delta);

    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long predict = std::max((long long)0, std::min((long long)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
            long long exp_search_pos = alex_exponential_search_lower_bound(data, n, predict, x);
            sum += exp_search_pos;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    printf("alex exp lower bound used time=%lld us\n", delta);

    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long real = std::lower_bound(data, data + n, x) - data;
            sum += real;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    printf("stl lower bound used time=%lld us\n", delta);

    
    AEX_PRINT("code=" << sum);
    return true;
}

template<typename _Tp>
bool test_search_with_error_bound_perf(_Tp* data, size_t n){
    AEX_HINT("[test_search_with_error_bound_perf]");
    std::sort(data, data + n);

    const int iter = 10;
    const int M = 1000000;
    const int ERROR_BOUND = 16;
    std::vector<size_t> query(M);
    std::vector<long long> predict(M);
    for (int i = 0; i < M ; ++i){
        query[i] = rand() % n;
        int offset = rand() % ERROR_BOUND;
        //int offset_r = rand() % (1 << ERROR_BOUND), offset=0;
        //for (int j = 0; j < ERROR_BOUND; ++j)
        //if ((offset_r >> j) & 1){
        //    offset = j;
        //    break;
        //}
        if (i < 100)
            std::cout << offset << ", ";
        predict[i] = std::max(0LL, std::min(static_cast<long long>(n - 1), static_cast<long long>(query[i] - (offset))));
    }

    system_clock::time_point t1, t2;
    long long sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict[i], x) - data;
            sum += exp_search_pos;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    long long delta = duration_cast<microseconds>(t2 - t1).count();
    AEX_IMPORTANT("code=" << sum << ", search used time=" << delta << " us");

    sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long exp_search_pos = alex_exponential_search_lower_bound(data, n, predict[i], x);
            sum += exp_search_pos;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    AEX_IMPORTANT("code=" << sum << ", search used time=" << delta << " us");

    sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long real = std::lower_bound(data + std::max(0LL, predict[i]), data + std::min(static_cast<long long>(n), predict[i] + ERROR_BOUND + 1), x) - data;
            sum += real;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    AEX_IMPORTANT("code=" << sum << ", search used time=" << delta << " us");

    sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            //long long lb = std::max(0LL, predict[i] - 8), ub = std::min(static_cast<long long>(n), predict[i] + ERROR_BOUND + 1);
            //long long real = ub;
            //for (long long j = lb; j < ub; ++j)
            //    if (data[j] >= x){
            //        real = j;
            //        break;
            //    }
            long long real = aex::linear_search_lower_bound(data + predict[i], data + n, x) - data;
            sum += real;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    AEX_IMPORTANT("linear search, code=" << sum << ", search used time=" << delta << " us");
    sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long real = aex::lower_bound_with_error_bound<_Tp, 8>(data + predict[i], data + n, x) - data;
            sum += real;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    AEX_IMPORTANT("code=" << sum << ", search used time=" << delta << " us");
    return true;
}
/*
template<typename key_type, 
        typename value_type,
        typename traits>
bool test_linear_probe(key_type* data, size_t n){
    AEX_HINT("[test linear probe]");
    aex_tree<key_type, value_type, traits> tree;
    typedef typename traits::slot_type slot_type;
    std::sort(data, data + n);
    typename aex_tree<key_type, value_type, traits>::DataNodeModel m;
    slot_type ret = tree.linear_probe(data, n, m);
    AEX_PRINT("ret=" << ret << ", end=" << m.args.end << ", slope=" << m.args.slope << ", inter=" << m.args.inter);
    for (slot_type i = 0; i < ret; ++i){
        slot_type pred_pos = static_cast<slot_type>(m.predict(data[i]) * (ret - 1));
        if (std::abs(pred_pos - i) > traits::DATA_NODE_ERROR_BOUND){
            AEX_ERROR("error wrong! predict pos=" << pred_pos << ", real pos=" << i << "ERROR=" << pred_pos - i);
            return false;
        }
    }
    const int ITER = 10;
    std::chrono::system_clock::time_point t1, t2;
    size_t sum = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i){
        sum += tree.linear_probe(data, n, m);
    }
    t2 = std::chrono::high_resolution_clock::now();
    double delta = duration_cast<microseconds>(t2 - t1).count();
    double NPS = 1e6 * ITER * ret / delta;
    AEX_SUCCESS("code=" << sum << ", linear probe=" << delta << "ms, NPS=" << NPS);
    return true;
}
*/