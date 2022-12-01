#pragma once

namespace aex{

enum node_property{
    LEAF=0x1,
    ML_NODE=0x2,
    CHECK_MERGE=0x4,
    COMPLEX_MODEL=0x8
};
#define UNF 0xFFFFFFFFFFFFFFFFLL

template<typename _Tp>
_Tp sqr(_Tp &x){return x * x;}

inline size_t align_8bytes(size_t x){
    return ((x + 7) & (~7) );
}

/* I don't know if it's speed up */
template<int x>
inline int lowbit_loop_unroll(int k){
    if (k & 1) return x;
    return lowbit_loop_unroll<x-1>(k >> 1);
}

template<typename traits>
class aex_bitmap_impl{
public:

    typedef size_t size_type;

    typedef unsigned long long ULL;

    typedef ULL* bitmap;

    static inline void set_one(ULL* text, const unsigned long long x) {
        text[x >> 6] |= (1LL << (x & 63));
    }
    static inline void set_zero(ULL* text, const unsigned long long x){
        text[x >> 6] &= ~(1LL << (x & 63));
    }
    static inline char at(const ULL* const text, const unsigned long long x){
        return ((text[x >> 6] >> (x & 63)) & 1);
    }

    static inline size_type next_empty_slot(ULL* text, size_type x){
        /*
        //lower bound: 4
        int p = x >> 6, q = x & 64;
        int s = text[p] >> q;

        if (s){
            return x + (traits::ERROR_BOUND - lowbit_loop_unroll<traits::ERROR_BOUND>(s));
        }
        else if (q < 48 && text[p + 1]){
            return x + (64 - q) + (traits::ERROR_BOUND - lowbit_loop_unroll<traits::ERROR_BOUND>(text[p + 1]));
        }
        else return -1;
        */
        for (size_type i = x; i < x + traits::ERROR_BOUND; ++i)
        if (!at(text, i)){
            return i;
        }
        return x + traits::ERROR_BOUND;
    }

};

template<typename RandomIter, typename _Val, typename _Comp>
inline RandomIter exponential_search_lower_bound(RandomIter begin, RandomIter end, RandomIter predict, _Val& key, _Comp comp=std::less<RandomIter>()){
    //if (*predict == key) return predict;
    //bool flag = (key < *predict) ? -1 : 1;
    size_t offset = 1;
    if (comp(key, *predict)){
        for (; predict - offset >= begin && !comp(*(predict - offset), key); offset <<= 1);
        offset >>= 1;
        predict -= offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict - offset >= begin && !comp(*(predict - offset), key) ) predict -= offset;
    }
    else{
        for (; predict + offset < end && !comp(*(predict + offset), key); offset <<= 1);
        offset >>= 1;
        predict += offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict + offset < end && !comp(*(predict + offset), key))
                predict += offset <<= 1;
        if (comp(key, *predict)) ++predict;
    }
    return predict;
}


// TODO: change lower bound to upper bound
template<typename T, typename _Val, typename _Comp>
inline T* exponential_search_upper_bound(T* begin, T* end, T* predict, _Val& key, _Comp comp=std::less<T>()){
    //if (*predict == key) return predict;
    //bool flag = (key < *predict) ? -1 : 1;
    size_t offset = 1;
    if (comp(key, *predict)){
        for (; predict - offset >= begin && !comp(*(predict - offset), key); offset <<= 1);
        offset >>= 1;
        predict -= offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict - offset >= begin && !comp(*(predict - offset), key) ) predict -= offset;
    }
    else{
        for (; predict + offset < end && !comp(*(predict + offset), key); offset <<= 1);
        offset >>= 1;
        predict += offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict + offset < end && !comp(*(predict + offset), key))
                predict += offset <<= 1;
        if (comp(key, *predict)) ++predict;
    }
    return predict;
}

}