#pragma once
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <atomic>
#include <immintrin.h>
#include <sched.h>
#include <mutex>
#include <random>
namespace aex{

#if !defined(forceinline)
#ifdef _MSC_VER
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define forceinline inline __attribute__((__always_inline__))
#else
#define forceinline inline
#endif
#else
#define forceinline inline
#endif
#endif

#define UNF 0xFFFFFFFFFFFFFFFFLL

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

typedef unsigned long long ULL;
typedef long long LL;

struct AEX_LOG{
    AEX_LOG(){
        #ifdef AEX_DEBUG
        ++recursive_cnt;
        #endif
    }
    void operator()(const char* File, int Line, const char* Function, std::string x){
        #ifdef AEX_DEBUG
        for (int i = 0; i < recursive_cnt - 1; ++i)
            std::cout << "| ";
        std::cout << x;
        std::cout << "( File: " << File << ":" << Line << ", Function:" << Function << ")" << std::endl;
        #endif
    }
    void operator()(const char* font, const char* name, const char* File, int Line, const char* Function, std::string x){
        #ifdef AEX_DEBUG
        std::cout << font;
        for (int i = 0; i < recursive_cnt - 1; ++i)
            std::cout << "| ";
        std::cout << x;
        std::cout << "( [" << name << "] File: " << File << ":" << Line << ", Function:" << Function << ")" << WHITE_FONT_TAG << std::endl;
        #endif
    }
    ~AEX_LOG(){
        #ifdef AEX_DEBUG
        --recursive_cnt;
        #endif
    }
    static int recursive_cnt;
};

#ifdef AEX_DEBUG

static std::mutex log_mutex;

//#define private public

#define AEX_PRINT(x)  do { aex::log_mutex.lock(); std::cout << "File: " << __FILE__ << ":" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << std::endl; aex::log_mutex.unlock();} while(0)
//#define AEX_PRINT(...)  do { ____(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while(0)

#define AEX_PRINT_TAG(x, TAG_FONT, TAG_NAME)  do { aex::log_mutex.lock(); std::cout << TAG_FONT << TAG_NAME << " File: " << __FILE__ << ":" << __LINE__ << ", Function:" << __FUNCTION__ << ", output:" << x << WHITE_FONT_TAG << std::endl; aex::log_mutex.unlock(); } while(0)
//#define AEX_PRINT_TAG(x, TAG_FONT, TAG_NAME)  do { ____(TAG_FONT, TAG_NAME, __FILE__, __LINE__, __FUNCTION__); } while(0)

#define AEX_FORMAT(FORMAT, ...) do{ printf("File: %s:%d, Function: %s, output: ", __FILE__, __LINE__, __FUNCTION__); printf(FORMAT, ##__VA_ARGS__); printf("\n"); fflush(stdout);} while(0);

#define AEX_PRINT_ELEMENT(x) do { AEX_PRINT(##x << "=" << x); } while(0)

#else

#define AEX_PRINT(x) 

#define AEX_PRINT_TAG(x, TAG_FONT, TAG_NAME) 

#define AEX_FORMAT(FORMAT, ...) 

#define AEX_PRINT_ELEMENT(x) 

#define AEX_DEBUG_BLOCK(x)

#endif

#ifdef AEX_DEBUG_ASSERT
#define AEX_ASSERT(x) do { assert(x); } while(0)
#define AEX_DEBUG_BLOCK(x) do { x } while(0)

#else
#define AEX_ASSERT(x)
#define AEX_DEBUG_BLOCK(x)

#endif

#define AEX_WARNING(x) AEX_PRINT_TAG(x, YELLOW_FONT_TAG, "[WARNING]")

#define AEX_ERROR(x) AEX_PRINT_TAG(x, RED_FONT_TAG, "[ERROR]")

#define AEX_SUCCESS(x) AEX_PRINT_TAG(x, GREEN_FONT_TAG, "[SUCCESS]")

#define AEX_HINT(x) AEX_PRINT_TAG(x, BLUE_FONT_TAG, "[HINT]")

#define AEX_IMPORTANT(x) AEX_PRINT_TAG(x, PURPLE_FONT_TAG, "[IMPORTANT]")


#ifdef AEX_DEBUG_THREAD
#define AEX_SGL_ASSERT(x)        AEX_ASSERT(x)
#define AEX_MUL_ASSERT(x)        AEX_DEBUG_BLOCK({ if constexpr (traits::AllowConcurrency) do { AEX_ASSERT(x); } while(0);})
#define AEX_SGL_DEBUG_BLOCK(x)   AEX_DEBUG_BLOCK({ if constexpr (!traits::AllowConcurrency) do { x } while(0);})
#define AEX_MUL_DEBUG_BLOCK(x)   AEX_DEBUG_BLOCK({ if constexpr (traits::AllowConcurrency) do { x } while(0);})
#define DEBUG_CHECK_LOCK(node)   AEX_ASSERT(check_lock(node));
#define DEBUG_CHECK_UNLOCK(node) AEX_ASSERT(check_unlock(node));
#else
#define AEX_SGL_ASSERT(x)
#define AEX_MUL_ASSERT(x)
#define AEX_SGL_DEBUG_BLOCK(x)
#define AEX_MUL_DEBUG_BLOCK(x)
#define DEBUG_CHECK_LOCK(node)
#define DEBUG_CHECK_UNLOCK(node)
#endif

#define n_n(node) static_cast<node_ptr>(node)
#define i_n(node) static_cast<inner_node_ptr>(node)
#define h_n(node) static_cast<hash_node_ptr>(node)
#define d_n(node) static_cast<dense_node_ptr>(node)
#define l_n(node) static_cast<data_node_ptr>(node)
#define r_h_n(node) reinterpret_cast<hash_node_ptr>(node)
#define r_d_n(node) reinterpret_cast<dense_node_ptr>(node)
#define r_l_n(node) reinterpret_cast<data_node_ptr>(node)

//#define CACHELINE_SIZE 64
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

inline void _yield(int count){
    if (count>3)
        sched_yield();
    else
        _mm_pause();
}

enum class NodeType{
    LeafNode,
    DenseNode,
    HashNode,
};

inline std::string to_string(NodeType type){
    switch (type){
        case NodeType::LeafNode  : { return "LeafNode";  }
        case NodeType::DenseNode : { return "DenseNode"; }
        case NodeType::HashNode  : { return "HashNode";  }
        default : { return "Unknown"; }
    }
}

template<typename _Tp>
inline _Tp highbit_64(const _Tp &x){
    return (x + 63) & (~63);
}

template<typename _Tp, int K>
inline _Tp highbit(const _Tp &x){
    return (x + (K - 1)) & (~(K - 1));
}

//template<typename _Tp, int K>
//class _pos2slot{
//public:
//    inline _Tp operator()(const _Tp pos) const{
//        return pos >> K;
//    }
//};
template<typename _Tp>
inline constexpr _Tp pos2slot(const _Tp pos) {
    return pos >> 6;
}

template<typename _Tp, int K>
union data_align_copy{
    unsigned char data[K];
    _Tp pointer;
};

//template<typename _Tp>
//inline _Tp pos2slot(const _Tp pos) {
//    return pos >> 6;
//}

template<typename _Tp>
inline _Tp rapid_pow(_Tp base, unsigned long long x){
    _Tp ans = 1;
    for (;x>0; x >>= 1){
        if (x & 1) ans *= base;
        base *= base;
    }
    return ans;
}

template<typename _Tp, typename _T>
inline _Tp min_slot_size(const _Tp x, const _T min_slot_size){
    _Tp slot_size = min_slot_size;
    while (slot_size < x) slot_size <<= 1;
    return slot_size;
}

template<typename _Tp, typename _T>
inline _Tp min_slot_size(const _Tp x, double ratio, const _T min_slot_size){
    _Tp slot_size = min_slot_size;
    while (slot_size * ratio < x) slot_size <<= 1;
    return slot_size;
}

template<typename K,
        typename V>
inline void sample_sort(K* k, V* v, int n){
    for (int i = 0; i < n - 1; ++i){
        int t = i;
        for (int j = i + 1; j < n; ++j)
            if (k[t] > k[j])
                t = j;
        std::swap(k[i], k[t]);
        std::swap(v[i], v[t]);
    }
}

template<typename _Tp>
_Tp sqr(const _Tp &x){return x * x;}

inline ULL align_8bytes(ULL x){
    return ((x + 7) & (~7) );
}

/* I don't know if it's speed up */
template<int x>
inline int lowbit_loop_unroll(int k){
    if (k & 1) return x;
    return lowbit_loop_unroll<x-1>(k >> 1);
}

//template<typename traits, bool _ = traits::AllowConcurrecny>
template<typename traits>
class aex_bitmap_impl{
public:
    typedef typename traits::bitmap_base bitmap_base;

    typedef typename traits::bitmap bitmap;

    typedef typename traits::slot_type slot_type;

    static inline void set_one(bitmap text, const slot_type x) {
        text[x >> 6] |= (1LL << (x & 63));
    }
    static inline void set_zero(bitmap text, const slot_type x){
        text[x >> 6] &= ~(1LL << (x & 63));
    }
    static inline bool at(const bitmap text, const slot_type x){
        return ((text[x >> 6] >> (x & 63)) & 1);
    }
};

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_lower_bound(RandomIter first, RandomIter last, RandomIter predict, _Val& key){
    AEX_ASSERT(first <= predict);
    AEX_ASSERT(predict < last);
    ULL offset = 1;
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
    ULL offset = 1;
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

template<typename RandomIter, typename _Val>
inline RandomIter exponential_search_lower_bound(RandomIter predict, RandomIter last,  _Val& key){
    AEX_ASSERT(predict < last);
    ULL offset = 8;
    //for (; predict + offset < last && *(predict + offset) < key; offset <<= 1);
    for (offset >>= 1; offset; offset >>= 1)
        if (predict + offset < last && *(predict + offset) < key)
            predict += offset;
    ++predict;
    return predict;
}
//template<typename RandomIter, typename _Val>
//inline RandomIter exponential_search_upper_bound(RandomIter predict, RandomIter last, _Val& key){
//    ULL offset = 1;
//    AEX_ASSERT(predict < last);
//    for (; predict + offset < last && *(predict + offset) <= key; offset <<= 1);
//    for (offset >>= 1; offset; offset >>= 1)
//        if (predict + offset < last && *(predict + offset) <= key) predict += offset;
//    ++predict;
//
//    return predict;
//}

inline double cross_product(double x0, double y0, double x1, double y1, double x2, double y2){
    return (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
}

template<typename _Tp>
inline _Tp MID_KEY(_Tp x, _Tp y){
    return (x + y) / 2;
}

template<>
inline unsigned long long MID_KEY(unsigned long long x, unsigned long long y){
    return (x >> 1) + (y >> 1) + (x&y&1);
}

template<>
inline long long MID_KEY(long long x, long long y){
    return (x / 2) + (y / 2) + (x&y&1);
}

template<typename _Tp>
inline std::pair<_Tp, size_t> argmax(_Tp* x, ULL n){
    _Tp v = x[0];
    ULL p = 0;
    for (ULL i = 1; i < n; ++i)
    if (x[i] > v){
        v = x[i];
        p = i;
    }
    return std::make_pair(v, p);
}

template<typename _Tp>
inline std::pair<_Tp, size_t> argmin(_Tp* x, ULL n){
    _Tp v = x[0];
    ULL p = 0;
    for (ULL i = 1; i < n; ++i)
    if (x[i] > v){
        v = x[i];
        p = i;
    }
    return std::make_pair(v, p);
}

template<typename _Tp>
inline _Tp max(_Tp* x, ULL n){
    _Tp ret = x[0];
    for (ULL i = 1; i < n; ++i)
        ret = std::max(ret, x[i]);
    return ret;
}

template<typename _Tp>
inline _Tp min(_Tp* x, ULL n){
    _Tp ret = x[0];
    for (ULL i = 1; i < n; ++i)
        ret = std::min(ret, x[i]);
    return ret;
}

template<typename _Tp>
inline bool is_sorted(_Tp *x, ULL n){
    for (ULL i = 1; i < n; ++i)
    if (x[i] < x[i - 1])    
        return false;
    return true;
}

inline LL qpow(LL a, LL n, LL p){
    LL ans = 1;
    while (n)
    {
        if (n & 1)
            ans = ans * a % p;
        a = a * a % p;
        n >>= 1;
    }
    return ans;
}

inline bool is_prime(LL x){
    if (x < 3) 
        return x == 2;
    if (x % 2 == 0)
        return false;
    LL A[] = {2, 3, 5, 7}, d = x - 1, r = 0;
    while (d % 2 == 0) 
        d /= 2, ++r;
    for (auto a : A)
    {
        LL v = qpow(a, d, x);
        if (v <= 1 || v == x - 1) 
            continue;
        for (int i = 0; i < r; ++i)
        {
            v = v * v % x;
            if (v == x - 1 && i != r - 1) {
                v = 1;
                break;
            }
            if (v == 1)  
                return false;
        }
        if (v != 1) 
            return false;
    }
    return true;
}

inline double utils_get_hash_key_d(ULL x){
    double A = 0.6180339887;
    double product = x * A;
    return product - static_cast<ULL>(product);
}

inline ULL utils_get_hash_key(ULL x, ULL M){
    double A = 0.6180339887;
    double product = x * A;
    double fractional = product - static_cast<ULL>(product);
    return static_cast<ULL>(M * fractional);
}

template<ULL K1>
inline ULL utils_get_hash_key(ULL x, ULL y, ULL M){
    return utils_get_hash_key(x * K1 + y, M);
}

inline ULL get_randint(ULL x){
    static thread_local std::minstd_rand generator(0);
    std::uniform_int_distribution<ULL> dist(1, x);
    return dist(generator);
}

//template<ULL K1, ULL K2, ULL K3>
//ULL get_hash_key(ULL x, ULL y, ULL z){
//    return x * K1 + y * K2 + z * K3;
//}

struct operation_stats{
    //operation_stats():hash_node_rebuild_cnt(0),  cast_to_hash_node_cnt(0),  hash_node_expand_cnt(0),  hash_node_narrow_cnt(0),
    //                  dense_node_rebuild_cnt(0), cast_to_dense_node_cnt(0), dense_node_expand_cnt(0), dense_node_narrow_cnt(0),
    //                  hash_node_split_cnt(0), hash_node_construct_cnt(0), dense_node_split_cnt(0), dense_node_construct_cnt(0),
    //                  data_node_split_cnt(0), data_node_merge_cnt(0){}
    operation_stats() = default;
    ULL cast_to_hash_node_cnt, hash_node_expand_cnt, hash_node_expand_size, hash_node_narrow_cnt, hash_node_narrow_size;
    ULL cast_to_dense_node_cnt, dense_node_expand_cnt, dense_node_expand_size, dense_node_narrow_cnt, dense_node_narrow_size;
    ULL inner_node_rebuild_cnt, inner_node_rebuild_size;
    ULL inner_node_split_cnt, inner_node_split_size, dense_node_split_cnt; 
    ULL get_childs_con_cnt, lock_array_con_cnt, construct_SMO_con_cnt;
    //ULL hash_node_construct_cnt, hash_node_construct_size, dense_node_construct_cnt, dense_node_construct_size;
    ULL data_node_split_cnt, data_node_merge_cnt;
    ULL model_train_cnt, model_train_size;
    ULL allocate_data_node_cnt, allocate_dense_node_cnt, allocate_hash_node_cnt;
    ULL free_data_node_cnt, free_dense_node_cnt, free_hash_node_cnt;
    void print_stats() const {
        AEX_SUCCESS("[Operation Stats]: ");
        AEX_IMPORTANT("cast_to_hash_node_cnt="    << cast_to_hash_node_cnt  << ", cast_to_dense_node_cnt="    << cast_to_dense_node_cnt);
        AEX_IMPORTANT("hash_node_expand_cnt="     << hash_node_expand_cnt   << ", hash_node_expand_size="     << hash_node_expand_size);
        AEX_IMPORTANT("hash_node_narrow_cnt="     << hash_node_narrow_cnt   << ", hash_node_narrow_size="     << hash_node_narrow_size);
        AEX_IMPORTANT("dense_node_expand_cnt="    << dense_node_expand_cnt  << ", dense_node_expand_size="    << dense_node_expand_size);
        AEX_IMPORTANT("dense_node_narrow_cnt="    << dense_node_narrow_cnt  << ", dense_node_narrow_size="    << dense_node_narrow_size);
        AEX_IMPORTANT("inner_node_rebuild_cnt="   << inner_node_rebuild_cnt << ", inner_node_rebuild_size="   << inner_node_rebuild_size);
        AEX_IMPORTANT("inner_node_spilt_cnt="     << inner_node_split_cnt   << ", inner_node_split_size="     << inner_node_split_size << ", dense_node_split_cnt=" << dense_node_split_cnt);
        AEX_IMPORTANT("get_childs_con_cnt="       << get_childs_con_cnt     <<  ", lock_array_con_cnt="       << lock_array_con_cnt << ", construct_SMO_con_cnt=" << construct_SMO_con_cnt);
        AEX_IMPORTANT("data_node_split_cnt="      << data_node_split_cnt    << ", data_node_merge_cnt="       << data_node_merge_cnt);
        AEX_IMPORTANT("model_train_cnt="          << model_train_cnt        << ", model_train_size="          << model_train_size);
        AEX_IMPORTANT("allocate_data_node_cnt="   << allocate_data_node_cnt << ", allocate_dense_node_cnt="   << allocate_dense_node_cnt << ", allocate_hash_node_cnt=" << allocate_hash_node_cnt);
        AEX_IMPORTANT("free_data_node_cnt="       << free_data_node_cnt     << ", free_dense_node_cnt="       << free_dense_node_cnt  << ", free_hash_node_cnt=" << free_hash_node_cnt);
    }
};
struct concurrency_stats{
    std::atomic<LL> insert_restart_cnt, find_restart_cnt, SL_wait_cnt, XL_wait_cnt, array_SL_wait_cnt, array_XL_wait_cnt;
    concurrency_stats(){
        insert_restart_cnt = 0;
        find_restart_cnt = 0;
        SL_wait_cnt = 0; 
        XL_wait_cnt = 0;
        array_SL_wait_cnt = 0;
        array_XL_wait_cnt = 0;
    }

    void print_stats() const {
        AEX_SUCCESS("[Concurrency Operation Stats]: ");
        AEX_IMPORTANT("insert_restart_cnt=" << insert_restart_cnt.load() << ", find_insert_cnt=" << find_restart_cnt.load());
        AEX_IMPORTANT("SL_wait_cnt="   << SL_wait_cnt.load()  << ", XL_wait_cnt=" << XL_wait_cnt.load());
        AEX_IMPORTANT("array_SL_wait_cnt="   << array_SL_wait_cnt.load()  << ", array_XL_wait_cnt=" << array_XL_wait_cnt.load());
    }
};

struct info_stats{
    //infomation_stats():hash_node_cnt(0), dense_node_cnt(0), data_node_cnt(0);
    info_stats() = default;
    ULL hash_node_cnt, dense_node_cnt, try_learn_dense_node_cnt, data_node_cnt;
    ULL hash_node_childs, dense_node_childs;
    ULL tot_depth, size;
    unsigned int max_depth;
    ULL level_node[16];
    ULL memory_used, hash_node_memory_used, dense_node_memory_used, hash_table_memory_used, data_node_memory_used;
    void print_stats() const {
        AEX_SUCCESS("[Infomation Stats]: ");
        AEX_HINT("memory used=" << 1.0 * memory_used / 1024 / 1024 << " (MB), hash node memory used=" << 1.0 * hash_node_memory_used / 1024 / 1024 << " (MB), dense node memory used=" << 1.0 * dense_node_memory_used / 1024 / 1024 << "(MB), hash table memory used=" << 1.0 * hash_table_memory_used / 1024 / 1024 << " (MB), data node memory used=" << 1.0 * data_node_memory_used / 1024 / 1024 << "(MB)");
        AEX_HINT("tot_cnt=" << hash_node_cnt + dense_node_cnt + data_node_cnt);
        //AEX_HINT("hash_node_cnt=" << hash_node_cnt << ", dense_node_cnt=" << dense_node_cnt << ", try_learn_dense_node_cnt" << try_learn_dense_node_cnt << ", data_node_cnt=" << data_node_cnt);
        AEX_HINT("hash_node_cnt=" << hash_node_cnt << ", dense_node_cnt=" << dense_node_cnt << ", data_node_cnt=" << data_node_cnt);
        AEX_HINT("try_learned_dense_node_cnt=" << try_learn_dense_node_cnt);
        AEX_HINT("hash_node_childs=" << hash_node_childs << ", dense_node_childs=" << dense_node_childs);
        AEX_HINT("size=" << size << ", avg_depth=" << 1.0 * tot_depth / size << ", max_depth=" << max_depth);
        for (int i = 0; i < 16; ++i)
        if (level_node[i] > 0)
            AEX_HINT("level " << i << "=" << level_node[i]);
    }
};

}