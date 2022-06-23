#pragma once
#include "aex/aex_traits.h"
#include<bits/stdc++.h>

namespace aex{

enum node_property{
    LEAF=0x1,
    INNER_LB_NODE=0x2,
    INNER_NODE=0x4
};

template<typename _Tp>
_Tp sqr(_Tp &x){return x * x;}

inline size_t align_8bytes(size_t x){
    return ((x + 7) & (~7) );
}

inline size_t BITMAP_MEMORY_USED(size_t x){
    return align_8bytes((ceil(1.0 * (x + ERROR_BOUND) / 64)) * sizeof(uint64_t));
}

template<typename _Tp>
inline size_t KEY_MEMORY_USED(size_t x){
    return align_8bytes((x + ERROR_BOUND) * sizeof(_Tp));
}

inline size_t PTR_MEMORY_USED(size_t x){
    return align_8bytes((x + ERROR_BOUND) * sizeof(void*));
}

template<typename _Val>
inline size_t DATA_MEMORY_USED(size_t x){
    return align_8bytes((x + ERROR_BOUND) * sizeof(_Val));
}



template<int x>
inline int lowbit_loop_unroll(int k){
    if (k & 1) return x;
    return get_bit_loop_unroll<x-1>(k >> 1);
}

template<typename traits>
class aex_bitmap_impl{
public:
    static void set_one(uint64_t* text, int x){text[x >> 6] |= (1 << (x & 64));}
    void set_zero(uint64_t* text, int x){text[x >> 6] &= (0xFFFFFFFFFFFFFFFF - (1 << (x & 64)));}
    char at(uint64_t* text, int x){return ((text[x >> 6]>>(x & 64)) & 1);}
    int next_empty_slot(uint64_t* text, int x){
        //lower bound: 4
        int p = x >> 6, q = x & 64;
        int s = text[p] >> q;

        if (s){
            return x + (ERROR_BOUND - lowbit_loop_unroll<ERROR_BOUND>(s));
        }
        else if (q < 48 && text[p + 1]){
            return x + (64 - q) + (ERROR_BOUND - lowbit_loop_unroll<ERROR_BOUND>(text[p + 1]));
        }
        else return -1;
    }

};

}