#pragma once

namespace aex{

template<typename _Tp>
class linear_model{
public:
    typedef _Tp key_type;
    typedef linear_model<key_type> self;
    typedef unsigned long long* bitmap;
    typedef size_t size_type;

    // return the predict position. value range from 0 to +inf.
    inline size_type predict(const key_type &key) const {
        return static_cast<size_type>(std::max(0, static_cast<int>(args.slopt * key + args.inter)));
    }

    // train model with an key array, array size n and slot size
    void train(const key_type* const key, const unsigned int n, const unsigned int slot_size){
        double cap_ratio = 1.0 * slot_size / n, sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (size_type  i = 0; i < n; ++i){
            size_type pos = static_cast<size_type>(i * cap_ratio);
            sum_y += pos;
            sum_xy += 1.0 * key[i] * pos;
            sum_x += key[i];
            sum_x2 += 1.0 * key[i] * key[i];
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slopt = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = bar_y - args.slopt * bar_x;
        AEX_PRINT("train. cap_ratio=" << cap_ratio << "sum_xy=" << sum_xy << " sum_x=" << sum_x << " sum_x2="  << sum_x2 << " bar_x=" << bar_x << "bar_y=" << bar_y << " fz=" << (sum_xy - n * bar_x * bar_y) << "fm=" << (sum_x2 - n * sqr(bar_x)) << " slope=" << args.slopt << " inter=" << args.inter);
        return;
    }

//private:
public:
    struct linear_arguments{
        double slopt;
        double inter;
    }args;
};


}

