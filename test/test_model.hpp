#pragma once
#include "test/test.h"

template<typename T>
bool test_linear_model(T* data, size_t n, bool spec_flag){
    AEX_HINT("[test_linear_model]");
    typedef typename aex::aex_default_traits<T, T> traits;
    mock_aex_tree<T, T> tree;
    aex::linear_model<T, aex::aex_default_traits<T, T>> m;
    m.train(data, n);
    std::cout << "slope=" << m.args.slope << "inter=" << m.args.inter << "end=" <<  m.args.end << std::endl;
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    size_t slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * tree.inner_node_few_ratio[1] < n) slot_size <<= 1;
    long long max_error = m.max_error(data, n, slot_size);
    AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error);
    return true;
}

template<typename T>
bool test_log_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test_logarithmic_model]" << std::endl;
    if (spec_flag){
        T bias = rand() % 100;
        for (size_t i = 0; i < n; ++i){
            data[i] = exp(data[i] / 30) + bias;
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }
    aex::logarithmic_model<T, aex::aex_default_traits<T, T>> m;
    m.train(data, n);
    std::cout << "slope=" << m.args.slope << "inter=" << m.args.inter << "end=" <<  m.args.end << std::endl;
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    for (size_t i = 0; i < n; ++i)
        std::cout << "key=" << data[i] << ", pos=" << i << ", predict=" << m.predict(data[i]) * n << " | ";
    return true;
}
template<typename T>
bool test_exp_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test_exponential_model]" << std::endl;
    aex::exponential_model<T, aex::aex_default_traits<T, T>> m;
    if (spec_flag){
        T bias = rand() % 100;
        for (size_t i = 0; i < n; ++i){
            data[i] = -exp(data[i] / 30) + bias;
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }
    std::sort(data, data + n);
    m.train(data, n);
    std::cout << "slope=" << m.args.slope << "inter=" << m.args.inter << "end=" <<  m.args.end << std::endl;
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    for (size_t i = 0; i < n; ++i)
        std::cout << "key=" << data[i] << ", pos=" << i << ", predict=" << m.predict(data[i]) * n << " | ";
    return true;
}

template<typename T>
bool test_quad_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test_quandratic_model]" << std::endl;
    if (spec_flag){
        T min_data = data[0], max_data = data[0];
        for (size_t i = 1; i < n; ++i){
            min_data = std::min(min_data, data[i]);
            max_data = std::max(max_data, data[i]);
        }
        std::cout << std::endl;
        T bias = rand() % 100;
        //const double quad = -1e-6, lin/-2quad = 10, lin=2e-5, inter = 1;
        for (size_t i = 0; i < n; ++i){
            double pos = (data[i] - min_data) / (max_data - min_data);
            data[i] = (10 - sqrt(100 + 1e6 * (1 - pos))) + bias;
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }

    //for (size_t i = 0; i < n; ++i){
    //    data[i] = exp(data[i] / 30) + bias;
    //    std::cout << data[i] << " ";
    //}
    //cout << std::endl;

    aex::quandratic_model<T, aex::aex_default_traits<T, T>> m;
    if (m.train(data, n) == false){
        printf("train failed.");
    }
    std::cout << "quad=" << m.args.quad << ", linear=" << m.args.lin << ", inter=" << m.args.inter << ", end=" <<  m.args.end << std::endl;
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    for (size_t i = 0; i < n; ++i)
        std::cout << "key=" << data[i] << ", pos=" << i << ", predict=" << m.predict(data[i]) * n << " | ";
    return true;
}

template<typename T>
bool test_gap_array_linear_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test gap array linear model]" << std::endl;
    mock_aex_tree<T, T> tree;
    typedef typename aex::aex_default_traits<T, T> traits;
    aex::gap_array_linear_model<T, aex::aex_default_traits<T, T>> m;
    size_t slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE;
    while (slot_size * tree.inner_node_full_ratio[1] < n) slot_size <<= 1;
    m.train(data, n);
    std::cout << "slope=" << m.args.slope << "end=" <<  m.args.end << std::endl;
    long long max_error = m.max_error(data, n, slot_size);
    printf("max error=%lld\n", max_error);
    double RMSE = m.RMSE(data, n);
    printf("RMSE=%.4f\n", RMSE);
    return true;
}

template<typename T>
bool test_piecewise_linear_model(T* data, size_t n){
    AEX_HINT("[test piecewise linear model]");
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    mock_aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[2];
    piecewise_linear_model<T, traits> m;
    slot_type slot_size = traits::MIN_ML_INNER_NODE_SLOT_SIZE, size = slot_size * ratio;
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        slot_size <<= 1;
        size = slot_size * ratio;
    }
    slot_size >>= 1;
    size = slot_size * ratio;
    m.train(data, size, slot_size);
    
    if (slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE){
        slot_type start = 0;
        for (slot_type i = 0; i < size; ++i){
            slot_type pos = std::max(0, std::min(static_cast<slot_type>(m.predict(data[i]) * slot_size), slot_size));    
            start = std::max(start, pos);
            ++start;
        }
    }
    if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE){
        AEX_ERROR("TRAIN ERROR!");
        return false;
    }

    slot_type max_error = m.max_error(data, slot_size * ratio, slot_size);
    AEX_PRINT("slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error);
    return true;
}

template<typename T>
bool test_aex_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test_model]" << std::endl;
    aex::aex_model<T, aex::aex_default_traits<T, T>> m;
    m.train(data, n);
    printf("RMSE=%.4f\n", m.RMSE(data, n));
    return true;
}