#pragma once
namespace aex{

enum model_type{
    linear=0,
    exponential=1,
    logarithmic=2
};

// model template class
template<typename _Tp,
        typename traits>
class aex_model_base{
public:
    typedef _Tp key_type;
    typedef typename traits::size_type size_type;
    virtual double predict(const key_type &key) = 0;
    virtual bool train(const key_type* const key, const size_type n) = 0;
};

// linear model
// predict: (key) -> Real[0, 1]
// return (key - start) * slope + intercept(start)
template<typename _Tp,
        typename traits>
class linear_model{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::size_type size_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<size_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return std::fma(args.slope, key - args.start, args.inter);
    }

    // train model with an key array, array size n and slot size
    bool train(const key_type* const key, const size_type n){
        //AEX_ASSERT(n > 1);

        this->args.start = key[0];

        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (size_type  i = 0; i < n; ++i){
            double pos = 1.0 * i / (n - 1);
            double rex = key[i] - this->args.start;
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        //long double fm = sum_x2 - 1.0 * n * sqr(bar_x);
        //if (abs(fm) < 1e-10)
        //    return false;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        
        //AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f start=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter, args.start);
        //AEX_PRINT("train. RMSE=" << this->RMSE(key, n));
        return true;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        double sum = 0;
        //size_type max_error = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
            //max_error = std::max(max_error, static_cast<size_type>(std::abs(this->predict(key[i]) * (n - 1) - i)));
            //AEX_FORMAT("%.4f %lld | ", this->predict(key[i]) * (n - 1), i);
        }
        //AEX_PRINT("max error=" << max_error);
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline size_type max_error(const key_type* const key, const size_type n, const size_type slot_size){
        size_type error = 0;
        for (size_type i = 0, start = 0; i < n; ++i){
            size_type pos = std::max(0LL, static_cast<size_type>(round(this->predict(key[i]) * slot_size)));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    struct linear_arguments{
        double slope, inter, start;
    }args;
};



// quandratic model
// predict: (key) -> Real[0, 1]
// return e^((key - start) * slope + intercept(start))
template<typename _Tp,
        typename traits>
class quandratic_model{
public:
    typedef _Tp key_type;

    typedef quandratic_model<key_type, traits> self;

    typedef typename traits::size_type size_type;

    inline double predict(const key_type &key) const{
        key_type rex = key - args.start;
        return args.quad * sqr(rex) + args.lin * rex + args.inter;
    }

    bool train(const _Tp* const key, const size_type n){
        this->args.start = key[0];
        long double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0, sum_xxx = 0, sum_xxxx = 0, sum_xxy = 0;
        
        for (size_type i = 0; i < n; ++i){
            long double pos = 1.0 * i / (n - 1);
            long double rex = key[i] - this->args.start;
            sum_x += rex;
            sum_xx += rex * rex;
            sum_xxx += rex * rex * rex;
            sum_xxxx += rex * rex * rex * rex;
            
            sum_y += pos;
            sum_xy += rex * pos;
            sum_xxy += rex * rex * pos;
            
        }
        long double matrix[3][4] = {{static_cast<long double>(n), sum_x, sum_xx, sum_y}, {sum_x, sum_xx, sum_xxx, sum_xy}, {sum_xx, sum_xxx, sum_xxxx, sum_xxy}};
        for (int i = 0; i < 3; ++i){
            for (int j = 0; j < 4; ++j) std::cout << matrix[i][j] << " ";
           std::cout << "\n";
        }

        long double ratio = matrix[1][0] / matrix[0][0];
        for (size_type i = 0; i < 4; ++i)
            matrix[1][i] -= ratio * matrix[0][i];
        ratio = matrix[2][0] / matrix[0][0];
        for (size_type i = 0; i < 4; ++i)
            matrix[2][i] -= ratio * matrix[0][i];
        
        //AEX_ASSERT(abs(matrix[1][i]) < 1e-6);
        ratio = matrix[2][1] / matrix[1][1];
        for (size_type i = 0; i < 4; ++i)
            matrix[2][i] -= ratio * matrix[1][i];

        this->args.quad = matrix[2][3] / matrix[2][2];
        this->args.lin = (matrix[1][3] - matrix[1][2] * this->args.quad) / matrix[1][1];
        this->args.inter = (matrix[0][3] - matrix[0][2] * this->args.quad - matrix[0][1] * this->args.lin) / matrix[0][0];

        for (int i = 0; i < 3; ++i){
            for (int j = 0; j < 4; ++j) std::cout << matrix[i][j] << " ";
           std::cout << "\n";
        }
        return true;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }


    struct quandratic_arguments{
        double quad, lin, inter, start;
    }args;
};

// exponential model
// predict: (key) -> Real[0, 1]
// return e^((key - start) * slope + intercept) - 1
template<typename _Tp,
        typename traits>
class exponential_model{
public:
    typedef _Tp key_type;

    typedef exponential_model<key_type, traits> self;

    typedef typename traits::size_type size_type;

    inline double predict(const key_type &key) const{
        return 1 - (log(std::max(1e-5, args.end - key + 1)) * args.slope + this->args.inter);
    }

    bool train(const _Tp* const key, const size_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (size_type  i = 0; i < n; ++i){
            double pos = 1 - 1.0 * (i + 1) / n;
            double rex = log(args.end - key[i] + 1);
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return true;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    struct exponential_arguments{
        double slope, inter, end;
    }args;
};

// logarithmic model
// predict: (key) -> Real[0, 1]
// return log(key - start + 1) * slope + intercept)
template<typename _Tp,
        typename traits>
class logarithmic_model{
public:
    typedef _Tp key_type;

    typedef logarithmic_model<key_type, traits> self;

    typedef typename traits::size_type size_type;

    inline double predict(const key_type &key) const{
        return log(std::max(1e-5, key - args.start + 1)) * args.slope + this->args.inter;
    }

    bool train(const _Tp* const key, const size_type n){
        AEX_ASSERT(n > 1);
        this->args.start = key[0];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (size_type  i = 0; i < n; ++i){
            double pos = 1.0 * (i + 1) / n;
            double rex = log(key[i] - args.start + 1); 
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        for (size_type  i = 0; i < n; ++i){
            double pos = 1.0 * (i + 1) / n;
            double rex = log(key[i] - args.start + 1);
            std::cout << rex << ":" << pos << "->" << std::fma(this->args.slope, rex, this->args.inter) << " | ";
        }

        AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return true;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    struct logarithmic_arguments{
        double slope, inter, start;
    }args;

};

template<typename _Tp,
        typename traits>
class gap_array_linear_model{
public:
    typedef _Tp key_type;

    typedef typename traits::size_type size_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<size_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return args.slope * (key - args.start);
    }

    // train model with an key array, array size n and slot size
    bool train(const key_type* const key, const size_type n){
        args.start = key[0];
        args.slope = 1 / (key[n - 1] - key[0]);
        return true;
    }

    inline size_type max_error(const key_type* const key, const size_type n, const size_type slot_size){
        size_type error = 0;
        for (size_type i = 0, start = 0; i < n; ++i){
            size_type pos = std::max(0LL, static_cast<size_type>(round(this->predict(key[i]) * slot_size)));
            start = std::max(start, pos);
            AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        double sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    struct linear_arguments{
        double slope, start;
    }args;

};


//template<typename _Tp,
//        typename traits>
//class auto_designed_model{
//public:
//    typedef _Tp key_type;
//
//    typedef auto_designed_model<key_type, traits> self;
//
//    typedef typename traits::size_type size_type;
//
//    bool train(const key_type* const key, const int n, long double* delta_buffer){
//        long double slope = key[n - 1] / key [0];
//        long double history = 0;
//        vector<key_type> zero_point;
//        for (size_type i = 0; i < n; ++i){
//            delta_buffer[i] = key[i] - slope * (key[i] - key[0]);
//            if (i > 0 && i < n - 2){
//                if (delta_buffer[i] * delta[i + 1] < 0){
//                    zero_point.push_back(i);
//                }
//                
//            }
//        }
//        
//    }
//
//    void train_with_RMSE(){}
//
//    void train_with_gap_array(){
//
//    }
//#ifndef AEX_DEBUG
//private:
//#endif
//    double* args;
//
//};

template<typename _Tp,
        typename traits>
class aex_model{
public:
    typedef _Tp key_type;

    typedef aex_model<key_type, traits> self;

    typedef typename traits::size_type size_type;

    inline double predict(const key_type &key) const{
        switch(this->select_model){
            case 0:{
                return _model.m0.predict(key);
            }
            case 1:{
                return _model.m1.predict(key);
            }
            case 2:{
                return _model.m2.predict(key);
            }
            case 3:{
                return _model.m3.predict(key);
            }
            default:{
                AEX_ASSERT(false);
                return 0;
            }
        }
    }
    bool train(const _Tp* const key, const size_type n){
        linear_model<_Tp, traits> m0;
        //m0.args.slope = m0.args.inter = m0.args.start = 0;
        quandratic_model<_Tp, traits> m1;
        exponential_model<_Tp, traits> m2;
        logarithmic_model<_Tp, traits> m3;
        double ans, min_RMSE;
        m0.train(key, n);
        _model.m0 = m0;
        select_model = 0;
        min_RMSE = m0.RMSE(key, n);
        if (min_RMSE < traits::MAX_ALLOW_ERROR * log(n)){
            return true;
        }
        else{
            m1.train(key, n);
            ans = m1.RMSE(key, n);
            if (ans < min_RMSE){
                select_model = 1;
                min_RMSE = ans;
                _model.m1 = m1;
            }
        }
        return false;

        m1.train(key, n);
        ans = m1.RMSE(key, n);
        if (ans < min_RMSE){
            select_model = 1;
            min_RMSE = ans;
            _model.m1 = m1;
        }
        m2.train(key, n);
        ans = m2.RMSE(key, n);
        if (ans < min_RMSE){
            select_model = 2;
            min_RMSE = ans;
            _model.m2 = m2;
        }
        m3.train(key, n);
        ans = m3.RMSE(key, n);
        if (ans < min_RMSE){
            select_model = 2;
            min_RMSE = ans;
            _model.m3 = m3;
        }
    }

    inline double RMSE(const _Tp* const key, const size_type n){
        switch(this->select_model){
            case 0:{
                return _model.m0.RMSE(key, n);
            }
            case 1:{
                return _model.m1.RMSE(key, n);
            }
            case 2:{
                return _model.m2.RMSE(key, n);
            }
            case 3:{
                return _model.m3.RMSE(key, n);
            }
            default:{
                AEX_ASSERT(false);
                return 0;
            }
        }
    }

    union model_args{
        linear_model<_Tp, traits> m0;
        quandratic_model<_Tp, traits> m1;
        exponential_model<_Tp, traits> m2;
        logarithmic_model<_Tp, traits> m3;
    }_model;
    unsigned char select_model;
};

template<typename _Tp,
        template<typename Elem> class layer,
        typename traits>
class two_layer_model{
public:
    typedef two_layer_model self;
    
    typedef _Tp key_type;

    typedef long long size_type;

    typedef layer<_Tp> model;
    
    two_layer_model():segments(nullptr), offset(nullptr){
        //assert(is_same<_Tp, typename layer::key_type>::value == true);
    }

    inline double predict(key_type &x){
        size_type predict_block_pos = max(0, min(block_num - 1, block_num * segments[0]->predict(x)));
        return 1.0 * (offset[predict_block_pos] + segments[1 + predict_block_pos]->predict(x) * (offset[1 + predict_block_pos] - offset[predict_block_pos]) ) / offset[block_num + 1];
    }

    void train(key_type *key, size_type size){
        if (segments != nullptr){
            this->free();
        }
        size_type max_block_size = sqrt(size);
        for (block_size = traits::MIN_BLOCK_SIZE; block_size > max_block_size; block_size <<= 1);
        block_num = (size - 1) / block_size + 1;
        char* data = (char*)malloc(align_8bytes((block_num + 1)* sizeof(model)) + (block_num + 2) * sizeof(size_type)); 
        segments = static_cast<model*>(data); 
        offset = reinterpret_cast<size_type*>(data + align_8bytes((block_num + 1)* sizeof(model)));
        std::vector<key_type> segments_data(block_num);
        for (size_type st = 0, i = 0; st < size; st += block_size, ++i){
            size_type now_block_size = std::min(block_size, size - st);
            offset[i] += st;
            segments[1 + i].train(key + st, now_block_size, now_block_size);
        }
        segments[0].train(segments_data.data(), block_num, block_num);
        offset[block_num + 1] = size;
    }

    inline double RMSE(key_type *key, size_type size){
        double err = 0;
        for (size_type i = 0; i < size; ++i)
            err += sqr(this->predict(key[i]) - i);
        err /= size;
        err = sqrt(err);
        return err;
    }

    inline void insert(size_type pos){
        size_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (size_type i = block_pos; i < block_num; ++i)
            ++offset[i];
    }
    
    inline void erase(size_type pos){
        size_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (size_type i = block_pos; i < block_num; ++i)
            --offset[i];
    }

    void free(){
        free(segments);
        free(offset);
    }
    
private:
    model* segments;
    size_type* offset;
    size_type block_size, block_num;
};



}