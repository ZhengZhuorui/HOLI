#include <bits/stdc++.h>
#include "test/test.h"

template<typename T>
bool test_linear_model(T* data, size_t n){
    aex::linear_model<T> m;
    m.train(data, n);
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    return true;
}
template<typename T>
bool test_exp_model(T* data, size_t n){
    aex::exponential_model<T> m;
    m.train(data, n);
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    return true;
}
template<typename T>
bool test_log_model(T* data, size_t n){
    aex::logarithmic_model<T> m;
    m.train(data, n);
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    return true;
}

template<typename T>
bool test_aex_model(T* data, size_t n){
    aex::aex_model<T> m;
    m.train(data, n);
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    return true;
}