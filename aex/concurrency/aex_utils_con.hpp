#pragma once
namespace aex{

template<typename traits, bool _ = traits::AllowConcurrecny>
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

template<typename traits>
class aex_bitmap_impl<traits, true>{
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



}