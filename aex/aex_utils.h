#pragma once
#include <atomic>
namespace aex{

#define UNF 0xFFFFFFFFFFFFFFFFLL
//#define RED_FONT(str) ("\033[31m"+(str)+"\033[0m")
//#define GREEN_FONT(str) ("\033[32m"+(str)+"\033[0m")
//#define YELLOW_FONT(str) ("\033[33m"+(str)+"\033[0m")
//#define BLUE_FONT(str) ("\033[34m"+(str)+"\033[0m")

#define WHITE_FONT_TAG "\033[0m"
#define RED_FONT_TAG "\033[31m"
#define GREEN_FONT_TAG "\033[32m"
#define YELLOW_FONT_TAG "\033[33m"
#define BLUE_FONT_TAG "\033[34m"
#define PURPLE_FONT_TAG "\033[35m"

inline std::string RED_FONT(std::string str){ return RED_FONT_TAG + str + WHITE_FONT_TAG; }
inline std::string GREEN_FONT(std::string str){ return GREEN_FONT_TAG + str + WHITE_FONT_TAG; }
inline std::string YELLOW_FONT(std::string str){ return YELLOW_FONT_TAG + str + WHITE_FONT_TAG; }
inline std::string BLUE_FONT(std::string str){ return BLUE_FONT_TAG + str + WHITE_FONT_TAG; }
//inline std::string DARK_GREEN_FONT(std::string str){ return DARK_GREEN_FONT_TAG + str + WHITE_FONT_TAG; }
inline std::string PURPLE_GREEN_FONT(std::string str){ return PURPLE_FONT_TAG + str + WHITE_FONT_TAG; }

#ifdef AEX_DEBUG

//#define private public

#define AEX_PRINT(x)  do { std::cout << "File: " << __FILE__ << ":" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << std::endl; } while(0)

//#define AEX_PRINT_TAG(x, TAG)  do { std::cout << TAG << "File: " << __FILE__ << ":" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << WHITE_FONT_TAG << std::endl; } while(0)

#define AEX_PRINT_TAG(x, TAG_FONT, TAG_NAME)  do { std::cout << TAG_FONT << TAG_NAME << " File: " << __FILE__ << ":" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << WHITE_FONT_TAG << std::endl; } while(0)

#define AEX_FORMAT(FORMAT, ...) do{ printf("File: %s:%d, Function: %s, output: ", __FILE__, __LINE__, __FUNCTION__); printf(FORMAT, ##__VA_ARGS__); printf("\n"); fflush(stdout);} while(0);

#define AEX_ASSERT(x) do { assert(x); } while(0)

#define AEX_PRINT_ELEMENT(x) do { AEX_PRINT(##x << "=" << x); } while(0)

#else

#define AEX_PRINT(x) 

#define AEX_PRINT_TAG(x, TAG_FONT, TAG_NAME) 

#define AEX_FORMAT(FORMAT, ...) 

#define AEX_ASSERT(x) 

#define AEX_PRINT_ELEMENT(x) 

#endif

#define AEX_WARNING(x) AEX_PRINT_TAG(x, YELLOW_FONT_TAG, "[WARNING]")

#define AEX_ERROR(x) AEX_PRINT_TAG(x, RED_FONT_TAG, "[ERROR]")

#define AEX_SUCCESS(x) AEX_PRINT_TAG(x, GREEN_FONT_TAG, "[SUCCESS]")

#define AEX_HINT(x) AEX_PRINT_TAG(x, BLUE_FONT_TAG, "[HINT]")

#define AEX_IMPORTANT(x) AEX_PRINT_TAG(x, PURPLE_FONT_TAG, "[IMPORTANT]")

enum node_property{
    LEAF=0x1,
    ML_NODE=0x2,
    CHECK_MERGE=0x4,
    CHECK_SPLIT=0x8,
    APPEND_NODE=0x10,
    SORTED_NODE=0x20,
};

#define IS_ML_NODE(node_ptr) (((node_ptr)->prop & node_property::ML_NODE) != 0)
#define IS_LEAF_NODE(node_ptr) (((node_ptr)->prop & node_property::LEAF) != 0)

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

    typedef typename traits::slot_type slot_type;

    static inline void set_one(bitmap text, const slot_type x) {
        text[x >> 6] |= (1LL << (x & 63));
    }
    static inline void set_zero(bitmap text, const slot_type x){
        text[x >> 6] &= ~(1LL << (x & 63));
    }
    static inline char at(const bitmap text, const slot_type x){
        return ((text[x >> 6] >> (x & 63)) & 1);
    }

    static inline slot_type next_empty_slot(ULL* text, size_type x){
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
        for (slot_type i = x; i < x + traits::ERROR_BOUND; ++i)
        if (!at(text, i)){
            return i;
        }
        return x + traits::ERROR_BOUND;
    }

};

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_lower_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key){
    AEX_ASSERT(first <= predict);
    AEX_ASSERT(predict < last);
    size_t offset = 1;
    if (key <= *predict){
        for (; predict - offset >= first && key <= *(predict - offset); offset <<= 1);
        for (offset >>= 1; offset; offset >>= 1)
            if (predict - offset >= first && key <= *(predict - offset)) predict -= offset;
    }
    else {
        for (; predict + offset < last && *(predict + offset) < key; offset <<= 1);
        for (offset >>= 1; offset; offset >>= 1)
            if (predict + offset < last && *(predict + offset) < key)
                predict += offset;
        ++predict;
    }
    return predict;
}

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_upper_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key){
    size_t offset = 1;
    AEX_ASSERT(first <= predict);
    AEX_ASSERT(predict < last);
    if (key < *predict){
        for (; predict - offset >= first && key < *(predict - offset); offset <<= 1);
        for (offset >>= 1; offset; offset >>= 1)
            if (predict - offset >= first && key < *(predict - offset)) predict -= offset;
    }
    else {
        for (; predict + offset < last && *(predict + offset) <= key; offset <<= 1);
        for (offset >>= 1; offset; offset >>= 1)
            if (predict + offset < last && *(predict + offset) <= key) predict += offset;
        ++predict;
    }
    return predict;
}

inline double cross_product(double x0, double y0, double x1, double y1, double x2, double y2){
    return (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
}

}