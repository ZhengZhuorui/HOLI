#pragma once
#include "aex_model.h"

namespace aex{
template<typename _Tp,
        typename Model,
        typename traits>
class PDM_AVX{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    //typedef piecewise_linear_model<_Tp, traits> Model;

    PDM_AVX(){}

    ~PDM_AVX(){}

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return Model::name() + "_avx";
    }
    #endif

    // return the predict position. value range from 0 to +inf.
    forceinline double predict(const key_type& key) const {
        double key_d = (double)key;
        double position[4];
        __m256d key_tensor = _mm256_set1_pd(key_d);
        __m256d zero_tensor = _mm256_setzero_pd();
        __m256d position_tensor = _mm256_setzero_pd();

        for (int i = 0; i < traits::MAX_SEGMENT_NUM / 4; ++i){
            __m256d slope_tensor = _mm256_loadu_pd(args.slope + i * 4);
            __m256d end_tensor = _mm256_loadu_pd(args.end + i * 4);
            __m256d gap_tensor = _mm256_sub_pd(key_tensor, end_tensor);
            __m256d mask_tensor = _mm256_min_pd(gap_tensor, zero_tensor);
            __m256d block_position_tensor = _mm256_mul_pd(mask_tensor, slope_tensor);
            position_tensor = _mm256_add_pd(position_tensor, block_position_tensor);
        }
        position_tensor = _mm256_hadd_pd(position_tensor, position_tensor);
        _mm256_storeu_pd(position, position_tensor);
        position[0] = (position[0] + position[2]) + 1;
        return position[0];
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        //AEX_PRINT("avx model train, n=" << n << ", slot_size=" << slot_size);
        Model m;
        //AEX_ASSERT(traits::MAX_SEGMENT_NUM * sizeof(double) <= 256);
        if (m.train(key, n, slot_size) == false)
            return false;
        this->args.seg_nums = m.args.seg_nums;
        //AEX_PRINT("seg_nums=" << m.args.seg_nums);
        for (unsigned int i = 0; i < m.args.seg_nums; ++i){
            this->args.end[i] = static_cast<double>(m.args.end[i]);
            this->args.slope[i] = static_cast<double>(m.args.slope[i]);
        }
        std::fill(this->args.end + this->args.seg_nums, this->args.end + traits::MAX_SEGMENT_NUM, 0);
        std::fill(this->args.slope + this->args.seg_nums, this->args.slope + traits::MAX_SEGMENT_NUM, 0);
        return true;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        long double sum = 0;
        //slot_type max_error = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    struct piecewise_linear_model_arguments{
        double end[traits::MAX_SEGMENT_NUM], slope[traits::MAX_SEGMENT_NUM];
        unsigned int seg_nums;
    }args;

};

template<typename _Tp,
        typename Model,
        typename traits>
class PDM_hash_table_AVX{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    //typedef piecewise_linear_model<_Tp, traits> Model;

    PDM_hash_table_AVX(){}

    ~PDM_hash_table_AVX(){}

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return Model::name() + "_avx";
    }
    #endif

    // return the predict position. value range from 0 to +inf.
    forceinline int predict(const key_type& key) const {
        double key_d = (double)key;
        double position[4];
        __m256d key_tensor = _mm256_set1_pd(key_d);
        __m256d zero_tensor = _mm256_setzero_pd();
        __m256d position_tensor = _mm256_setzero_pd();

        for (int i = 0; i < traits::MAX_SEGMENT_NUM / 4; ++i){
            __m256d slope_tensor = _mm256_loadu_pd(args.slope + i * 4);
            __m256d end_tensor = _mm256_loadu_pd(args.end + i * 4);
            __m256d gap_tensor = _mm256_sub_pd(key_tensor, end_tensor);
            __m256d mask_tensor = _mm256_min_pd(gap_tensor, zero_tensor);
            __m256d block_position_tensor = _mm256_mul_pd(mask_tensor, slope_tensor);
            position_tensor = _mm256_add_pd(position_tensor, block_position_tensor);
        }
        position_tensor = _mm256_hadd_pd(position_tensor, position_tensor);
        _mm256_storeu_pd(position, position_tensor);
        position[0] = (position[0] + position[2]) + this->args.slot_size;
        return (int)(position[0]);
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        //AEX_PRINT("avx model train, n=" << n << ", slot_size=" << slot_size);
        Model m;
        //AEX_ASSERT(traits::MAX_SEGMENT_NUM * sizeof(double) <= 256);
        if (m.train(key, n, slot_size) == false)
            return false;
        this->args.seg_nums = m.args.seg_nums;
        this->args.slot_size = m.args.slot_size;
        //AEX_PRINT("seg_nums=" << m.args.seg_nums);
        for (unsigned int i = 0; i < m.args.seg_nums; ++i){
            this->args.end[i] = static_cast<double>(m.args.end[i]);
            this->args.slope[i] = static_cast<double>(m.args.slope[i]);
        }
        std::fill(this->args.end + this->args.seg_nums, this->args.end + traits::MAX_SEGMENT_NUM, 0);
        std::fill(this->args.slope + this->args.seg_nums, this->args.slope + traits::MAX_SEGMENT_NUM, 0);
        return true;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        long double sum = 0;
        //slot_type max_error = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0, m_error = 0, start = -1;
        for (slot_type i = 0; i < n; ++i){
            slot_type pos = std::max(0, this->predict(key[i]));
            if (pos == start) ++error;
            else error = 0;
            m_error = std::max(m_error, error);
            start = pos;
        }
        return m_error;
    }

    struct piecewise_linear_model_arguments{
        double end[traits::MAX_SEGMENT_NUM], slope[traits::MAX_SEGMENT_NUM];
        unsigned int seg_nums, slot_size;
    }args;

};

}