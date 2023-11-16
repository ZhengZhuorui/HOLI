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

}
