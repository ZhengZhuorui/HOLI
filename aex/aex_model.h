
#pragma once
namespace aex{

enum model_type{
    linear=0,
    exponential=1,
    logarithmic=2,
    piecewise_linear=3,
    piecewise_linear_2=4,
    piecewise_linear_3=5,
    piecewise_linear_4=6
};

// model template class
template<typename _Tp,
        typename traits>
class aex_model_base{
public:
    typedef _Tp key_type;
    typedef typename traits::slot_type slot_type;
    virtual double predict(const key_type &key) = 0;
    virtual bool train(const key_type* const key, const slot_type n) = 0;
    virtual bool train(const key_type* const key, const slot_type n, const slot_type slot_size) = 0;
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

    typedef typename traits::slot_type slot_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<slot_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return std::fma(args.slope, key - args.end, args.inter);
    }

    inline bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        return train(key, n);
    }

    // train model with an key array, array size n and slot size
    bool train(const key_type* const key, const slot_type n){
        //AEX_ASSERT(n > 1);

        this->args.end = key[n - 1];

        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;

        for (slot_type  i = 0; i < n; ++i){
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

    inline double RMSE(const key_type* const key, const slot_type n){
        double sum = 0;
        //slot_type max_error = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
            //max_error = std::max(max_error, static_cast<slot_type>(std::abs(this->predict(key[i]) * (n - 1) - i)));
            //AEX_FORMAT("%.4f %lld | ", this->predict(key[i]) * (n - 1), i);
        }
        //AEX_PRINT("max error=" << max_error);
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "linear model";
    }
    #endif

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

    typedef typename traits::slot_type slot_type;

    inline double predict(const key_type &key) const{
        key_type rex = key - args.end;
        return args.quad * sqr(rex) + args.lin * rex + args.inter;
    }

    inline bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        return train(key, n);
    }

    bool train(const key_type* const key, const slot_type n){
        this->args.end = key[n - 1];
        long double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0, sum_xxx = 0, sum_xxxx = 0, sum_xxy = 0;
        
        for (slot_type i = 0; i < n; ++i){
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
        for (slot_type i = 0; i < 4; ++i)
            matrix[1][i] -= ratio * matrix[0][i];
        ratio = matrix[2][0] / matrix[0][0];
        for (slot_type i = 0; i < 4; ++i)
            matrix[2][i] -= ratio * matrix[0][i];
        
        //AEX_ASSERT(abs(matrix[1][i]) < 1e-6);
        ratio = matrix[2][1] / matrix[1][1];
        for (slot_type i = 0; i < 4; ++i)
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

    inline double RMSE(const key_type* const key, const slot_type n){
        slot_type sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "quandratic model";
    }
    #endif

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

    typedef typename traits::slot_type slot_type;

    inline double predict(const key_type &key) const{
        return 1 - (log(std::max(1e-5, args.end - key + 1)) * args.slope + this->args.inter);
    }

    bool train(const _Tp* const key, const slot_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[n - 1];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (slot_type  i = 0; i < n; ++i){
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

    inline double RMSE(const key_type* const key, const slot_type n){
        slot_type sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "exponential model";
    }
    #endif

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

    typedef typename traits::slot_type slot_type;

    inline double predict(const key_type &key) const{
        return log(std::max(1e-5, key - args.end + 1)) * args.slope + this->args.inter;
    }

    bool train(const _Tp* const key, const slot_type n){
        AEX_ASSERT(n > 1);
        this->args.end = key[0];
        long double sum_x2 = 0, sum_xy = 0, sum_x = 0, bar_x, bar_y, sum_y = 0;
        for (slot_type  i = 0; i < n; ++i){
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
        //for (slot_type  i = 0; i < n; ++i){
        //    double pos = 1.0 * (i + 1) / n;
        //    double rex = log(key[i] - args.end + 1);
        //    std::cout << rex << ":" << pos << "->" << std::fma(this->args.slope, rex, this->args.inter) << " | ";
        //}

        //AEX_FORMAT("train. sum_xy=%.4Lf, sum_x=%.4Lf sum_x2=%.4Lf, bar_x=%.4Lf bar_y=%.4Lf, fz=%.4Lf, fm=%.4Lf, slope=%.4f inter=%.4f", sum_xy, sum_x, sum_x2, bar_x, bar_y, (sum_xy - n * bar_x * bar_y), (sum_x2 - n * sqr(bar_x)), args.slope, args.inter);
        return true;
    }

    inline double RMSE(const key_type* const key, const slot_type n){
        slot_type sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "logarithmic model";
    }
    #endif

    struct logarithmic_arguments{
        double slope, inter, end;
    }args;

};

template<typename _Tp,
        typename traits>
class gap_array_linear_model{
public:
    typedef _Tp key_type;

    typedef typename traits::slot_type slot_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<slot_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return 1 + args.slope * (key - args.end);
    }

    inline bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        return train(key, n);
    }

    // train model with an key array, array size n and slot size
    inline bool train(const key_type* const key, const slot_type n){
        args.end = key[n - 1];
        args.slope = 1.0 / (key[n - 1] - key[0]);
        return true;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    inline double RMSE(const key_type* const key, const slot_type n){
        double sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "gap array linear model";
    }
    #endif

    struct linear_arguments{
        double slope, end;
    }args;

};

template<typename _Tp,
        typename traits>
class gap_array_linear_model_hash_table{
public:
    typedef _Tp key_type;

    typedef typename traits::slot_type slot_type;

    // return the predict position. value range from -inf to +inf.
    inline long double predict(const key_type key) const {
        //return static_cast<slot_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        //return this->args.slot_size + std::min(0, static_cast<slot_type>(args.slope * (key - args.end)));
        //return this->args.slot_size + (key < args.end) * args.slope * (key - args.end);
        return (key - this->args.start) * this->args.slope + 1;
    }

    inline bool train(const key_type* const keys, const slot_type n, const slot_type slot_size){
        AEX_ASSERT(n > 2);
        AEX_ASSERT(slot_size >= traits::MIN_HASH_NODE_SLOT_SIZE);
        slot_type H = 1, T = n - 1;
        slot_type S = n / 8, E = n / 8 * 7;
        while (S < E && keys[S] == keys[0])
            ++S;
        if (S >= E || keys[S] == keys[E])
            return false;
        while (H < S && 1.0 * (S - H) / (keys[S] - keys[H]) < 0.5 * (E - S) / (keys[E] - keys[S]))
            ++H;
        while (T > E && 1.0 * (T - E) / (keys[T] - keys[E]) < 0.5 * (E - S) / (keys[E] - keys[S]))
            --T;
        args.start = keys[H];
        args.slope = 1.0 * (slot_size - 2) / (keys[T] - keys[H]);
        //AEX_PRINT("keys[0]=" << keys[0] << ", pos[0]=" << this->predict(keys[0]));
        //if (this->predict(keys[0]) > 0){
            //AEX_PRINT("n=" << n << ", keys[n-1]=" << keys[n - 1] << ", slot_size=" << slot_size << ", slope=" << this->args.slope << "start=" << start << ", key[0]=" << keys[0] << ", key[start]" << keys[start] << ", " << this->predict(keys[0]) << ", " << this->predict(keys[start]) << ", " << (keys[0] - this->args.start) * this->args.slope + 1);
        //}
        //offset = (8 * traits::SLOT_PER_LOCK * this->args.slope < (this->args.start - keys[0])) ? (HASH_NODE_MAX_GAP_SLOT * traits::SLOT_PER_LOCK) : -((keys[0] - this->args.start) * this->args.slope);
        AEX_ASSERT(static_cast<slot_type>(this->predict(keys[0])) <= 0);
        return true;
    }

    // train model with an key array, array size n and slot size
    //inline bool train(const key_type* const key, const slot_type n){
    //    args.end = key[n - 1];
    //    args.slope = 1.0 / (key[n - 1] - key[0]);
    //    return true;
    //}

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

    inline double RMSE(const key_type* const key, const slot_type n){
        double sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "gap array linear model";
    }
    #endif

    struct linear_arguments{
        long double slope, start;
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
//    typedef typename traits::slot_type slot_type;
//
//    bool train(const key_type* const key, const int n, long double* delta_buffer){
//        long double slope = key[n - 1] / key [0];
//        long double history = 0;
//        vector<key_type> zero_point;
//        for (slot_type i = 0; i < n; ++i){
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

    typedef typename traits::slot_type slot_type;

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

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        return train(key, n);
    }

    bool train(const key_type* const key, const slot_type n){
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

    inline double RMSE(const _Tp* const key, const slot_type n){
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

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "aex model";
    }
    #endif

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

    typedef long long slot_type;

    typedef layer<_Tp> model;
    
    two_layer_model():segments(nullptr), offset(nullptr){
    }

    inline double predict(key_type &x){
        slot_type predict_block_pos = max(0, min(block_num - 1, block_num * segments[0]->predict(x)));
        return 1.0 * (offset[predict_block_pos] + segments[1 + predict_block_pos]->predict(x) * (offset[1 + predict_block_pos] - offset[predict_block_pos]) ) / offset[block_num + 1];
    }

    void train(const key_type *key, const slot_type n, const slot_type slot_size){
        return train(key, n);
    }

    void train(const key_type *key, const slot_type size){
        if (segments != nullptr){
            this->free();
        }
        slot_type max_block_size = sqrt(size);
        for (block_size = traits::MIN_BLOCK_SIZE; block_size > max_block_size; block_size <<= 1);
        block_num = (size - 1) / block_size + 1;
        char* data = (char*)malloc(align_8bytes((block_num + 1)* sizeof(model)) + (block_num + 2) * sizeof(slot_type)); 
        segments = static_cast<model*>(data); 
        offset = reinterpret_cast<slot_type*>(data + align_8bytes((block_num + 1)* sizeof(model)));
        std::vector<key_type> segments_data(block_num);
        for (slot_type st = 0, i = 0; st < size; st += block_size, ++i){
            slot_type now_block_size = std::min(block_size, size - st);
            offset[i] += st;
            segments[1 + i].train(key + st, now_block_size, now_block_size);
        }
        segments[0].train(segments_data.data(), block_num, block_num);
        offset[block_num + 1] = size;
    }

    inline double RMSE(key_type *key, slot_type size){
        double err = 0;
        for (slot_type i = 0; i < size; ++i)
            err += sqr(this->predict(key[i]) - i);
        err /= size;
        err = sqrt(err);
        return err;
    }

    inline void insert(slot_type pos){
        slot_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (slot_type i = block_pos; i < block_num; ++i)
            ++offset[i];
    }
    
    inline void erase(slot_type pos){
        slot_type block_pos = std::lower_bound(offset, offset + block_num, pos) - offset;
        for (slot_type i = block_pos; i < block_num; ++i)
            --offset[i];
    }

    void free(){
        free(segments);
        free(offset);
    }
    
private:
    model* segments;
    slot_type* offset;
    slot_type block_size, block_num;
};

template<typename _Tp,
        typename traits>
class piecewise_linear_model{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    piecewise_linear_model(){
        this->args.seg_nums = 0;
        std::fill(this->args.slope, this->args.slope + traits::MAX_SEGMENT_NUM, 0);
    }

    ~piecewise_linear_model(){}

    // return the predict position. value range from 0 to +inf.
    forceinline long double predict(const key_type &key) const {
        long double ret = 1;
        for (unsigned int i = 0; i < args.seg_nums; ++i){
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
        }
        return ret;
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        #ifdef AEX_DEBUG
        //std::chrono::system_clock::time_point t1, t2;
        //t1 = std::chrono::high_resolution_clock::now();
        //static double train_time = 0;
        #endif
        //AEX_PRINT("train...");
        const double density = 1.0 * n / slot_size;
        AEX_ASSERT(n > 1);
        const long double max_density = 2.0 / (1.0 + 1.0 / density); //harmonic mean
        const long double gap = 1.0 / (n - 1) * max_density;
        const slot_type max_offset = traits::ERROR_BOUND - 2;
        const slot_type windows_size = ceil(1.0 * max_offset / (1 - max_density));
        std::vector<long double> slope(n), windows_slope(windows_size), max_windows_slope(max_offset + 1);
        std::fill(windows_slope.begin(), windows_slope.end(), 1.0 * max_density / (key[n - 1] - key[0]));

        for (slot_type i = n - 2; i >= 0; --i){
            if (key[i + 1] == key[i]) return false;
            long double k = gap / (key[i + 1] - key[i]);
            slope[i] = k;
            windows_slope[i % windows_size] = k;
            
            std::fill(max_windows_slope.data(), max_windows_slope.data() + max_offset + 1, 0);
            for (slot_type j = 0; j < windows_size; ++j){
                long double now_slope = windows_slope[(j + i) % windows_size];
                for (slot_type k = 0; k < max_offset + 1; ++k)
                    if (now_slope > max_windows_slope[k]){
                        std::move_backward(max_windows_slope.data() + k, max_windows_slope.data() + max_offset, max_windows_slope.data() + max_offset + 1);
                        max_windows_slope[k] = now_slope;
                        break;
                    }
                slope[i] = std::min(slope[i], max_windows_slope[static_cast<slot_type>(floor(1.0 * (j + 1) * (1 - max_density)))]);
            }
            //AEX_PRINT("slope[" << i << "]=" << slope[i] << ", real=" << gap / (key[i + 1] - key[i]));
        }

        std::fill(args.end, args.end + traits::MAX_SEGMENT_NUM, std::numeric_limits<key_type>::lowest());
        std::fill(args.slope, args.slope + traits::MAX_SEGMENT_NUM, 0);

        slot_type segment_start[traits::MAX_SEGMENT_NUM + 1];
        
        long double segment_slope[traits::MAX_SEGMENT_NUM]; 
        for (unsigned int max_segment_num = 1; max_segment_num <= static_cast<unsigned int>(std::min(traits::MAX_SEGMENT_NUM, n / 4)); max_segment_num *= 2){
            
            slot_type seg_len = static_cast<slot_type>(ceil(1.0 * (n - 1) / max_segment_num));
            for (unsigned int i = 0; i < max_segment_num; ++i)
                segment_start[i] = seg_len * i;
            segment_start[max_segment_num] = n - 1;

            std::fill(segment_slope, segment_slope + traits::MAX_SEGMENT_NUM, 0);
            for (unsigned int i = 0; i < max_segment_num; ++i)
            for (slot_type j = segment_start[i] ; j < segment_start[i + 1]; ++j)
                segment_slope[i] =  std::max(segment_slope[i], slope[j]);

            for (unsigned int i = 0; i < max_segment_num; ++i){
                if (i > 0 && segment_slope[i] < segment_slope[i - 1]){
                    for (slot_type j = segment_start[i]; j > segment_start[i - 1] && slope[j] <= segment_slope[i]; --j)
                        --segment_start[i];
                }
                if (i < max_segment_num - 1 && segment_slope[i] < segment_slope[i + 1]){
                    for (slot_type j = segment_start[i + 1]; j < segment_start[i + 2] && slope[j] <= segment_slope[i]; ++j)
                        ++segment_start[i + 1];
                }
            }

            long double S = 0;
            for (unsigned int i = 0; i < max_segment_num; ++i){
                this->args.slope[i] = segment_slope[i];
                this->args.end[i] = key[segment_start[i + 1]];
                S += 1.0 * (key[segment_start[i + 1]] - key[segment_start[i]]) * segment_slope[i];
            }

            //AEX_PRINT("S=" << S);
            if (S > 1){
                continue;
            }

            long double left_slope = (1.0 - S) / (key[n - 1] - key[0]);
            //AEX_PRINT("left_slope=" << left_slope << ", S=" << S);
            for (unsigned int i = 0; i < max_segment_num; ++i)
                this->args.slope[i] += left_slope;

            for (unsigned int i = 0; i < max_segment_num - 1; ++i)
                this->args.slope[i] -= this->args.slope[i + 1];
            
            args.seg_nums = max_segment_num;
            #ifdef AEX_DEBUG
            //t2 = std::chrono::high_resolution_clock::now();
            //train_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            //AEX_PRINT("train_time=" << train_time);
            #endif
            return true;
        }
        return false;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        return -1;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "piecewise linear model 4";
    }
    #endif

    struct piecewise_linear_model_arguments{
        long double end[traits::MAX_SEGMENT_NUM], slope[traits::MAX_SEGMENT_NUM];
        unsigned int seg_nums;
    }args;
};

template<typename _Tp,
        typename traits>
class piecewise_linear_model_2{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    piecewise_linear_model_2(){
        this->args.seg_nums = 0;
        std::fill(this->args.slope, this->args.slope + traits::MAX_SEGMENT_NUM, 0);
    }

    ~piecewise_linear_model_2(){}

    // return the predict position. value range from 0 to +inf.
    forceinline long double predict(const key_type &key) const {
        long double ret = 1;
        //for (unsigned int i = 0; i < args.seg_nums; ++i){
        for (unsigned int i = 0; i < args.seg_nums; ++i){
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
        }
        return ret;
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        #ifdef AEX_DEBUG
        //std::chrono::system_clock::time_point t1, t2;
        //t1 = std::chrono::high_resolution_clock::now();
        //static double train_time = 0;
        #endif
        //AEX_PRINT("train...");
        if (key[n] - key[0] >= static_cast<key_type>(traits::INNER_NODE_MAX_DIFFERENT_VALUE))
            return false;
        for (int i = 0; i < n - 1; ++i)
            if (key[i] == key[i + 1])
                return false;
        long double density = 1.0 * n / slot_size;
        long double max_density = 2.0 / (1.0 + 1.0 / density);
        long double gap = 1.0 / (n - 1) * max_density;
        //AEX_PRINT("density=" << density << ", max_density=" << max_density << ", gap=" << gap);
        slot_type max_error = traits::ERROR_BOUND - 2;
        slot_type block_size = max_error;
        slot_type block_num = (n - 1) / block_size + ((n - 1) % block_size != 0);
        auto block_slope = [&key, block_size, gap, n](int i){
            return ((i + 1) * block_size >= n - 1) ? 
                    (((n - 1 - block_size * i) * gap) / (key[n - 1] - key[block_size * i])) : 
                    (1.0 * block_size * gap / (key[block_size * (i + 1)] - key[block_size * i]));
            };
        [[maybe_unused]] auto segment_distance = [&key, block_size, n](int x, int y){
            return ((y + 1) * block_size >= n) ? (key[n - 1] - key[block_size * x]) : (key[block_size * y] - key[block_size * x]);
        };

        {
        //    AEX_PRINT("n=" << n << ", block_size=" << block_size << ", block nums=" << block_num << ", gap=" << gap << ", max_density=" << max_density << ", block[0] key=" << (key[block_size*1] - key[0]));
        //    for (int i = 0; i < block_num; ++i)
        //        AEX_PRINT("block_slope=" << block_slope(i));
        }
            
        int segment_start[traits::MAX_SEGMENT_NUM + 1];
        for (int segment_num = 1; segment_num <= traits::MAX_SEGMENT_NUM && segment_num <= block_num; segment_num <<= 1){
            std::fill(args.slope, args.slope + traits::MAX_SEGMENT_NUM, 0);
            std::fill(args.end, args.end + traits::MAX_SEGMENT_NUM, 0);
            
            int avg_segment_block_num = block_num / segment_num;

            for (int start = 0, i = 0; i < segment_num; ++i, start += avg_segment_block_num)
                segment_start[i] = start;

            segment_start[segment_num] = block_num;

            for (int i = 0; i < segment_num; ++i)
                for (int j = segment_start[i]; j < segment_start[i + 1]; ++j)
                    args.slope[i] = std::max(args.slope[i], block_slope(j));

            for (int i = 0; i < segment_num; ++i){
                if (i > 0 && args.slope[i] < args.slope[i - 1]){
                    while (segment_start[i] > segment_start[i - 1] + 1 && args.slope[i] >= block_slope(segment_start[i] - 1))
                        --segment_start[i];
                }
                if (i < segment_num - 1 && args.slope[i] < args.slope[i + 1]){
                    while (segment_start[i + 1] + 1 < segment_start[i + 2] && args.slope[i] >= block_slope(segment_start[i + 1]))
                        ++segment_start[i + 1];
                }
            }
            
            //for (int i = 0; i < segment_num; ++i){
            //    if (i > 0 && args.slope[i] < args.slope[i - 1]){
            //        while (segment_start[i] > segment_start[i - 1] + 1 && ( args.slope[i] >= block_slope(segment_start[i] - 1) ||
            //        (
            //            segment_distance(segment_start[i - 1], segment_start[i]) * args.slope[i - 1] + segment_distance(segment_start[i], segment_start[i + 1]) * args.slope[i] > 
            //            segment_distance(segment_start[i - 1], segment_start[i] - 1) * args.slope[i - 1] + segment_distance(segment_start[i] - 1, segment_start[i + 1]) * std::max(args.slope[i], block_slope(segment_start[i] - 1)) 
            //        )
            //        )){
            //            --segment_start[i];
            //            args.slope[i] = std::max(args.slope[i], block_slope(segment_start[i] - 1));
            //        }
            //    }
            //    if (i < segment_num - 1 && args.slope[i] < args.slope[i + 1]){
            //        while (segment_start[i + 1] + 1 < segment_start[i + 2] && (args.slope[i] >= block_slope(segment_start[i + 1]) || 
            //        (
            //            segment_distance(segment_start[i], segment_start[i + 1]) * args.slope[i] + segment_distance(segment_start[i + 1], segment_start[i + 2]) * args.slope[i + 1] > 
            //            segment_distance(segment_start[i], segment_start[i + 1] + 1) * std::max(args.slope[i], block_slope(segment_start[i + 1])) + segment_distance(segment_start[i + 1] + 1, segment_start[i + 2]) * args.slope[i + 1] 
            //        )
            //        )){
            //            args.slope[i] = std::max(args.slope[i], block_slope(segment_start[i + 1]));
            //            ++segment_start[i + 1];
            //        }
            //    }
            //}

            for (int i = 0; i < segment_num; ++i){
                args.end[i] = key[std::min(n - 1, segment_start[i + 1] * block_size)];
            }

            long double S = 0, last_key = key[0];
            for (int i = 0; i < segment_num; last_key = this->args.end[i], ++i)
                S += 1.0 * (this->args.end[i] - last_key) * this->args.slope[i];

            //AEX_PRINT("segment_num=" << segment_num << ", S=" << S);
            if (S > 1){
                continue;
            }
            long double left_slope = (1 - S) / (key[n - 1] - key[0]);
            for (int i = 0; i < segment_num; ++i)
                this->args.slope[i] += left_slope;
            
            //for (int i = 0; i < segment_num; ++i)
            //    AEX_PRINT("end=" << this->args.end[i] << ", slope=" << this->args.slope[i]);

            #ifdef AEX_DEBUG
            //{
            //    S = 0;
            //    last_key = key[0];
            //    for (int i = 0; i < segment_num; last_key = this->args.end[i], ++i)
            //        S += 1.0 * (this->args.end[i] - last_key) * this->args.slope[i];
            //    
            //    //if (!(std::abs(S - 1) < 1e-6)){
            //    //    AEX_PRINT("S=" << S << ", n=" << n << ", slot_size=" << slot_size << ", segment_num=" << segment_num);
            //    //    for (int i = 0; i < segment_num; ++i)
            //    //        AEX_PRINT("i=" << i << "end=" << this->args.end[i] << ", slope=" << this->args.slope[i] << ", segment start=" << segment_start[i] << ", key_start=" << key[segment_start[i]] << ", key_end=" << key[segment_start[i + 1]]);
            //    //}
            //    
            //    AEX_ASSERT(std::abs(S - 1) < 1e-6);
            //}
            #endif

            for (int i = 0; i < segment_num - 1; ++i)
                this->args.slope[i] -= this->args.slope[i + 1];

            args.seg_nums = segment_num;
            //AEX_PRINT("max error= " << this->max_error(key, n, slot_size));
            return true;
            
        }
        return false;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        return -1;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "piecewise linear model 2";
    }
    #endif

    struct piecewise_linear_model_arguments{
        long double slope[traits::MAX_SEGMENT_NUM];
        long double end[traits::MAX_SEGMENT_NUM];
        unsigned int seg_nums;
    }args;
};


template<typename _Tp,
        typename traits>
class piecewise_linear_model_3{
public:
    typedef _Tp key_type;

    typedef linear_model<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    piecewise_linear_model_3(){
        this->args.seg_nums = 0;
        std::fill(this->args.slope, this->args.slope + traits::MAX_SEGMENT_NUM, 0);
    }

    ~piecewise_linear_model_3(){}

    // return the predict position. value range from 0 to +inf.
    forceinline long double predict(const key_type &key) const {
        long double ret = 1;
        for (unsigned int i = 0; i < args.seg_nums; ++i){
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
        }
        return ret;
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        //AEX_PRINT("train...");
        const double density = 1.0 * n / slot_size;
        AEX_ASSERT(n > 1);
        const long double min_gap = 1.0 / slot_size;
        const slot_type max_offset = traits::ERROR_BOUND - 2;
        std::vector<long double> slope(n);

        for  (slot_type i = 0; i < n - 1; ++i){
            if (key[i + 1] == key[i])
                return false;
            slope[i] = min_gap / (key[i + 1] - key[i]);
            for  (slot_type j = 2; j < max_offset && i + j < n; ++j){
                slope[i] = std::min(slope[i], min_gap * (j + 1) / (key[i + j] - key[i]));
            }
            if (i + max_offset >= n)
                slope[i] = std::min(slope[i - 1], slope[i]);
        }

        int windows[3] = {64, 4096, 262144};
        //2^6  2^12 2^18
        //long double min_density[3];
        int windows_sz = 3;

        for (int i = 0; i < windows_sz; ++i){
            long double min_density = (1 + density) / (windows_sz + 1) * (i + 1);
            for (slot_type j = 0; i + j < n; ++i){
                slope[i] = std::max(slope[i], windows[i] * min_gap / min_density / (key[j + windows[i] + 1] - key[j]) );
            }
        }

        std::fill(args.end, args.end + traits::MAX_SEGMENT_NUM, std::numeric_limits<key_type>::lowest());
        std::fill(args.slope, args.slope + traits::MAX_SEGMENT_NUM, 0);

        slot_type segment_start[traits::MAX_SEGMENT_NUM + 1];
        
        long double segment_slope[traits::MAX_SEGMENT_NUM]; 
        for (unsigned int max_segment_num = 1; max_segment_num <= static_cast<unsigned int>(std::min(traits::MAX_SEGMENT_NUM, n / 4)); max_segment_num *= 2){
            
            slot_type seg_len = static_cast<slot_type>(ceil(1.0 * (n - 1) / max_segment_num));
            for (unsigned int i = 0; i < max_segment_num; ++i)
                segment_start[i] = seg_len * i;
            segment_start[max_segment_num] = n - 1;

            std::fill(segment_slope, segment_slope + traits::MAX_SEGMENT_NUM, 0);
            for (unsigned int i = 0; i < max_segment_num; ++i)
            for (slot_type j = segment_start[i] ; j < segment_start[i + 1]; ++j)
                segment_slope[i] =  std::max(segment_slope[i], slope[j]);

            for (unsigned int i = 0; i < max_segment_num; ++i){
                if (i > 0 && segment_slope[i] < segment_slope[i - 1]){
                    for (slot_type j = segment_start[i]; j > segment_start[i - 1] && slope[j] <= segment_slope[i]; --j)
                        --segment_start[i];
                }
                if (i < max_segment_num - 1 && segment_slope[i] < segment_slope[i + 1]){
                    for (slot_type j = segment_start[i + 1]; j < segment_start[i + 2] && slope[j] <= segment_slope[i]; ++j)
                        ++segment_start[i + 1];
                }
            }

            long double S = 0;
            for (unsigned int i = 0; i < max_segment_num; ++i){
                this->args.slope[i] = segment_slope[i];
                this->args.end[i] = key[segment_start[i + 1]];
                S += 1.0 * (key[segment_start[i + 1]] - key[segment_start[i]]) * segment_slope[i];
            }

            //AEX_PRINT("S=" << S);
            if (S > 1){
                continue;
            }

            long double left_slope = (1.0 - S) / (key[n - 1] - key[0]);
            //AEX_PRINT("left_slope=" << left_slope << ", S=" << S);
            for (unsigned int i = 0; i < max_segment_num; ++i)
                this->args.slope[i] += left_slope;

            for (unsigned int i = 0; i < max_segment_num - 1; ++i)
                this->args.slope[i] -= this->args.slope[i + 1];
            
            args.seg_nums = max_segment_num;
            return true;
        }
        return false;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        return -1;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            //AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "piecewise linear model 3";
    }
    #endif

    struct piecewise_linear_model_arguments{
        long double end[traits::MAX_SEGMENT_NUM], slope[traits::MAX_SEGMENT_NUM];
        unsigned int seg_nums;
    }args;
};

//template<_Tp>
//struct slope_gap{
//    long double slope;
//    _Tp gap;
//    unsigned int pos;
//};

template<typename _Tp,
        typename traits>
class PDM{
public:
    typedef _Tp key_type;

    typedef PDM<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    //typedef slope_gap<_Tp> sg;

    PDM(){
        this->args.seg_nums = 0;
        std::fill(this->args.slope, this->args.slope + traits::MAX_MODEL_ARGS, 0);
    }

    ~PDM(){}


    static long double S[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE], segment_slope[traits::MAX_MODEL_DP_SEGMENT_SIZE];
    static int ans[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];
    static key_type segment_start[traits::MAX_MODEL_DP_SEGMENT_SIZE];

    // return the predict position. value range from 0 to +inf.
    forceinline long double predict(const key_type &key) const {
        long double ret = 1;
        for (unsigned int i = 0; i < args.seg_nums; ++i){
            //ret += (key < args.end[i]) * ((key - args.end[i]) * args.slope[i]);
            //ret += std::min(args.delta_pos[i], (key < args.end[i]) * (key - args.end[i]) * args.slope[i]);
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
            //AEX_PRINT((key > args.start[i]) * (key < args.end[i]) * ((key - args.end[i]) * args.slope[i]));
        }
        //AEX_PRINT("key=" << key << ", ret=" << ret);
        return ret;
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        //AEX_PRINT("n=" << n);
        //AEX_ASSERT(n >= traits::MIN_ML_INNER_NODE_SIZE);
        
        //const long double max_density = 2.0 / (1.0 + 1.0 / density); //harmonic mean
        const long double min_gap = 1.0 / slot_size;
        const slot_type max_offset = traits::ERROR_BOUND - 2;
        
        std::vector<long double> slope(n);
        for  (slot_type i = 0; i < n - 1; ++i){
            if (key[i + 1] == key[i])
                return false;
            slope[i] = min_gap / (key[i + 1] - key[i]);
            for  (slot_type j = 2; j < max_offset && i + j < n; ++j){
                slope[i] = std::min(slope[i], min_gap * (j + 1) / (key[i + j] - key[i]));
            }
            if (i + max_offset >= n)
                slope[i] = std::min(slope[i - 1], slope[i]);
        }
        const double density = 1.0 * n / slot_size;
        constexpr int windows[3] = {64, 4096, 262144};
        //2^6  2^12 2^18
        int windows_sz = (windows[0] < n) + (windows[1] < n) + (windows[2] < n);
        //long double min_density[3] = {(1 + density) / (windows_sz + 1), (1 + density) / (windows_sz + 1) * 2, (1 + density) / (windows_sz + 1) * 3};
        //AEX_PRINT("windows_sz=" << windows_sz);
        for (int i = 0; i < windows_sz; ++i){
            //long double min_density = (1 + density) / (windows_sz + 1) * (i + 1);
            long double min_density = 1 - (1 - density) / (windows_sz + 1) * (i + 1);
            for (slot_type j = 0; windows[i] + j + 1 < n; ++j){
                slope[j] = std::max(slope[j], windows[i] * min_gap / min_density / (key[j + windows[i] + 1] - key[j]) );
            }
        }
        
        //auto slope = [&key, n, min_gap, max_offset, &windows, &min_density, windows_sz](int i){
        //    //2^6 2^12 2^18
        //    long double ret;
        //    ret = min_gap / (key[i + 1] - key[i]);
        //    for  (slot_type j = 2; j < max_offset && i + j < n; ++j)
        //        ret = std::min(ret, min_gap * (j + 1) / (key[i + j] - key[i]));
        //    if (i + max_offset >= n)
        //        ret = std::min(ret, min_gap * max_offset / (key[n - 1] - key[i]));
        //    for (int j = 0; j < windows_sz && i + windows[j] + 1 < n; ++j){
        //        ret = std::max(ret, windows[j] * min_gap / min_density[j] / (key[i + windows[j] + 1] - key[i]));
        //    }
        //    return ret;
        //};

        //for (slot_type i = 0; i < n - 1; ++i)
        //    AEX_PRINT("slope[" << i << "]=" << slope[i] << ", key=" << key[i]);
        
        std::fill(args.end, args.end + traits::MAX_SEGMENT_NUM, std::numeric_limits<key_type>::lowest());
        std::fill(args.slope, args.slope + traits::MAX_SEGMENT_NUM, 0);
        
        const int M = std::min(static_cast<slot_type>(sqrt(n)), 1 << traits::MAX_SEGMENT_NUM);
        int N = 2 * M;
        
        //constexpr int MAX_SEGMENT_CANDICATE = (1 << (traits::MAX_SEGMENT_NUM + 1)) + 1;

        long double segment_length = (key[n - 1] - key[0]) / M;
        for (slot_type i = 0; i < M; ++i){
            segment_start[i * 2] = key[0] + segment_length * i;
            segment_start[i * 2 + 1] = key[n / M * i];
        }
        
        std::sort(segment_start, segment_start + N);
        N = std::unique(segment_start, segment_start + N) - segment_start;
        segment_start[N] = key[n - 1];
        std::fill(segment_slope, segment_slope + N, 0);
        int key_id = 0;
        for (slot_type i = 0; i < N; ++i){
            if (segment_start[i] >= key[key_id + 1]) 
                ++key_id;
            while (key_id < n && segment_start[i + 1] >= key[key_id]){
                //segment_slope[i] = std::max(segment_slope[i], slope[key_id]);
                segment_slope[i] = std::max(segment_slope[i], slope[key_id]);
                ++key_id;
            }
            --key_id;
        }

        //for (int i = 0; i < N; ++i)
        //    AEX_PRINT("segment_start[" << i << "]=" << segment_start[i] << ", slope=" << segment_slope[i]);
        
        long double max_slope = segment_slope[0];
        // initial
        for (slot_type i = 0; i < N; ++i){
            max_slope = std::max(max_slope, segment_slope[i]);
            S[0][i] = (segment_start[i + 1] - segment_start[0]) * max_slope;
            ans[0][i] = -1;
        }
        int segment_num = 1;
        // Dynamic Programing
        for (int &k = segment_num; k < traits::MAX_SEGMENT_NUM && S[k - 1][N - 1] > 1; ++k){
            S[k][0] = (segment_start[1] - segment_start[0]) * segment_slope[0];
            ans[k][0] = -1;
            for (slot_type i = 1; i < N; ++i){
                long double max_slope = segment_slope[i];
                S[k][i] = std::numeric_limits<long double>::max();
                //S[k][i] = (segment_start[i + 1] - segment_start[0]) * max_slope;
                ans[k][i] = -1;
                for (slot_type j = i; j >= 1; --j){
                    max_slope = std::max(max_slope, segment_slope[j]);
                    if (S[k][i] > S[k - 1][j - 1] + max_slope * (segment_start[i + 1] - segment_start[j])){
                        S[k][i] = S[k - 1][j - 1] + max_slope * (segment_start[i + 1] - segment_start[j]);
                        ans[k][i] = j - 1;
                    }
                }
            }
        }

        if (S[segment_num - 1][N - 1] > 1){
            return false;
        }
        else{
            //AEX_PRINT("S[" << segment_num - 1 << "][" << N - 1 << "]=" << S[segment_num - 1][N - 1]);
            long double delta_slope = 0;
            for (int i = 0, segment_end = N - 1; i < segment_num; ++i){
                this->args.end[i] = segment_start[segment_end + 1];
                key_type start = ans[segment_num - i - 1][segment_end];
                long double max_slope = 0;
                for (int j = start + 1; j <= segment_end; ++j)
                    max_slope = std::max(max_slope, segment_slope[j]);
                
                this->args.slope[i] = max_slope - delta_slope;
                delta_slope = max_slope;
                segment_end = start;
            }
            long double left_slope = (1 - S[segment_num - 1][N - 1]) / (key[n - 1] - key[0]);
            this->args.seg_nums = segment_num;
            this->args.slope[0] += left_slope;
            //for (slot_type i = 0; i < segment_num; ++i)
            //    AEX_PRINT("slope[" << i << "]=" << this->args.slope[0] << ", end=" << this->args.end[i]);
        }

        //for (unsigned int i = 0; i < this->args.seg_nums; ++i)
        //    AEX_PRINT("end=" << this->args.end[i] << ", slope=" << this->args.slope[i] << ", ");
        //AEX_PRINT("example:" << this->predict(key[0]) << ", " << this->predict(key[n / 2]) << ", " << this->predict(key[n - 1]));
        //AEX_PRINT(this->max_error(key, n, slot_size));
        return true;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        return -1;
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

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "piecewise linear model 4";
    }
    #endif

    struct piecewise_linear_model_arguments{
        unsigned int seg_nums;
        long double end[traits::MAX_MODEL_ARGS], slope[traits::MAX_MODEL_ARGS];
    }args;
};

template<typename _Tp, typename traits>
long double PDM<_Tp, traits>::S[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
long double PDM<_Tp, traits>::segment_slope[traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
int PDM<_Tp, traits>::ans[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
_Tp PDM<_Tp, traits>::segment_start[traits::MAX_MODEL_DP_SEGMENT_SIZE];


template<typename _Tp,
        typename traits>
class PDM_hash_table{
public:
    typedef _Tp key_type;

    typedef PDM_hash_table<key_type, traits> self;

    typedef typename traits::slot_type slot_type;

    //typedef slope_gap<_Tp> sg;

    PDM_hash_table(){
        this->args.seg_nums = 0;
        std::fill(this->args.slope, this->args.slope + traits::MAX_MODEL_ARGS, 0);
    }

    ~PDM_hash_table(){}


    static long double S[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE], segment_slope[traits::MAX_MODEL_DP_SEGMENT_SIZE];
    static int ans[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];
    static key_type segment_start[traits::MAX_MODEL_DP_SEGMENT_SIZE];

    // return the predict position. value range from 0 to +inf.
    forceinline int predict(const key_type &key) const {
        long double ret = this->args.slot_size;
        for (unsigned int i = 0; i < args.seg_nums; ++i){
            //ret += (key < args.end[i]) * ((key - args.end[i]) * args.slope[i]);
            //ret += std::min(args.delta_pos[i], (key < args.end[i]) * (key - args.end[i]) * args.slope[i]);
            ret += (key < args.end[i]) * (key - args.end[i]) * args.slope[i];
            //AEX_PRINT((key > args.start[i]) * (key < args.end[i]) * ((key - args.end[i]) * args.slope[i]));
        }
        //AEX_PRINT("key=" << key << ", ret=" << ret);
        return (int)ret;
    }

    bool train(const key_type* const key, const slot_type n){
        return false;
    }

    bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        //AEX_PRINT("n=" << n);
        //AEX_ASSERT(n >= traits::MIN_ML_INNER_NODE_SIZE);
        
        //const long double max_density = 2.0 / (1.0 + 1.0 / density); //harmonic mean
        //const long double min_gap = 1.0 / slot_size;
        this->args.slot_size = slot_size - 1;
        if (ceil(sqrt(n)) > traits::MAX_MODEL_DP_SEGMENT_SIZE)
            return false;

        const long double min_gap = 1.0;
        const slot_type max_offset = traits::ERROR_BOUND / 2 - 2;
        
        std::vector<long double> slope(n);
        for  (slot_type i = 0; i < n - 1; ++i){
            if (key[i + max_offset] == key[i])
                return false;
            //slope[i] = min_gap / (key[i + 1] - key[i]);
            //for  (slot_type j = 2; j < max_offset && i + j < n; ++j){
            //    slope[i] = std::min(slope[i], min_gap * j / (key[i + j] - key[i]));
            //}
            //AEX_PRINT("ori_slope=" << slope[i]);
            if (i + max_offset >= n)
                slope[i] = std::min(slope[i - 1], slope[i]);
            else{ 
                slope[i] = min_gap / (key[i + max_offset] - key[i]);
              //AEX_PRINT("i=" << i << ", key=" << key[i] << ", slope=" << slope[i] << ", gap=" << min_gap << ", " << slope[i] * (key[i + max_offset] - key[i]));
            }
        }
        const double density = 1.0 * n / args.slot_size;
        constexpr int windows[3] = {64, 4096, 262144};
        //2^6  2^12 2^18
        int windows_sz = (windows[0] < n) + (windows[1] < n) + (windows[2] < n);
        //long double min_density[3] = {(1 + density) / (windows_sz + 1), (1 + density) / (windows_sz + 1) * 2, (1 + density) / (windows_sz + 1) * 3};
        //AEX_PRINT("windows_sz=" << windows_sz);
        for (int i = 0; i < windows_sz; ++i){
            long double min_density = 1 - (1 - density) / (windows_sz + 1) * (i + 1);
            //AEX_PRINT("min_density=" << min_density);
            for (slot_type j = 0; windows[i] + j + 1 < n; ++j){
                slope[j] = std::max(slope[j], windows[i] * min_gap / min_density / (key[j + windows[i] + 1] - key[j]) );
            }
        }
        
        //auto slope = [&key, n, min_gap, max_offset, &windows, &min_density, windows_sz](int i){
        //    //2^6 2^12 2^18
        //    long double ret;
        //    ret = min_gap / (key[i + 1] - key[i]);
        //    for  (slot_type j = 2; j < max_offset && i + j < n; ++j)
        //        ret = std::min(ret, min_gap * (j + 1) / (key[i + j] - key[i]));
        //    if (i + max_offset >= n)
        //        ret = std::min(ret, min_gap * max_offset / (key[n - 1] - key[i]));
        //    for (int j = 0; j < windows_sz && i + windows[j] + 1 < n; ++j){
        //        ret = std::max(ret, windows[j] * min_gap / min_density[j] / (key[i + windows[j] + 1] - key[i]));
        //    }
        //    return ret;
        //};

        //for (slot_type i = 0; i < n - 1; ++i)
        //    AEX_PRINT("slope[" << i << "]=" << slope[i] << ", key=" << key[i]);
        
        std::fill(args.end, args.end + traits::MAX_SEGMENT_NUM, std::numeric_limits<key_type>::lowest());
        std::fill(args.slope, args.slope + traits::MAX_SEGMENT_NUM, 0);
        
        const int M = std::max(traits::MAX_SEGMENT_NUM, std::min(static_cast<slot_type>(ceil(sqrt(n))), 1 << traits::MAX_SEGMENT_NUM));
        int N = 2 * M;
            //AEX_PRINT("M=" << M);

            //constexpr int MAX_SEGMENT_CANDICATE = (1 << (traits::MAX_SEGMENT_NUM + 1)) + 1;
        long double segment_length = (key[n - 1] - key[0]) / M;
        for (slot_type i = 0; i < M; ++i){
            segment_start[i * 2] = key[0] + segment_length * i;
            segment_start[i * 2 + 1] = key[n / M * i];
        }

        std::sort(segment_start, segment_start + N);
        N = std::unique(segment_start, segment_start + N) - segment_start;
        segment_start[N] = key[n - 1];
        std::fill(segment_slope, segment_slope + N, 0);
        int key_id = 0;
        for (slot_type i = 0; i < N; ++i){
            //AEX_PRINT("segnemt_start=" << segment_start[i]);
            if (segment_start[i] >= key[key_id + 1]) 
                ++key_id;
            while (key_id < n && segment_start[i + 1] > key[key_id]){
                //segment_slope[i] = std::max(segment_slope[i], slope[key_id]);
                //AEX_PRINT("i=" << i << "key_id=" << key_id << ", key=" << key[key_id] << ", segment_slope=" <<segment_slope[i]);
                segment_slope[i] = std::max(segment_slope[i], slope[key_id]);
                ++key_id;
            }
            --key_id;
        }

        //AEX_PRINT("N=" << N);
        //for (int i = 0; i < N; ++i)
        //    AEX_PRINT("segment_start[" << i << "]=" << segment_start[i] << ", slope=" << segment_slope[i]);
        
        long double max_slope = segment_slope[0];
        // initial
        for (slot_type i = 0; i < N; ++i){
            max_slope = std::max(max_slope, segment_slope[i]);
            S[0][i] = (segment_start[i + 1] - segment_start[0]) * max_slope;
            ans[0][i] = -1;
        }
        int segment_num = 1;
        // Dynamic Programing
        for (int &k = segment_num; k < traits::MAX_SEGMENT_NUM; ++k){
            S[k][0] = (segment_start[1] - segment_start[0]) * segment_slope[0];
            ans[k][0] = -1;
            for (slot_type i = 1; i < N; ++i){
                long double max_slope = segment_slope[i];
                S[k][i] = std::numeric_limits<long double>::max();
                //S[k][i] = (segment_start[i + 1] - segment_start[0]) * max_slope;
                ans[k][i] = -1;
                for (slot_type j = i; j >= 1; --j){
                //for (slot_type j = i; j >= 1; --j){
                    max_slope = std::max(max_slope, segment_slope[j]);
                    if (S[k][i] > S[k - 1][j - 1] + max_slope * (segment_start[i + 1] - segment_start[j])){
                        S[k][i] = S[k - 1][j - 1] + max_slope * (segment_start[i + 1] - segment_start[j]);
                        ans[k][i] = j - 1;
                    }
                }
            }
        }

        if (S[segment_num - 1][N - 1] > args.slot_size){
            return false;
        }
        else{
            //AEX_PRINT("S[" << segment_num - 1 << "][" << N - 1 << "]=" << S[segment_num - 1][N - 1]);
            long double delta_slope = 0;
            for (int i = 0, segment_end = N - 1; i < segment_num; ++i){
                this->args.end[i] = segment_start[segment_end + 1];
                key_type start = ans[segment_num - i - 1][segment_end];
                long double max_slope = 0;
                for (int j = start + 1; j <= segment_end; ++j)
                    max_slope = std::max(max_slope, segment_slope[j]);
                
                this->args.slope[i] = max_slope - delta_slope;
                delta_slope = max_slope;
                segment_end = start;
            }
            this->args.seg_nums = segment_num;
            if (S[segment_num - 1][N - 1] >= 1){
                long double left_slope = this->args.slot_size / S[segment_num - 1][N - 1];
                for (int i = 0; i < segment_num; ++i)
                    this->args.slope[i] *= left_slope;
            }
            else{
                long double left_slope = (1 - S[segment_num - 1][N - 1]) / (key[n - 1] - key[0]);
                this->args.slope[0] += left_slope;
            }
            //for (slot_type i = 0; i < segment_num; ++i)
            //    AEX_PRINT("slope[" << i << "]=" << this->args.slope[0] << ", end=" << this->args.end[i]);
        }

        //for (unsigned int i = 0; i < this->args.seg_nums; ++i)
        //    AEX_PRINT("end=" << this->args.end[i] << ", slope=" << this->args.slope[i] << ", ");
        //AEX_PRINT("example:" << this->predict(key[0]) << ", " << this->predict(key[n / 2]) << ", " << this->predict(key[n - 1]));
        //AEX_PRINT(this->max_error(key, n, slot_size));
        return true;
    }

    inline long double RMSE(const key_type* const key, const slot_type n){
        return -1;
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

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "piecewise linear model 4";
    }
    #endif

    struct piecewise_linear_model_arguments{
        unsigned int seg_nums, slot_size;
        long double end[traits::MAX_MODEL_ARGS], slope[traits::MAX_MODEL_ARGS];
    }args;
};

template<typename _Tp, typename traits>
long double PDM_hash_table<_Tp, traits>::S[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
long double PDM_hash_table<_Tp, traits>::segment_slope[traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
int PDM_hash_table<_Tp, traits>::ans[traits::MAX_SEGMENT_NUM][traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp, typename traits>
_Tp PDM_hash_table<_Tp, traits>::segment_start[traits::MAX_MODEL_DP_SEGMENT_SIZE];

template<typename _Tp,
        typename traits>
class normal_linear_model{
public:
    typedef _Tp key_type;

    typedef typename traits::slot_type slot_type;

    // return the predict position. value range from 0 to +inf.
    inline double predict(const key_type &key) const {
        //return static_cast<slot_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        return args.slope * (key - args.start);
    }

    inline bool train(const key_type* const key, const slot_type n, const slot_type slot_size){
        if (n < slot_size)
            return false;
        args.start = key[0];
        args.slope = 1.0 / (key[n - 1] - key[0]);
        int size = 1, tot_size = 0, diff_size = 0;
        slot_type prev_pos = 0;
        for (int i = 1; i < n; ++i){
            slot_type pos = static_cast<int>(this->predict(key[i]));
            if (pos != prev_pos){
                ++diff_size;
                tot_size += (size > traits::NORMAL_MAX_COLLISION) ? 1 : size;
                size = 0;
                pos = prev_pos;
            }
            else 
                ++size;
        }
        ++diff_size;
        tot_size += (size > traits::NORMAL_MAX_COLLISION) ? 1 : size;
        double density = 1.0 * tot_size / slot_size;
        if (1.0 * tot_size / slot_size < traits::MIN_NORMAL_DENSITY_RATIO)
            return false;
        //if (density < density_bound::MIN_DENSITY_RATIO || density > density_bound::MAX_DENSITY_RATIO) 
        //    return false;
        return true;
    }

    // train model with an key array, array size n and slot size
    inline bool train(const key_type* const key, const slot_type n){
        args.start = key[0];
        args.slope = 1.0 / (key[n - 1] - key[0]);
        return true;
    }

    inline slot_type max_error(const key_type* const key, const slot_type n, const slot_type slot_size){
        
        slot_type error = 0;
        for (slot_type i = 0, start = 0; i < n; ++i){
            slot_type pos = std::max(0, static_cast<slot_type>(this->predict(key[i]) * slot_size));
            start = std::max(start, pos);
            AEX_PRINT("key=" << key[i] << ", pos=" << pos << ", start=" << start);
            error = std::max(error, start - pos);
            ++start;
        }
        return error;
    }

    inline double RMSE(const key_type* const key, const slot_type n){
        double sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "gap array linear model";
    }
    #endif

    struct linear_arguments{
        double slope, start;
    }args;

};

template<typename _Tp,
        typename traits>
class FMCD{
public:
    typedef _Tp key_type;

    typedef typename traits::slot_type slot_type;

    // return the predict position. value range from -inf to +inf.
    inline long double predict(const key_type &key) const {
        //return static_cast<slot_type>(std::max(0, static_cast<int>(args.slope * key + args.inter)));
        //return this->args.slot_size + std::min(0, static_cast<slot_type>(args.slope * (key - args.end)));
        //return this->args.slot_size + (key < args.end) * args.slope * (key - args.end);
        return (key - this->args.start) * this->args.slope + 1;
    }

    inline bool train(const key_type* const keys, const slot_type n, const slot_type slot_size){
        AEX_ASSERT(n > 2);
        AEX_ASSERT(slot_size >= traits::MIN_HASH_NODE_SLOT_SIZE);
        slot_type H = 1, T = n - 1;
        slot_type S = n / 8, E = n / 8 * 7;
        while (S < E && keys[S] == keys[0])
            ++S;
        if (S >= E || keys[S] == keys[E])
            return false;
        while (H < S && 1.0 * (S - H) / (keys[S] - keys[H]) < 0.5 * (E - S) / (keys[E] - keys[S]))
            ++H;
        while (T > E && 1.0 * (T - E) / (keys[T] - keys[E]) < 0.5 * (E - S) / (keys[E] - keys[S]))
            --T;
        args.start = keys[H];
        args.slope = 1.0 * (slot_size - 2) / (keys[T] - keys[H]);
        //AEX_PRINT("keys[0]=" << keys[0] << ", pos[0]=" << this->predict(keys[0]));
        //if (this->predict(keys[0]) > 0){
            //AEX_PRINT("n=" << n << ", keys[n-1]=" << keys[n - 1] << ", slot_size=" << slot_size << ", slope=" << this->args.slope << "start=" << start << ", key[0]=" << keys[0] << ", key[start]" << keys[start] << ", " << this->predict(keys[0]) << ", " << this->predict(keys[start]) << ", " << (keys[0] - this->args.start) * this->args.slope + 1);
        //}
        //offset = (8 * traits::SLOT_PER_LOCK * this->args.slope < (this->args.start - keys[0])) ? (HASH_NODE_MAX_GAP_SLOT * traits::SLOT_PER_LOCK) : -((keys[0] - this->args.start) * this->args.slope);
        AEX_ASSERT(static_cast<slot_type>(this->predict(keys[0])) <= 0);
        return true;
    }

    // train model with an key array, array size n and slot size
    //inline bool train(const key_type* const key, const slot_type n){
    //    args.end = key[n - 1];
    //    args.slope = 1.0 / (key[n - 1] - key[0]);
    //    return true;
    //}

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

    inline double RMSE(const key_type* const key, const slot_type n){
        double sum = 0;
        for (slot_type i = 0; i < n; ++i){
            sum += sqr(this->predict(key[i]) * (n - 1) - i);
        }
        sum /= n;
        sum = sqrt(sum);
        return sum;
    }

    #ifdef AEX_DEBUG
    inline static std::string name(){
        return "gap array linear model";
    }
    #endif

    struct linear_arguments{
        long double slope, start;
        //int offset;
    }args;

};


}

