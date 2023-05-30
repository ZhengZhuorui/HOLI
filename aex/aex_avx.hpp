#pragma once
template<typename _Tp,
        int K>
unsigned char find_lower_avx(const _Tp* const key, const _Tp &x){
    for (unsigned char i = 0; i < 8; ++i)
        if (key >= x) return key;
    return 8;
}


template<>
unsigned char find_lower_avx<double, 8>(const double* const key, const _Tp &x){
    __m128 k = __m_load    
}


template<typename T>
class LinearModelCore{
public:
    static pair<double, double> train(const T* const key, const size_type n){
        double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (int i = 0; i < n; ++i){
            double pos = 1.0 * (i + 1) / slot_size;
            sum_y += pos / slot_size;
            sum_xy += 1.0 * (key[i] - key[0]) * pos;
            sum_x += (key[i] - key[0]);
            sum_x2 += 1.0 * (key[i] - key[0]) * (key[i] - key[0]);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        double slopt = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        AEX_FORMAT("train. cap_ratio=%.4f, sum_xy=%.4f, sum_x=%.4f sum_x2=%.4f, bar_x=%.4f bar_y=%.4f, fz=%.4f, fm=%.4f, slopt=%.4f inter=%.4f", cap_ratio, sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slopt, args.inter);
        return std::pair<double, double>(key[0], slopt);
    }
};

template<typename T>
class ExponentialModelCore{
public:
    static pair<double, double> train(const T* const key, const size_type n){
        double *y1, *x1;
	    y1 = (double*)malloc(n * sizeof(double));
	    x1 = (double*)malloc(n * sizeof(double));
        for (int i = 0; i < n; ++i){
            double pos = 1.0 * (i + 1) / slot_size;
            y1[i] = log(pos);
            x1[i] = -1 / x[i];
        }
        double k1, k2, k3, k4, a, b;
        for (int i = 0; i < n; ++i){
            k1 += x1[i];
            k2 += y1[i];
            k3 += x1[i] * x1[i];
            k4 += y1[i] * y1[i];
        }
        a = (k1 * k4 - k2 * k3) / (k1 * k1 - n * k3);
        b = (k1 * k2 - n * k4) / (k1 * k1 - n * k3);
    }
    return std::pair<double, double>(a, b);
};

template<>
class LinearModelCore<float>{
public:
    static std::pair<T, T> train(){

    }

};

template<>
class LinearModelCore<double>{
public:
    std::pair<T, T> train(){

    }
    
    double RMSE(){

    }
};

template<>
class LinearModelCore<int64_t>{
public:
    std::pair<T, T> train(){

    }
    
    double RMSE(){

    }
};

template<>
class LinearModelCore<uint64_t>{
public:
    std::pair<T, T> train(){

    }
    
    double RMSE(){

    }
};

template<>
class LinearModelCore<uint32_t>{
public:
    std::pair<T, T> train(){

    }
    
    double RMSE(){

    }
};

template<>
class LinearModelCore<int32_t>{
public:
    std::pair<T, T> train(){

    }
    
    double RMSE(){

    }
};