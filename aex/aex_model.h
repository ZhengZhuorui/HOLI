#pragma once
namespace aex{

enum model_type{
    linear=0,
    exponential=1,
    logarithmic=2
};

// model template class
template<typename _Tp>
class aex_model_base{
public:
    typedef _Tp key_type;
    typedef size_t size_type;
    inline double predict(const key_type &key){return 0;}
    inline void train(const key_type* const key, const size_type n){}
};

// linear model
// predict: (key) -> Real[0, 1]
// return (key - end) * slope + intercept(end)
template<typename _Tp>
class linear_model{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type> self;

    typedef size_t size_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<size_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return std::fma(args.slope, key - args.end, args.inter);
    }

    // train model with an key array, array size n and slot size
    void train(const key_type* const key, const size_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];

        double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (size_type  i = 0; i < n; ++i){
            double pos = 1.0 * i / (n - 1);
            double rex = key[i] - this->args.end;
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        AEX_FORMAT("train. cap_ratio=%.4f, sum_xy=%.4f, sum_x=%.4f sum_x2=%.4f, bar_x=%.4f bar_y=%.4f, fz=%.4f, fm=%.4f, slope=%.4f inter=%.4f", cap_ratio, sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) - i / (n - 1));
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

private:
    struct linear_arguments{
        double slope, inter, end;
    }args;
};

// exponential model
// predict: (key) -> Real[0, 1]
// return e^((key - end) * slope + intercept(end))
template<typename _Tp>
class exponential_model{
public:
    typedef _Tp key_type;

    typedef exponential_model<key_type> self;

    typedef size_t size_type;

    inline double predict(const key_type &key) const{
        return exp(std::fma(args.slope, key - args.end, args.inter));
    }

    void train(const _Tp* const key, const size_t n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];
        double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (size_type  i = 0; i < n; ++i){
            double pos = log(1.0 * (i + 1) / n);
            double rex = key[i] - this->args.end;
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        AEX_FORMAT("train. cap_ratio=%.4f, sum_xy=%.4f, sum_x=%.4f sum_x2=%.4f, bar_x=%.4f bar_y=%.4f, fz=%.4f, fm=%.4f, slope=%.4f inter=%.4f", cap_ratio, sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) - (i + 1) / n);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }
private:
    struct exponential_arguments{
        double slope, inter, end;
    }args;
};

// logarithmic model
// predict: (key) -> Real[0, 1]
// return log((key - end) * slope + intercept(end))
template<typename _Tp>
class logarithmic_model{
public:
    typedef _Tp key_type;

    typedef logarithmic_model<key_type> self;

    typedef size_t size_type;

    inline double predict(const key_type &key) const{
        return log(std::max(1e-5, std::fma(args.slope, key - args.end, args.inter)));
    }

    void train(const _Tp* const key, const size_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];
        double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (size_type  i = 0; i < n; ++i){
            double pos = exp(1.0 * (i + 1) / n);
            double rex = key[i] - this->args.end;
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        AEX_FORMAT("train. cap_ratio=%.4f, sum_xy=%.4f, sum_x=%.4f sum_x2=%.4f, bar_x=%.4f bar_y=%.4f, fz=%.4f, fm=%.4f, slope=%.4f inter=%.4f", cap_ratio, sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return;
    }

    inline double RMSE(const key_type* const key, const unsigned int n){
        size_type sum = 0;
        for (size_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) - (i + 1) / n);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }
private:
    struct logarithmic_arguments{
        double slope, inter, end;
    }args;
};

template<typename _Tp>
class aex_model{
public:
typedef _Tp key_type;

    typedef logarithmic_model<key_type> self;

    typedef size_t size_type;

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
            default:{
                AEX_ASSERT(false);
                return 0;
            }
        }
    }
    void train(const _Tp* const key, const size_type n){
        linear_model<_Tp> m0;
        exponential_model<_Tp> m1;
        logarithmic_model<_Tp> m2;
        double ans, min_RMSE;
        m0.train(key, n);
        _model.m0 = m0;
        select_model = 0;
        min_RMSE = m0.RMSE(key, n);

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
            default:{
                AEX_ASSERT(false);
                return 0;
            }
        }
    }

private:
    union model_args{
        linear_model<_Tp> m0;
        exponential_model<_Tp> m1;
        logarithmic_model<_Tp> m2;
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

    typedef size_t size_type;

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