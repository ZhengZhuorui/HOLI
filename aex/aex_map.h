 #pragma once

#include "aex/aex.h"

namespace aex
{

template<typename _Key, 
        typename _Val, 
        typename _Alloc = std::allocator<unsigned char>,
        typename traits=aex_default_traits<_Key, _Val, std::false_type> >
class aex_map{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef std::pair<key_type, value_type> kv_type;

    typedef aex_tree<key_type, value_type, traits> tree;

    typedef typename tree::size_type size_type;

    typedef typename tree::iterator iterator;

    typedef typename tree::const_iterator const_iterator;

    typedef typename tree::reverse_iterator reverse_iterator;

    typedef typename tree::const_reverse_iterator const_reverse_iterator;

    typedef typename tree::aex_stats stats;

//#ifndef AEX_EXPERIMENT
private:
    tree _m;
//#endif

public:
    explicit inline aex_map() : _m(){}

    template<typename InputIterator>
    inline aex_map(InputIterator first, InputIterator last) : _m(first, last) {}

    inline ~aex_map(){}

    void clear(){_m.clear();}

    inline iterator begin() { return _m.begin(); }

    inline iterator end() { return _m.end(); }

    inline iterator begin() const { return _m.begin(); }

    inline iterator end() const { return _m.end(); }

    inline iterator rbegin() { return _m.rbegin(); }

    inline iterator rend() { return _m.rend(); }

    inline iterator rbegin() const { return _m.rbegin(); }

    inline iterator rend() const { return _m.rend(); }

    inline size_t size() const { return _m.size(); }

    inline bool empty() const { return _m.empty(); }

    inline bool exists(const key_type &key) { return _m.exists(key); }

    inline iterator find(const key_type &key) { return _m.find(key); }

    inline const_iterator find(const key_type &key) const { return _m.find(key); }

    inline void range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer){
        return _m.range_query(lower_key, upper_key, answer);
    }

    inline size_t erase(const key_type &key){ return _m.erase(key);}
    
    inline iterator erase(const_iterator iter){ return _m.erase(iter);}

    inline size_t count(const key_type &key) { return _m.count(key); }

    inline iterator lower_bound(const key_type &key) {
        return _m.lower_bound(key);
    }

    inline iterator upper_bound(const key_type &key) {
        return _m.upper_bound(key);
    }

    inline std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return _m.insert(x);
    }

    inline std::pair<iterator, bool> insert(const key_type &key, const value_type &data){
        return _m.insert(key, data);
    }

    inline value_type& operator[](const key_type &key){
        iterator iter = insert(std::pair<key_type, value_type>(key, value_type())).first;
        return iter.value();
    }

    inline void bulk_load(const std::pair<key_type, value_type>* const data, const int nums){
        _m.bulk_load(data, nums);
    }

    inline const stats& get_stats() const{
        return _m.get_stats();
    }

    inline void print_stats(){
        return _m.print_stats();
    }

    inline void print_detail(){
        return _m.print_detail();
    }
    /*
    inline std::pair<iterator, iterator> equal_range(const key_type &key){
        return _m.equal_range(key);
    }

    inline std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const{
        return _m.equal_range(key);
    }
    */

    inline size_type memory_used()const{
        return _m._memory_used();
    }

    #ifdef AEX_DEBUG
    void set_debug_level(int level){
        tree::debug_level = level;
    }

    inline bool debug_error(){
        return _m.debug_error();
    }
    #endif
};

} // namespace aex
