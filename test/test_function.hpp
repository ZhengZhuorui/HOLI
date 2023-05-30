#include <bits/stdc++.h>
#include "aex/aex_utils.h"
#include "test/test.h"

using namespace std::chrono;

template<typename _Tp>
bool test_exponential_search_lower_bound(_Tp* data, size_t n){
    std::cout << "[test_exponential_search_lower_bound]" << std::endl;
    std::sort(data, data + n);
    _Tp min_value = data[0], max_value = data[n - 1];
    //cout << min_value << " " << max_value << std::endl;

    const int M = 1000000;
    vector<size_t> query(M);
    for (int i = 0; i < M ; ++i) query[i] = rand() % n;

    for (size_t i = 0; i < M; ++i){
        _Tp x = data[query[i]];
        long long predict = std::max((long long)0, std::min((long long)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
        long long exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict, x) - data;
        long long real = std::lower_bound(data, data + n, x) - data;
        if (exp_search_pos != real){
            //printf("Error!, item %d, real position %d, predict position %d, expenential search position %d\n", x, real, predict, exp_search_pos);
            std::cout << "Error!, item " << x << ", real position " << real << ", predict position " << predict <<  ", expenential search position " << exp_search_pos << " \n";
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
bool test_exponential_search_lower_bound_perf(_Tp* data, size_t n){
    std::cout << "[test_exponential_search_lower_bound_perf]" << std::endl;
    std::sort(data, data + n);
    _Tp min_value = data[0], max_value = data[n - 1];
    //cout << min_value << " " << max_value << std::endl;

    const int iter = 10;
    const int M = 1000000;
    vector<size_t> query(M);
    for (int i = 0; i < M ; ++i) query[i] = rand() % n;

    system_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            long long predict = std::max((long long)0, std::min((long long)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
            [[maybe_unused]] long long exp_search_pos = aex::exponential_search_lower_bound(data, data + n, data + predict, x) - data;
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
            [[maybe_unused]] long long exp_search_pos = alex_exponential_search_lower_bound(data, n, predict, x);
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    printf("alex exp lower bound used time=%lld us\n", delta);

    t1 = std::chrono::high_resolution_clock::now();
    for (int T = 0; T < iter; ++T){
        for (size_t i = 0; i < M; ++i){
            _Tp x = data[query[i]];
            [[maybe_unused]] long long real = std::lower_bound(data, data + n, x) - data;
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    delta = duration_cast<microseconds>(t2 - t1).count();
    printf("stl lower bound used time=%lld us\n", delta);
    return true;
}

template<typename _Tp>
bool test_exponential_search_upper_bound(_Tp* data, size_t n){
    std::cout << "[test_exponential_search_upper_bound]" << std::endl;
    std::sort(data, data + n);
    _Tp min_value = data[0], max_value = data[n - 1];
    //cout << min_value << " " << max_value << std::endl;

    for (size_t i = 0; i < n; ++i){
        _Tp x = data[i];
        long long predict = std::max((long long)0, std::min((long long)n - 1, static_cast<long long>(1.0 * (x - min_value) / (max_value - min_value) * n)));
        long long exp_search_pos = aex::exponential_search_upper_bound(data, data + n, data + predict, x) - data;
        long long real = std::upper_bound(data, data + n, x) - data;
        if (exp_search_pos != real){
            //printf("Error!, item %d, real position %d, predict position %d, expenential search position %d\n", x, real, predict, exp_search_pos);
            std::cout << "Error!, item " << x << ", real position " << real << ", predict position " << predict <<  ", expenential search position " << exp_search_pos << std::endl;
            return false; 
        }
    }
    return true;
}

template<typename key_type, 
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_linear_probe(key_type* data, size_t n){
    AEX_HINT("[test linear probe]");
    mock_aex_tree<key_type, value_type, traits> tree;
    typedef typename traits::size_type size_type;
    std::sort(data, data + n);
    typename mock_aex_tree<key_type, value_type, traits>::data_node_model m;
    size_type ret = tree.linear_probe(data, n, m);
    AEX_PRINT("ret=" << ret << ", start=" << m.args.start << ", slope=" << m.args.slope << ", inter=" << m.args.inter);
    double ERROR = traits::MAX_ALLOW_ERROR * log(std::max(static_cast<size_type>(traits::MIN_ML_DATA_NODE_SLOT_SIZE), ret));
    for (size_type i = 0; i < ret; ++i){
        size_type pred_pos = m.predict(data[i]) * (ret - 1);
        if (std::abs(pred_pos - i) > ERROR){
            AEX_ERROR("error wrong! predict pos=" << pred_pos << ", real pos=" << i << "ERROR=" << ERROR);
            return false;
        }
    }
    const int ITER = 10;
    std::chrono::system_clock::time_point t1, t2;
    size_type sum = 0;
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