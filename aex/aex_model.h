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
    typedef typename traits::pos_type pos_type;
    virtual double predict(const key_type &key) = 0;
    virtual bool train(const key_type* const key, const pos_type n) = 0;
};

// linear model
// predict: (key) -> Real[0, 1]
// return (key - end) * slope + intercept(end)
template<typename _Tp,
        typename traits>
class linear_model{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::pos_type pos_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<pos_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return std::fma(args.slope, key - args.end, args.inter);
    }

    // train model with an key array, array size n and slot size
    bool train(const key_type* const key, const pos_type n){
        //AEX_ASSERT(n > 1);

        this->args.end = key[n - 1];

        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (pos_type  i = 0; i < n; ++i){
            double pos = 1.0 * i / (n - 1);
            double rex = key[i] - this->args.end;
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
        //this->args.inter = std::fma();
        
        //AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f end=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter, args.end);
        //AEX_PRINT("train. RMSE=" << this->RMSE(key, n));
        return true;
    }

    inline double RMSE(const key_type* const key, const pos_type n){
        double sum = 0;
        //pos_type max_error = 0;
        for (pos_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
            //max_error = std::max(max_error, static_cast<pos_type>(std::abs(this->predict(key[i]) * (n - 1) - i)));
            //AEX_FORMAT("%.4f %lld | ", this->predict(key[i]) * (n - 1), i);
        }
        //AEX_PRINT("max error=" << max_error);
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline pos_type max_error(const key_type* const key, const pos_type n, const pos_type slot_size){
        pos_type error = 0;
        for (pos_type i = 0, start = 0; i < n; ++i){
            pos_type pos = std::max(0, static_cast<pos_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    struct linear_arguments{
        double slope, inter, end;
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

    typedef typename traits::pos_type pos_type;

    inline double predict(const key_type &key) const{
        key_type rex = key - args.end;
        return args.quad * sqr(rex) + args.lin * rex + args.inter;
    }

    bool train(const _Tp* const key, const pos_type n){
        this->args.end = key[n - 1];
        long double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0, sum_xxx = 0, sum_xxxx = 0, sum_xxy = 0;
        
        for (pos_type i = 0; i < n; ++i){
            long double pos = 1.0 * i / (n - 1);
            long double rex = key[i] - this->args.end;
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
        for (pos_type i = 0; i < 4; ++i)
            matrix[1][i] -= ratio * matrix[0][i];
        ratio = matrix[2][0] / matrix[0][0];
        for (pos_type i = 0; i < 4; ++i)
            matrix[2][i] -= ratio * matrix[0][i];
        
        //AEX_ASSERT(abs(matrix[1][i]) < 1e-6);
        ratio = matrix[2][1] / matrix[1][1];
        for (pos_type i = 0; i < 4; ++i)
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

    inline double RMSE(const key_type* const key, const pos_type n){
        pos_type sum = 0;
        for (pos_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }


    struct quandratic_arguments{
        double quad, lin, inter, end;
    }args;
};

// exponential model
// predict: (key) -> Real[0, 1]
// return e^((key - end) * slope + intercept) - 1
template<typename _Tp,
        typename traits>
class exponential_model{
public:
    typedef _Tp key_type;

    typedef exponential_model<key_type, traits> self;

    typedef typename traits::pos_type pos_type;

    inline double predict(const key_type &key) const{
        return 1 - (log(std::max(1e-5, args.end - key + 1)) * args.slope + this->args.inter);
    }

    bool train(const _Tp* const key, const pos_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (pos_type  i = 0; i < n; ++i){
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

    inline double RMSE(const key_type* const key, const pos_type n){
        pos_type sum = 0;
        for (pos_type i = 0; i < n; ++i){
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
// return log(key - end + 1) * slope + intercept)
template<typename _Tp,
        typename traits>
class logarithmic_model{
public:
    typedef _Tp key_type;

    typedef logarithmic_model<key_type, traits> self;

    typedef typename traits::pos_type pos_type;

    inline double predict(const key_type &key) const{
        return log(std::max(1e-5, key - args.end + 1)) * args.slope + this->args.inter;
    }

    bool train(const _Tp* const key, const pos_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[0];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (pos_type  i = 0; i < n; ++i){
            double pos = 1.0 * (i + 1) / n;
            double rex = log(key[i] - args.end + 1); 
            sum_y += pos;
            sum_xy += 1.0 * rex * pos;
            sum_x += rex;
            sum_x2 += 1.0 * sqr(rex);
        }
        bar_y = sum_y / n;
        bar_x = sum_x / n;
        this->args.slope = (sum_xy - 1.0 * n * bar_x * bar_y) / (sum_x2 - 1.0 * n * sqr(bar_x));
        this->args.inter = std::fma(args.slope, -bar_x, bar_y);
        //for (pos_type  i = 0; i < n; ++i){
        //    double pos = 1.0 * (i + 1) / n;
        //    double rex = log(key[i] - args.end + 1);
        //    std::cout << rex << ":" << pos << "->" << std::fma(this->args.slope, rex, this->args.inter) << " | ";
        //}

        //AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return true;
    }

    inline double RMSE(const key_type* const key, const pos_type n){
        pos_type sum = 0;
        for (pos_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    struct logarithmic_arguments{
        double slope, inter, end;
    }args;

};

template<typename _Tp,
        typename traits>
class gap_array_linear_model{
public:
    typedef _Tp key_type;

    typedef typename traits::pos_type pos_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<pos_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return 1 + args.slope * (key - args.end);
    }

    // train model with an key array, array size n and slot size
    bool train(const key_type* const key, const pos_type n){
        args.end = key[n - 1];
        args.slope = 1 / (key[n - 1] - key[0]);
        return true;
    }

    inline pos_type max_error(const key_type* const key, const pos_type n, const pos_type slot_size){
        pos_type error = 0;
        for (pos_type i = 0, start = 0; i < n; ++i){
            pos_type pos = std::max(0, static_cast<pos_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    inline double RMSE(const key_type* const key, const pos_type n){
        double sum = 0;
        for (pos_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    struct linear_arguments{
        double slope, end;
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
//    typedef typename traits::pos_type pos_type;
//
//    bool train(const key_type* const key, const int n, long double* delta_buffer){
//        long double slope = key[n - 1] / key [0];
//        long double history = 0;
//        vector<key_type> zero_point;
//        for (pos_type i = 0; i < n; ++i){
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

    typedef typename traits::pos_type pos_type;

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
    bool train(const _Tp* const key, const pos_type n){
        linear_model<_Tp, traits> m0;
        //m0.args.slope = m0.args.inter = m0.args.end = 0;
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

    inline double RMSE(const _Tp* const key, const pos_type n){
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

    typedef long long pos_type;

    typedef layer<_Tp> model;
    
    two_layer_model():segments(nullptr), offset(nullptr){
        //assert(is_same<_Tp, typename layer::key_type>::value == true);
    }

    inline double predict(key_type &x){
        pos_type predict_block_pos = max(0, min(block_num - 1, block_num * segments[0]->predict(x)));
        return 1.0 * (offset[predict_block_pos] + segments[1 + predict_block_pos]->predict(x) * (offset[1 + predict_block_pos] - offset[predict_block_pos]) ) / offset[block_num + 1];
    }

    void train(key_type *key, pos_type size){
        if (segments != nullptr){
            this->free();
        }
        pos_type max_block_size = sqrt(size);
        for (block_size = traits::MIN_BLOCK_SIZE; block_size > max_block_size; block_size <<= 1);
        block_num = (size - 1) / block_size + 1;
        char* data = (char*)malloc(align_8bytes((block_num + 1)* sizeof(model)) + (block_num + 2) * sizeof(pos_type)); 
        segments = static_cast<model*>(data); 
        offset = reinterpret_cast<pos_type*>(data + align_8bytes((block_num + 1)* sizeof(model)));
        std::vector<key_type> segments_data(block_num);
        for (pos_type st = 0, i = 0; st < size; st += block_size, ++i){
            pos_type now_block_size = std::min(block_size, size - st);
            offset[i] += st;
            segments[1 + i].train(key + st, now_block_size, now_block_size);
        }
        segments[0].train(segments_data.data(), block_num, block_num);
        offset[block_num + 1] = size;
    }

    inline double RMSE(key_type *key, pos_type size){
        double err = 0;
        for (pos_type i = 0; i < size; ++i)
            err += sqr(this->predict(key[i]) - i);
        err /= size;
        err = sqrt(err);
        return err;
    }

    inline void insert(pos_type pos){
        pos_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (pos_type i = block_pos; i < block_num; ++i)
            ++offset[i];
    }
    
    inline void erase(pos_type pos){
        pos_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (pos_type i = block_pos; i < block_num; ++i)
            --offset[i];
    }

    void free(){
        free(segments);
        free(offset);
    }
    
private:
    model* segments;
    pos_type* offset;
    pos_type block_size, block_num;
};

template<typename _Tp,
        typename traits>
class piecewise_linear_model{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::pos_type pos_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<pos_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        double ret = 0;
        for (unsigned int i = 0; i < args.seg_nums; ++i)
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
        return ret;
    }

    bool train(const key_type* const key, const pos_type n, const pos_type slot_size){
        AEX_HINT("[train]");
        const double density = n / slot_size;

        AEX_ASSERT(n > 1);
        AEX_ASSERT(density <= 0.5);

        double max_density = (1 + density) / 2;
        double gap = 1.0 / max_density / slot_size;
        
        AEX_PRINT("n=" << n << ", slot size=" << slot_size);

        const pos_type max_offset = traits::ERROR_BOUND / 2;
        const pos_type windows_size = ceil(max_offset / (1 - max_density));
        std::vector<double> slope(n), windows_slope(windows_size), max_windows_slope(max_offset);
        std::vector<pos_type> sta(n);

        std::vector<double> f(traits::MAX_SEGMENT_NUM * n);
        std::vector<pos_type> g(traits::MAX_SEGMENT_NUM * n);
        
        std::fill(windows_slope.begin(), windows_slope.end(), std::numeric_limits<double>::max());
        
        for (pos_type i = n - 2; i >= 0; --i){
            double k = gap / (key[i + 1] - key[i]);
            AEX_PRINT("slope " << i << " = " << k);
            slope[i] = k;
            std::move_backward(windows_slope.data(), windows_slope.data() + windows_size - 1, windows_slope.data() + windows_size);
            windows_slope[0] = k;
            std::fill(max_windows_slope.data(), max_windows_slope.data() + max_offset, 0);
            
            for (pos_type j = 0; j < windows_size; ++j){
                for (pos_type k = 0; k < max_offset; ++k)
                    if (windows_slope[j] > max_windows_slope[k]){
                        std::move_backward(max_windows_slope.data() + k, max_windows_slope.data() + max_offset - 1, max_windows_slope.data() + max_offset);
                        max_windows_slope[k] = windows_slope[j];
                        break;
                    }
                AEX_PRINT("max_windows_slope[" << static_cast<pos_type>(floor((j + 1) * (1 - max_density))) << "]=" << max_windows_slope[static_cast<pos_type>(floor((j + 1) * (1 - max_density)))]);
                slope[i] = std::min(slope[i], max_windows_slope[static_cast<pos_type>(floor((j + 1) * (1 - max_density)))]);
            }
            
        }

        // Dynamic Programing:
        //double f[seg_num][n];
        //pos_type g[seg_num][n];
        // for j = 0 to seg_num do
        //   for i = n-1 downto 0 do
        //     for k = i+1 to n-1 do
        //       f[j][i] = min(f[j][i], f[j][k] + max(slope[i], ... , slope[k-1]) * (key[k] - key[i]))
        // use Monotonic Stack improve it
        this->args.seg_nums = 0;
        {
            double max_segment_slope = 0;
            for (pos_type i = n - 2; i >= 0; --i){
                max_segment_slope = std::max(max_segment_slope, slope[i]);
                f[i] = max_segment_slope * (key[n - 1] - key[i]);
                std::cout << "f[0][" << i << "]=" << f[i] << " ";
            }
            
        }
        
        for (pos_type j = 1; j < traits::MAX_SEGMENT_NUM; ++j){
            std::cout << std::endl;
            if (f[(j - 1) * n] < 1){
                this->args.seg_nums = j;
                break;
            }
            pos_type head = 0, tail = 1;
            sta[0] = n - 1;
            for (pos_type i = n - 2; i < n - 2; ++i){
                while (tail >= head && slope[sta[tail]] < slope[i]) --tail;
                sta[++tail] = slope[i];
                // while (head < tail - 1 && f[j-1][sta[head]] + slope[sta[head + 1]] * (key[sta[head]] - key[i]) > ) ++head;
                while (head < tail - 1 && f[(j - 1) * n + sta[head]] + slope[sta[head + 1]] * (key[sta[head]] - key[i]) >
                                        f[(j - 1) * n + sta[head + 1]] + slope[sta[head + 2]] * (key[sta[head + 1]] - key[i])  ) ++head;
                // f[j][i] = f[j - 1][sta[head]] + slope[sta[head]] * (key[sta[head] + 1] - key[i]);
                f[j * n + i] = f[(j - 1) * n + sta[head]] + slope[sta[head]] * (key[sta[head + 1]] - key[i]);
                //g[j][i] = sta[head];
                g[j * n + i] = sta[head];
                std::cout << "f[" << j << "][" << i << "]=" << f[j * n + i] << ", g[" << j << "][" << i << "]=" << g[j * n + i] << std::endl;
            }
        }

        if (f[(traits::MAX_SEGMENT_NUM - 1) * n] < 1) this->args.seg_nums = traits::MAX_SEGMENT_NUM;

        if (this->args.seg_nums == 0) 
            return false;

        for (pos_type j = 0, i = this->args.seg_nums - 1; i >= 0; j = g[i * n + j], --i){
            double max_seg_slope = 0;
            for (pos_type k = j; k < g[i * n + j]; ++k)
                max_seg_slope = std::max(max_seg_slope, slope[k]);
            this->args.slope[this->args.seg_nums - i - 1] = 1.0 / slot_size * max_seg_slope;
            this->args.end[this->args.seg_nums - i - 1] = key[g[i * n + j]];
        }

        for (unsigned int i = 0; i < this->args.seg_nums - 1; ++i)
            this->args.slope[i] -= this->args.slope[i + 1];

        return true;
    }

    inline double RMSE(const key_type* const key, const pos_type n){
        double sum = 0;
        //pos_type max_error = 0;
        for (pos_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline pos_type max_error(const key_type* const key, const pos_type n, const pos_type slot_size){
        pos_type error = 0;
        for (pos_type i = 0, start = 0; i < n; ++i){
            pos_type pos = std::max(0, static_cast<pos_type>(this->predict(key[i]) * slot_size));
1            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
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

}