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
    size_t slot_size = min_slot_size(n, tree.inner_node_few_ratio[1], traits::MIN_INNER_NODE_SLOT_SIZE);
    //while (slot_size * tree.inner_node_few_ratio[1] < n) slot_size <<= 1;
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
    AEX_HINT("[test_quandratic_model]");
    if (spec_flag){
        T min_data = data[0], max_data = data[0];
        for (size_t i = 1; i < n; ++i){
            min_data = std::min(min_data, data[i]);
            max_data = std::max(max_data, data[i]);
        }
        T bias = rand() % 100;
        //const double quad = -1e-6, lin/-2quad = 10, lin=2e-5, inter = 1;
        for (size_t i = 0; i < n; ++i){
            double pos = (data[i] - min_data) / (max_data - min_data);
            data[i] = (10 - sqrt(100 + 1e6 * (1 - pos))) + bias;
        }
    }

    aex::quandratic_model<T, aex::aex_default_traits<T, T>> m;
    if (m.train(data, n) == false){
        printf("train failed.");
    }
    std::cout << "quad=" << m.args.quad << ", linear=" << m.args.lin << ", inter=" << m.args.inter << ", end=" <<  m.args.end << std::endl;
    AEX_SUCCESS("RMSE=" << m.RMSE(data, n));
    //for (size_t i = 0; i < n; ++i)
    //    std::cout << "key=" << data[i] << ", pos=" << i << ", predict=" << m.predict(data[i]) * n << " | ";
    return true;
}

template<typename T>
bool test_gap_array_linear_model(T* data, size_t n, bool spec_flag){
    std::cout << "[test gap array linear model]" << std::endl;
    mock_aex_tree<T, T> tree;
    typedef typename aex::aex_default_traits<T, T> traits;
    aex::gap_array_linear_model<T, aex::aex_default_traits<T, T>> m;
    size_t slot_size = min_slot_size(n, traits::MIN_INNER_NODE_SLOT_SIZE);
    m.train(data, n);
    std::cout << "slope=" << m.args.slope << "end=" <<  m.args.end << std::endl;
    long long max_error = m.max_error(data, n, slot_size);
    printf("max error=%lld\n", max_error);
    double RMSE = m.RMSE(data, n);
    printf("RMSE=%.4f\n", RMSE);
    return true;
}

template<typename T>
bool test_piecewise_linear_model(T* data, size_t n, int level){
    AEX_HINT("[test piecewise linear model] n=" << n);
    typedef mock_aex_tree<T, T> Index;
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    mock_aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    piecewise_linear_model<T, traits> m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        slot_size <<= 1;
        size = slot_size * ratio;
    }
    slot_size >>= 1;
    size = slot_size * ratio;
    m.train(data, size, slot_size);

    if (slot_size < traits::MIN_INNER_NODE_SLOT_SIZE){
        AEX_ERROR("TRAIN ERROR!");
        return false;
    }

    if (Index::check_collision(data, size, slot_size, m) == false){
        AEX_ERROR("Data Collision!");
        return false;
    }

    slot_type max_error = m.max_error(data, size, slot_size);
    AEX_PRINT("size=" << size << ", slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error << ", seg_nums=" << m.args.seg_nums);
    system_clock::time_point t1, t2;
    const int ITER = 10000;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    return true;
}

template<typename T>
bool test_piecewise_linear_model_2(T* data, size_t n, int level){
    AEX_HINT("[test piecewise linear model 2]");
    typedef mock_aex_tree<T, T> Index;
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    mock_aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    piecewise_linear_model_2<T, traits> m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        slot_size <<= 1;
        size = slot_size * ratio;
    }
    slot_size >>= 1;
    size = slot_size * ratio;
    m.train(data, size, slot_size);
    
    if (slot_size < traits::MIN_INNER_NODE_SLOT_SIZE){
        AEX_ERROR("TRAIN ERROR!");
        return false;
    }

    if (Index::check_collision(data, size, slot_size, m) == false){
        AEX_ERROR("Data Collision!");
        return false;
    }

    slot_type max_error = m.max_error(data, slot_size * ratio, slot_size);
    //AEX_PRINT("size=" << size << ", slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    //for (int i = 0; i < size; ++i)
    //    AEX_PRINT("key=" << data[i] << ", pos=" << m.predict(data[i]));

    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", size=" << size << ", max error=" << max_error << ", seg_nums=" << m.args.seg_nums);
    system_clock::time_point t1, t2;
    const int ITER = 10000;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    return true;
}


template<typename T>
bool test_piecewise_linear_model_3(T* data, size_t n, int level){
    AEX_HINT("[test piecewise linear model 3]");
    typedef mock_aex_tree<T, T> Index;
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    mock_aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    piecewise_linear_model_3<T, traits> m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        slot_size <<= 1;
        size = slot_size * ratio;
    }
    slot_size >>= 1;
    size = slot_size * ratio;
    m.train(data, size, slot_size);
    
    if (slot_size < traits::MIN_INNER_NODE_SLOT_SIZE){
        AEX_ERROR("TRAIN ERROR!");
        return false;
    }

    if (Index::check_collision(data, size, slot_size, m) == false){
        AEX_ERROR("Data Collision!");
        return false;
    }

    slot_type max_error = m.max_error(data, slot_size * ratio, slot_size);
    //AEX_PRINT("size=" << size << ", slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    //for (int i = 0; i < size; ++i)
    //    AEX_PRINT("key=" << data[i] << ", pos=" << m.predict(data[i]));

    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", size=" << size << ", max error=" << max_error << ", seg_nums=" << m.args.seg_nums);
    system_clock::time_point t1, t2;
    const int ITER = 10000;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    return true;
}

template<typename T>
bool test_piecewise_linear_model_avx_perf(T* data, size_t n, size_t qn, int level){
    AEX_HINT("[test piecewise linear model with avx performance]");
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    mock_aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    piecewise_linear_model_avx<T, traits> m;
    typename piecewise_linear_model_avx<T, traits>::Model ori_m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        slot_size <<= 1;
        size = slot_size * ratio;
    }
    slot_size >>= 1;
    size = slot_size * ratio;
    m.train(data, size, slot_size);
    ori_m.train(data, size, slot_size);

    slot_type start = 0;
    AEX_SUCCESS("size=" << size);
    for (slot_type i = 0; i < size; ++i){
        slot_type pos = std::max(0, std::min(static_cast<slot_type>(m.predict(data[i]) * slot_size), slot_size));    
        slot_type real_pos = std::max(0, std::min(static_cast<slot_type>(ori_m.predict(data[i]) * slot_size), slot_size));    
        if (pos != real_pos){
            AEX_PRINT("pos=" << pos << ", real_pos=" << real_pos);
            return false;
        }
        start = std::max(start, pos);
        ++start;
    }

    if (tree.check_collision(data, size, slot_size, m) == false){
        AEX_ERROR("Data Collision!");
        return false;
    }

    slot_type max_error = m.max_error(data, size, slot_size);
    AEX_PRINT("slot_size=" << slot_size << ", size=" << size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error << ", ori max error=" << ori_m.max_error(data, size, slot_size));
    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    //AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error);

    std::vector<T> query(qn);
    for (size_t i = 0; i < qn; ++i)
        query[i] = data[rand() % n];
    
    system_clock::time_point t1, t2;
    double q1 = 0, q2 = 0;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < qn; ++i)
        q1 += m.predict(query[i]);
    t2 = std::chrono::high_resolution_clock::now();
    double delta1 = duration_cast<microseconds>(t2 - t1).count();

    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < qn; ++i)
        q2 += ori_m.predict(query[i]);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    AEX_PRINT("code1= " << q1 << ", code2= " << q2 << "avx model use time= " << delta1 << " ms, original model use time= " << delta2 << ", ms");
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