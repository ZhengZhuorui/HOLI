#pragma once
#include <atomic>
namespace aex{

enum node_property{
    LEAF=0x1,
    ML_NODE=0x2,
    CHECK_MERGE=0x4,
    CHECK_SPLIT=0x8,
    COMPLEX_MODEL=0x10,
    SORTED_NODE=0x20
};

#define UNF 0xFFFFFFFFFFFFFFFFLL

template<typename _Tp>
inline _Tp rapid_pow(_Tp base, unsigned long long x){
    _Tp ans = 1;
    for (;x>0; x >>= 1){
        if (x & 1) ans *= base;
        base *= base;
    }
    return ans;
}

template<typename _Tp>
_Tp sqr(const _Tp &x){return x * x;}

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

    typedef typename traits::bitmap bitmap;

    static inline void set_one(bitmap text, const unsigned long long x) {
        text[x >> 6] |= (1LL << (x & 63));
    }
    static inline void set_zero(bitmap text, const unsigned long long x){
        text[x >> 6] &= ~(1LL << (x & 63));
    }
    static inline char at(const bitmap text, const unsigned long long x){
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

class aex_spinlock{
public:
    aex_spinlock():locked(ATOMIC_FLAG_INIT){}
    aex_spinlock(const aex_spinlock&) = delete;
    void lock(){
        //uint8_t unlocked = 0, locked = 1;
        while (!locked.test_and_set()){}
    }
    void unlock(){
        //this->locked = 0;
        locked.clear();
    }
    std::atomic_flag locked;
};

class aex_read_write_lock{
public:
aex_read_write_lock() = default;
aex_read_write_lock(const aex_read_write_lock&) = delete;

void lock_reader(){
    while(true){
        int c = counter.load(std::memory_order_relaxed);
        if (c == -1){
            //pause_cpu();
            continue;
        }
        while (c != -1){
            if (counter.compare_exchange_strong(c, c + 1, std::memory_order_acquire))
                return;
        }
    }

}
void unlock_reader(){
    counter.fetch_sub(1, std::memory_order_release);
}

void lock_writer(){
    while (true){
        while (counter.load(std::memory_order_relaxed) != 0){
            //pause_cpu();
        }
        int c = 0;
        if (counter.compare_exchange_strong(c, -1, std::memory_order_acquire))
            break;
    }
}

void unlock_writer(){
    counter.exchange(0, std::memory_order_release);
}
private:
    std::atomic_int counter{0};
};

template<typename RandomIter, typename _Val, typename _Comp>
inline RandomIter exponential_search_lower_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key, _Comp comp){
    size_t offset = 1;
    if (comp(key, *predict)){
        for (; predict - offset >= first && !comp(*(predict - offset), key); offset <<= 1);
        offset >>= 1;
        predict -= offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict - offset >= first && !comp(*(predict - offset), key) ) predict -= offset;
    }
    else{
        for (; predict + offset < last && !comp(*(predict + offset), key); offset <<= 1);
        offset >>= 1;
        predict += offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict + offset < last && !comp(*(predict + offset), key))
                predict += offset <<= 1;
        if (comp(key, *predict)) ++predict;
    }
    return predict;
}

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_lower_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key){
    //using _Comp = bool* (_Val&, _Val&);
    return exponential_search_lower_bound<RandomIter, _Val>(first, last, predict, key, std::less<_Val>());
}

// TODO: change lower bound to upper bound
template<typename RandomIter, typename _Val, typename _Comp>
inline RandomIter exponential_search_upper_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key, _Comp comp=std::less<_Val>()){
    //if (*predict == key) return predict;
    //bool flag = (key < *predict) ? -1 : 1;
    size_t offset = 1;
    if (comp(key, *predict)){
        for (; predict - offset >= first && !comp(*(predict - offset), key); offset <<= 1);
        offset >>= 1;
        predict -= offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict - offset >= first && !comp(*(predict - offset), key) ) predict -= offset;
    }
    else{
        for (; predict + offset < last && !comp(*(predict + offset), key); offset <<= 1);
        offset >>= 1;
        predict += offset;
        for (offset <<= 1; offset; offset <<= 1)
            if (predict + offset < last && !comp(*(predict + offset), key))
                predict += offset <<= 1;
        if (comp(key, *predict)) ++predict;
    }
    return predict;
}

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_upper_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key){
    return exponential_search_upper_bound(first, last, predict, key, std::less<_Val>());
}


}