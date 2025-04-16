#pragma once
#include "test/test.h"
/*
template<typename T>
bool test_linear_model(T* data, size_t n, bool spec_flag){
    AEX_HINT("[test_linear_model]");
    typedef typename aex::aex_default_traits<T, T> traits;
    aex_tree<T, T> tree;
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
    aex_tree<T, T> tree;
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

template<typename T,
        typename Model>
bool test_model_hash_table(T* data, size_t n, int level, int batch=1000){
    AEX_HINT("[test " << Model::name() << "] n=" << n);
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    Model m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    //for (size_t i = 0; i < n / 16; ++i)
    //    data[i] = data[i * 16];
//
    //n /= 16;
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true && tree.check_collision_hash_table(data, size, slot_size, m)){
        AEX_PRINT("size=" << size);
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

    if (tree.check_collision_hash_table(data, size, slot_size, m) == false){
        AEX_ERROR("Data Collision!");
        return false;
    }

    slot_type max_error = m.max_error(data, size, slot_size);
    AEX_PRINT("size=" << size << ", slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error);
    system_clock::time_point t1, t2;
    const int ITER = 10;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    double _ = 0;
    std::vector<long long> query_pos(batch);
    std::vector<T> query(batch);
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, batch, 0, size - 1);
    for (long long i = 0; i < batch; ++i) query[i] = data[query_pos[i]];
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        for (long long j = 0; j < batch; ++j)
            _ += m.predict(query[j]);
    t2 = std::chrono::high_resolution_clock::now();
    double delta3 = duration_cast<microseconds>(t2 - t1).count();
    long double QPS = 1.0 * 1e6 * ITER * batch / delta3;
    AEX_SUCCESS("code=" << _ << ", model Query time= " << delta3 << " ms, QPS=" << QPS);
    return true;
}

template<typename T,
        typename Model>
bool test_model(T* data, size_t n, int level, int batch=1000){
    AEX_HINT("[test " << Model::name() << "] n=" << n);
    typedef aex_tree<T, T> Index;
    typedef typename aex::aex_default_traits<T, T> traits;
    typedef typename traits::slot_type slot_type;
    aex_tree<T, T> tree;
    double ratio = tree.inner_node_few_ratio[level];
    Model m;
    slot_type size = traits::MIN_ML_INNER_NODE_SIZE;
    slot_type slot_size = min_slot_size(size, ratio, traits::MIN_INNER_NODE_SLOT_SIZE);
    while (static_cast<size_t>(size) < n && m.train(data, size, slot_size) == true){
        AEX_PRINT("size=" << size);
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
    const int ITER = 10;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    double _ = 0;
    std::vector<long long> query_pos(batch);
    std::vector<T> query(batch);
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, batch, 0, size - 1);
    for (long long i = 0; i < batch; ++i) query[i] = data[query_pos[i]];
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        for (long long j = 0; j < batch; ++j)
            _ += m.predict(query[j]);
    t2 = std::chrono::high_resolution_clock::now();
    double delta3 = duration_cast<microseconds>(t2 - t1).count();
    long double QPS = 1.0 * 1e6 * ITER * batch / delta3;
    AEX_SUCCESS("code=" << _ << ", model Query time= " << delta3 << " ms, QPS=" << QPS);
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

*/


template<typename T,
        typename traits>
bool test_model(T* data, size_t size, int batch=1000){
    typedef typename traits::slot_type slot_type;
    typedef typename aex_tree<T, T, traits>::Model Model;
    AEX_HINT("[test " << Model::name() << "] n=" << size);
    aex_tree<T, T, traits> tree;
    Model m;
    slot_type slot_size = traits::MIN_HASH_NODE_SLOT_SIZE, ans = 0;

    while (slot_size < traits::MAX_HASH_NODE_SLOT_SIZE && m.train(data, size, slot_size) == true){
        if (tree.check_model(m, data, size, slot_size)){
            AEX_PRINT("slot size=" << slot_size);
            ans = slot_size;
            slot_size <<= 1;
        }
        else
            break;
    }
    slot_size = ans;
    m.train(data, size, slot_size);

    if (slot_size < traits::MIN_INNER_NODE_SLOT_SIZE){
        AEX_ERROR("TRAIN ERROR!");
        return false;
    }
    /*
    slot_type max_error = m.max_collision(data, size, slot_size);
    AEX_PRINT("size=" << size << ", slot_size=" << slot_size << ", RMSE=" << m.RMSE(data, n) << ", max_error=" << max_error);
    if (max_error > traits::ERROR_BOUND){
        AEX_ERROR("max error larger than ERROR_BOUND, max_error=" << max_error << ", ERROR_BOUND=" << traits::ERROR_BOUND);
        return false;
    }
    AEX_SUCCESS("slot size=" << slot_size << ", max error=" << max_error << ", seg_nums=" << m.args.seg_nums);
    */
    system_clock::time_point t1, t2;
    const int ITER = 10;
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        m.train(data, size, slot_size);
    t2 = std::chrono::high_resolution_clock::now();
    double delta2 = duration_cast<microseconds>(t2 - t1).count();
    double OPS = 1.0 * 1e6 * ITER / delta2;
    AEX_SUCCESS("model train time= " << delta2 << " ms, OPS=" << OPS);
    double _ = 0;
    std::vector<long long> query_pos(batch);
    std::vector<T> query(batch);
    generate_data<long long, std::uniform_int_distribution<long long>, long long>(query_pos, batch, 0, size - 1);
    for (long long i = 0; i < batch; ++i) query[i] = data[query_pos[i]];
    t1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ITER; ++i)
        for (long long j = 0; j < batch; ++j)
            _ += m.predict(query[j]);
    t2 = std::chrono::high_resolution_clock::now();
    double delta3 = duration_cast<microseconds>(t2 - t1).count();
    long double QPS = 1.0 * 1e6 * ITER * batch / delta3;
    AEX_SUCCESS("code=" << _ << ", model Query time= " << delta3 << " ms, QPS=" << QPS);
    return true;
}