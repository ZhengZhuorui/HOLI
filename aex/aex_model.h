#pragma once
#include <bits/stdc++.h>
#include "aex_traits.h"
namespace aex{

template<typename _Tp>
class linear_model{
public:
    typedef _Tp key_type;
    typedef linear_model<key_type> self;
    typedef u_int64_t* bitmap;
    int predict(const key_type &k){
        return (int)(line_args.slopt * k + inter);
    }

    void train(const key_type* const k, const int n, const int slot_size){
        int n = ptr->size;
        float cap_ratio = slot_size / n;
        float sum_x2 = 0, sum_xy = 0, sum_x = 0, sum_y = cap_ratio * (n - 1) * n / 2, bar_x, bar_y = slot_size / 2;

        for (int i = 0; i < n; ++i){
            sum_xy += k[i] * i;
            sum_x += k[i];
            sum_x2 = k[i] * k[i];
        }
        sum_xy *= cap_ratio;
        bar_x = sum_x / n;
        this->args.slopt = (sum_xy - n * bar_x * bar_y) / (sum_x2 - n * sqr(bar_x));
        this->args.inter = bar_y - args.slopt * bar_x;
        return;
    }

private:
    struct linear_arguments{
        float slopt;
        float inter;
    }args;
};


}

