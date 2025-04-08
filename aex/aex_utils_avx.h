#pragma once
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <immintrin.h>

namespace aex{

__m256i static forceinline _mm256_cmpge_epu32(__m256i a, __m256i b) {
    return _mm256_cmpeq_epi32(_mm256_max_epu32(a, b), a);
}

__m256i static forceinline _mm256_cmple_epu32(__m256i a, __m256i b) {
    return _mm256_cmpge_epu32(b, a);
}

__m256i static forceinline _mm256_cmpgt_epu32(__m256i a, __m256i b) {
    return _mm256_xor_si256(_mm256_cmple_epu32(a, b), _mm256_set1_epi32(-1));
}

__m256i static forceinline _mm256_cmplt_epu32(__m256i a, __m256i b) {
    return _mm256_cmpgt_epu32(b, a);
}

__m256i static forceinline _mm256_cmpgt_epu64(__m256i a, __m256i b) {  
    static __m256i highBit = _mm256_set1_epi64x((long long)0x8000000000000000);   
    a = _mm256_xor_si256(a, highBit);
    b = _mm256_xor_si256(b, highBit);
    return _mm256_cmpgt_epi64(a, b);
}

__m256i static forceinline _mm256_cmplt_epu64(__m256i a, __m256i b) {  
    return _mm256_cmpgt_epu64(b, a);
}

__m256i static forceinline _mm256_cmplt_epi64(__m256i a, __m256i b) {  
    return _mm256_cmpgt_epi64(b, a);
}

__m256i static forceinline _mm256_cmplt_epi32(__m256i a, __m256i b) {  
    return _mm256_cmpgt_epi32(b, a);
}

__m256i static forceinline _mm256_cmpge_epu64(__m256i a, __m256i b) {  
    return _mm256_xor_si256(_mm256_cmplt_epu64(a, b), _mm256_set1_epi32(-1));
}

__m256i static forceinline _mm256_cmpge_epi64(__m256i a, __m256i b){
    return _mm256_xor_si256(_mm256_cmplt_epi64(a, b), _mm256_set1_epi32(-1));
}

__m256i static forceinline _mm256_cmpge_epi32(__m256i a, __m256i b){
    return _mm256_xor_si256(_mm256_cmplt_epi32(a, b), _mm256_set1_epi32(-1));
}

__m256i static forceinline _mm256_cmple_epu64(__m256i a, __m256i b) {  
    return _mm256_cmpge_epu64(b, a);
}

template<typename _Tp,
        int ERROR_BOUND>
inline _Tp* lower_bound_with_error_bound(_Tp *first, _Tp* last, _Tp x){
    if (last - first > ERROR_BOUND) last = first + ERROR_BOUND;
    for (_Tp* i = first; i < last; ++i)
        if (x <= *i)
            return i;
}

template<>
inline double* lower_bound_with_error_bound<double, 8>(double* first, double* last, double x){
    if (last - first <= 8){
        for (double* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256d key = _mm256_set1_pd(x);
    __m256d v0 = _mm256_loadu_pd(first);
    __m256d v1 = _mm256_loadu_pd(first + 4);
    __m256d cmp0d = _mm256_cmp_pd(v0, key, _CMP_LT_OS);
    __m256d cmp1d = _mm256_cmp_pd(v1, key, _CMP_LT_OS);
    __m256i cmp0 = _mm256_castpd_si256(cmp0d);
    __m256i cmp1 = _mm256_castpd_si256(cmp1d);
    __m256i packs = _mm256_packs_epi32(cmp0, cmp1);
    int res = _mm256_movemask_epi8(packs);
    res = __builtin_popcount(res) >> 2;
    return first + res;
}

template<>
inline float* lower_bound_with_error_bound<float, 8>(float* first, float* last, float x){
    if (last - first <= 8){
        for (float* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256 key = _mm256_set1_ps(x);
    __m256 v0 = _mm256_loadu_ps(first);
    __m256 cmp0 = _mm256_cmp_ps(v0, key, _CMP_LT_OS);
    int res = _mm256_movemask_ps(cmp0);
    res = __builtin_popcount(res);
    return first + res;
}

template<>
inline unsigned long long* lower_bound_with_error_bound<unsigned long long, 8>(unsigned long long* first, unsigned long long* last, unsigned long long x){
    if (last - first <= 8){
        for (unsigned long long* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256i key = _mm256_set1_epi64x(x);
    __m256i v0 = _mm256_loadu_si256((__m256i*)first);
    __m256i v1 = _mm256_loadu_si256((__m256i*)(first + 4));
    __m256i cmp0 = _mm256_cmplt_epu64(v0, key);
    __m256i cmp1 = _mm256_cmplt_epu64(v1, key);
    __m256i packs = _mm256_packs_epi32(cmp0, cmp1);
    int res = _mm256_movemask_epi8(packs);
    res = __builtin_popcount(res) >> 2;
    return first + res;
}

template<>
inline unsigned int* lower_bound_with_error_bound<unsigned int, 8>(unsigned int* first, unsigned int* last, unsigned int x){
    if (last - first <= 8){
        for (unsigned int* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256i key = _mm256_set1_epi32(x);
    __m256i v0 = _mm256_loadu_si256((__m256i*)first);
    __m256i cmp0 = _mm256_cmplt_epu32(v0, key);
    int res = _mm256_movemask_epi8(cmp0);
    res = __builtin_popcount(res) >> 2;
    return first + res;
}

template<>
inline long long* lower_bound_with_error_bound<long long, 8>(long long* first, long long* last, long long x){
    if (last - first <= 8){
        for (long long* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256i key = _mm256_set1_epi64x(x);
    __m256i v0 = _mm256_loadu_si256((__m256i*)first);
    __m256i v1 = _mm256_loadu_si256((__m256i*)(first + 4));
    __m256i cmp0 = _mm256_cmplt_epi64(v0, key);
    __m256i cmp1 = _mm256_cmplt_epi64(v1, key);
    __m256i packs = _mm256_packs_epi32(cmp0, cmp1);
    int res = _mm256_movemask_epi8(packs);
    res = __builtin_popcount(res) >> 2;
    return first + res;
}

template<>
inline int* lower_bound_with_error_bound<int, 8>(int* first, int* last, int x){
    if (last - first <= 8){
        for (int* i = first; i < last; ++i)
            if (x <= *i)
                return i;
    }
    __m256i key = _mm256_set1_epi32(x);
    __m256i v0 = _mm256_loadu_si256((__m256i*)first);
    __m256i cmp0 = _mm256_cmplt_epi32(v0, key);
    int res = _mm256_movemask_epi8(cmp0);
    res = __builtin_popcount(res) >> 2;
    return first + res;
}

template<int K>
inline int cmp_eq_epi8(const unsigned char* x, const unsigned char y){
    int mask = 0;
    for (int i = 0; i < K; ++i) mask |= (1 << i);
    return mask;
}

inline int cmp_eq_epi8(const unsigned char* x, int size, const unsigned char y){
    int mask = 0;
    for (int i = 0; i < size; ++i)
        if (x[i] == y) mask |= (1 << i);
    return mask;
}

//template<>
//inline int cmp_eq_epi8<8>(const unsigned char* x, const char y){
//    //__m128i q = _mm_set1_epi8(y);
//    //__m128i k = _mm_loadu_si128((__m128i*)x);
//    __m64 q = _mm_set1_epi8(y);
//    //__mm64 k = _mm_
//    __m128i r = _mm_cmpeq_pi8(y, (_mm64)x);
//    int mask = _mm_movemask_pi8(r);
//    return mask;
//}

template<>
inline int cmp_eq_epi8<16>(const unsigned char* x, const unsigned char y){
    __m128i q = _mm_set1_epi8(y);
    __m128i k = _mm_loadu_si128((__m128i*)x);
    __m128i r = _mm_cmpeq_epi8(q, k);
    int mask = _mm_movemask_epi8(r);
    return mask;
}

template<>
inline int cmp_eq_epi8<32>(const unsigned char* x, const unsigned char y){
    __m256i q = _mm256_set1_epi8(y);
    __m256i k = _mm256_loadu_si256((__m256i*)x);
    __m256i r = _mm256_cmpeq_epi8(q, k);
    int mask = _mm256_movemask_epi8(r);
    return mask;
}


template<typename _Tp>
inline int cmp_eq_epi64x16(const _Tp* x, const _Tp y){
    __m512i q = _mm512_set1_epi64(y);
    __m512i k1 = _mm512_loadu_si512((__m512i*)x);
    __m512i k2 = _mm512_loadu_si512((__m512i*)(x + 8));
    __mmask8 r1 = _mm512_cmpeq_epi64_mask(q, k1);
    __mmask8 r2 = _mm512_cmpeq_epi64_mask(q, k2);
    //int mask = (_mm512_movemask_epi8(r1) << 8) | ();
    int mask = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    return __builtin_ctz(mask);
}

template<>
inline int cmp_eq_epi64x16(const double* x, const double y){
    __m512d q = _mm512_set1_pd(y);
    __m512d k1 = _mm512_loadu_pd(x);
    __m512d k2 = _mm512_loadu_pd((x + 8));
    __mmask8 r1 = _mm512_cmpeq_pd_mask(q, k1);
    __mmask8 r2 = _mm512_cmpeq_pd_mask(q, k2);
    //int mask = (_mm512_movemask_epi8(r1) << 8) | ();
    int mask = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    return __builtin_ctz(mask);
}

template<typename _Tp1, typename _Tp2>
inline int cmp_eq_epi64x16x2(const _Tp1* x, const _Tp1 tx, const _Tp2* y, const _Tp2 ty){
    __m512i q = _mm512_set1_epi64(reinterpret_cast<unsigned long long>(tx));
    __m512i k1 = _mm512_loadu_si512((__m512i*)x);
    __m512i k2 = _mm512_loadu_si512((__m512i*)(x + 8));
    __mmask8 r1 = _mm512_cmpeq_epi64_mask(q, k1);
    __mmask8 r2 = _mm512_cmpeq_epi64_mask(q, k2);
    //int mask = (_mm512_movemask_epi8(r1) << 8) | ();
    int mask_1 = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    q = _mm512_set1_epi64(ty);
    k1 = _mm512_loadu_si512((__m512i*)y);
    k2 = _mm512_loadu_si512((__m512i*)(y + 8));
    r1 = _mm512_cmpeq_epi64_mask(q, k1);
    r2 = _mm512_cmpeq_epi64_mask(q, k2);
    int mask_2 = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    mask_1 &= mask_2;
    return __builtin_ctz(mask_1);
}

template<typename _Tp1, typename _Tp2>
inline int cmp_eq_epi64x8x2(const _Tp1* x, const _Tp1 tx, const _Tp2* y, const _Tp2 ty){
    __m512i q = _mm512_set1_epi64(reinterpret_cast<unsigned long long>(tx));
    __m512i k1 = _mm512_loadu_si512((__m512i*)x);
    __m512i k2 = _mm512_loadu_si512((__m512i*)(x + 8));
    __mmask8 r1 = _mm512_cmpeq_epi64_mask(q, k1);
    __mmask8 r2 = _mm512_cmpeq_epi64_mask(q, k2);
    //int mask = (_mm512_movemask_epi8(r1) << 8) | ();
    int mask_1 = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    q = _mm512_set1_epi64(ty);
    k1 = _mm512_loadu_si512((__m512i*)y);
    k2 = _mm512_loadu_si512((__m512i*)(y + 8));
    r1 = _mm512_cmpeq_epi64_mask(q, k1);
    r2 = _mm512_cmpeq_epi64_mask(q, k2);
    int mask_2 = ((unsigned int)(r2) << 8) | (unsigned int)r1;
    mask_1 &= mask_2;
    return __builtin_ctz(mask_1);
}

template<typename _Tp>
inline _Tp* linear_search_lower_bound(_Tp* first, _Tp* last, const _Tp& key){
    for (; first < last && key > *first; ++first);
    return first;
}

template<typename _Tp>
inline _Tp* linear_search_upper_bound(const _Tp* first, const _Tp* last, const _Tp& key){
    for (; first < last && key >= *first; ++first);
    return const_cast<_Tp*>(first);
}

// ========== linear_search_lower_bound_avx512x16 ========== 
template<typename _Tp>
inline int linear_search_lower_bound_avx512x16(const _Tp* first, const int size, const _Tp x){
    return std::lower_bound(first, first + size, x) - first;
}

template<>
inline int linear_search_lower_bound_avx512x16(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __mmask8 cmp0 = _mm512_cmpgt_epu64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpgt_epu64_mask(key, v1);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x16(const long long *first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __mmask8 cmp0 = _mm512_cmpgt_epi64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpgt_epi64_mask(key, v1);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x16(const double *first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __m512d v1 = _mm512_loadu_pd(first + 8);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GT_OQ);
    __mmask8 cmp1 = _mm512_cmp_pd_mask(key, v1, _CMP_GT_OQ);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

// ========== linear_search_upper_bound_avx512x32 ========== 

template<typename _Tp>
inline int linear_search_lower_bound_avx512x32(const _Tp* first, const int size, const _Tp x){
    return std::lower_bound(first, first + size, x) - first;
}

template<>
inline int linear_search_lower_bound_avx512x32(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512(first);
    __m512i v1 = _mm512_loadu_si512(first + 8);
    __m512i v2 = _mm512_loadu_si512(first + 16);
    __m512i v3 = _mm512_loadu_si512(first + 24);
    __mmask8 cmp0 = _mm512_cmpgt_epu64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpgt_epu64_mask(key, v1);
    __mmask8 cmp2 = _mm512_cmpgt_epu64_mask(key, v2);
    __mmask8 cmp3 = _mm512_cmpgt_epu64_mask(key, v3);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x32(const long long* first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __m512i v2 = _mm512_loadu_si512((__m512i*)(first + 16));
    __m512i v3 = _mm512_loadu_si512((__m512i*)(first + 24));
    __mmask8 cmp0 = _mm512_cmpgt_epi64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpgt_epi64_mask(key, v1);
    __mmask8 cmp2 = _mm512_cmpgt_epi64_mask(key, v2);
    __mmask8 cmp3 = _mm512_cmpgt_epi64_mask(key, v3);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x32(const double* first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __m512d v1 = _mm512_loadu_pd(first + 8);
    __m512d v2 = _mm512_loadu_pd(first + 16);
    __m512d v3 = _mm512_loadu_pd(first + 24);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GT_OQ);
    __mmask8 cmp1 = _mm512_cmp_pd_mask(key, v1, _CMP_GT_OQ);
    __mmask8 cmp2 = _mm512_cmp_pd_mask(key, v2, _CMP_GT_OQ);
    __mmask8 cmp3 = _mm512_cmp_pd_mask(key, v3, _CMP_GT_OQ);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

// ========== linear_search_lower_bound_avx512x8 ========== 

template<typename _Tp>
inline int linear_search_lower_bound_avx512x8(const _Tp* first, const int size, const _Tp x){
    return linear_search_lower_bound<const _Tp>(first, first + size, x) - first;
}


template<>
inline int linear_search_lower_bound_avx512x8(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __mmask8 cmp0 = _mm512_cmpgt_epu64_mask(key, v0);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x8(const long long* first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __mmask8 cmp0 = _mm512_cmpgt_epi64_mask(key, v0);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_lower_bound_avx512x8(const double* first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GT_OQ);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<int K, typename _Tp>
inline int linear_search_lower_bound_avx512(const _Tp* first, const int size, const _Tp x){
    if constexpr (K != 8 || sizeof(_Tp) != 8){
        return linear_search_lower_bound<const _Tp>(first, first + size, x) - first;
    }
    else if constexpr (K == 8)
        return linear_search_lower_bound_avx512x8(first, size, x);
    else if constexpr (K == 16)
        return linear_search_lower_bound_avx512x16(first, size, x);
    else if constexpr (K == 32)
        return linear_search_lower_bound_avx512x32(first, size, x);
}



// ========== linear_search_upper_bound_avx512x16 ========== 

template<typename _Tp>
inline int linear_search_upper_bound_avx512x16(const _Tp* first, const int size, const _Tp x){
    return std::upper_bound(first, first + size, x) - first;
}

template<>
inline int linear_search_upper_bound_avx512x16(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __mmask8 cmp0 = _mm512_cmpge_epu64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpge_epu64_mask(key, v1);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x16(const long long *first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __mmask8 cmp0 = _mm512_cmpge_epi64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpge_epi64_mask(key, v1);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x16(const double *first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __m512d v1 = _mm512_loadu_pd(first + 8);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GE_OQ);
    __mmask8 cmp1 = _mm512_cmp_pd_mask(key, v1, _CMP_GE_OQ);
    unsigned int mask = (((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}



// ========== linear_search_upper_bound_avx512x32 ========== 

template<typename _Tp>
inline int linear_search_upper_bound_avx512x32(const _Tp* first, const int size, const _Tp x){
    return std::upper_bound(first, first + size, x) - first;
}

template<>
inline int linear_search_upper_bound_avx512x32(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __m512i v2 = _mm512_loadu_si512((__m512i*)(first + 16));
    __m512i v3 = _mm512_loadu_si512((__m512i*)(first + 24));
    __mmask8 cmp0 = _mm512_cmpge_epu64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpge_epu64_mask(key, v1);
    __mmask8 cmp2 = _mm512_cmpge_epu64_mask(key, v2);
    __mmask8 cmp3 = _mm512_cmpge_epu64_mask(key, v3);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x32(const long long* first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __m512i v1 = _mm512_loadu_si512((__m512i*)(first + 8));
    __m512i v2 = _mm512_loadu_si512((__m512i*)(first + 16));
    __m512i v3 = _mm512_loadu_si512((__m512i*)(first + 24));
    __mmask8 cmp0 = _mm512_cmpge_epi64_mask(key, v0);
    __mmask8 cmp1 = _mm512_cmpge_epi64_mask(key, v1);
    __mmask8 cmp2 = _mm512_cmpge_epi64_mask(key, v2);
    __mmask8 cmp3 = _mm512_cmpge_epi64_mask(key, v3);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x32(const double* first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __m512d v1 = _mm512_loadu_pd(first + 8);
    __m512d v2 = _mm512_loadu_pd(first + 16);
    __m512d v3 = _mm512_loadu_pd(first + 24);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GE_OQ);
    __mmask8 cmp1 = _mm512_cmp_pd_mask(key, v1, _CMP_GE_OQ);
    __mmask8 cmp2 = _mm512_cmp_pd_mask(key, v2, _CMP_GE_OQ);
    __mmask8 cmp3 = _mm512_cmp_pd_mask(key, v3, _CMP_GE_OQ);
    unsigned int mask = (((unsigned int)cmp3 << 24) | ((unsigned int)cmp2 << 16) | ((unsigned int)cmp1 << 8) | (unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

// ========== linear_search_upper_bound_avx512x8 ========== 

template<typename _Tp>
inline int linear_search_upper_bound_avx512x8(const _Tp* first, const int size, const _Tp x){
    return linear_search_upper_bound(first, first + size, x) - first;
}

template<>
inline int linear_search_upper_bound_avx512x8(const unsigned long long* first, const int size, const unsigned long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __mmask8 cmp0 = _mm512_cmpge_epu64_mask(key, v0);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x8(const long long* first, const int size, const long long x){
    __m512i key = _mm512_set1_epi64(x);
    __m512i v0 = _mm512_loadu_si512((__m512i*)(first));
    __mmask8 cmp0 = _mm512_cmpge_epi64_mask(key, v0);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<>
inline int linear_search_upper_bound_avx512x8(const double* first, const int size, const double x){
    __m512d key = _mm512_set1_pd(x);
    __m512d v0 = _mm512_loadu_pd(first);
    __mmask8 cmp0 = _mm512_cmp_pd_mask(key, v0, _CMP_GE_OQ);
    unsigned int mask = ((unsigned int)cmp0) & ((1 << size) - 1);
    return __builtin_popcount(mask);
}

template<int K, typename _Tp>
inline int linear_search_upper_bound_avx512(const _Tp* first, const int size, const _Tp x){
    if constexpr (K == 8)
        return linear_search_upper_bound_avx512x8(first, size, x);
    else if constexpr (K == 16)
        return linear_search_upper_bound_avx512x16(first, size, x);
    else if constexpr (K == 32)
        return linear_search_upper_bound_avx512x32(first, size, x);
    else{
        AEX_ASSERT(0 == 1);
    }
}

template<typename _Tp>
inline void move_avx256(_Tp* src, _Tp* dst){
    __m256i src_vec = _mm256_loadu_si256((__m256i*)src);
    _mm256_storeu_si256((__m256i*)dst, src_vec);
}

template<typename _Tp>
inline void move_avx512(_Tp* src, _Tp* dst){
    __m512i src_vec = _mm512_loadu_si512(src);
    _mm512_storeu_si512(dst, src_vec);
}

template<int K>
inline void memmove_avx512(char *src, char* dst){
    static_assert(K % 64 == 0, "must 512bit size");
    if constexpr (K == 64){
        __m512i src_vec = _mm512_loadu_si512(src);
        _mm512_storeu_si512(dst, src_vec);
    }
    else if constexpr (K == 128){
        __m512i v0 = _mm512_loadu_si512(src);
        __m512i v1 = _mm512_loadu_si512(src + 64);
        _mm512_storeu_si512(dst, v0);
        _mm512_storeu_si512(dst + 64, v1);
    }
    else{
        for (int i = 0; i < K; i += 64){
            __m512i src_vec = _mm512_loadu_si512(src);
            _mm512_storeu_si512(dst, src_vec);
        }
    }
}

template<int K, typename _Tp>
inline void move_item_avx(_Tp* src, _Tp* dst){
    static_assert(K == 4 || K == 8 || K == 16, "move_avx support 4 or 8 or 16 now");
    if constexpr (K == 8){
        move_avx512(src, dst);
    }
    else if constexpr (K == 4){
        move_avx256(src, dst);
    }
    else if constexpr (K == 16){
        move_avx512(src, dst);
        move_avx512(src + 8, dst + 8);
    }
    else{
        AEX_ASSERT(0 == 1);
    }
}

template<typename key_type, typename value_type>
void pack_pair_avxx8(key_type *key, value_type *value, std::pair<key_type, value_type>* results){
    __m512i v0 = _mm512_loadu_si512(key);
    __m512i v1 = _mm512_loadu_si512(value);
    __m512i index0 = _mm512_set_epi64(0b1011, 0b0011, 0b1010, 0b0010, 0b1001, 0b0001, 0b1000, 0b0000);
    __m512i index1 = _mm512_set_epi64(0b1111, 0b0111, 0b1110, 0b0110, 0b1101, 0b0101, 0b1100, 0b0100);
    __m512i result0 = _mm512_permutex2var_epi64(v0, index0, v1);
    __m512i result1 = _mm512_permutex2var_epi64(v0, index1, v1);
    _mm512_storeu_epi64(results, result0);
    _mm512_storeu_epi64(results + 4, result1);
}

long long avx_sum_32(long long *x, int size){
    __m512i res = _mm512_set1_epi64(0);
    for (int i = 0; i < size; i += 8){
        __m512i v0 = _mm512_loadu_epi64(x + i);
        res = _mm512_add_epi64(res, v0);
    }
    long long result = _mm512_reduce_add_epi64(res);
    return result;
}


}

