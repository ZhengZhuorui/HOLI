#pragma once

#include "aex/aex.h"

namespace aex
{

template<typename _Key, typename _Val>
class aex_map{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef std::pair<key_type, value_type> kv_type;

    typedef aex_tree<key_type, value_type, aex_traits<key_type, value_type, std::false_type, std::false_type> > tree;

    typedef typename tree::size_type size_type;

    typedef typename tree::iterator iterator;

    typedef typename tree::const_iterator const_iterator;

    typedef typename tree::reverse_iterator reverse_iterator;

    typedef typename tree::const_reverse_iterator const_reverse_iterator;

private:
    tree _m;

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

    inline size_type size() const { return _m.size(); }

    inline bool empty() const { return _m.empty(); }

    inline bool exists(const key_type &key) { return _m.exists(key); }

    inline iterator find(const key_type &key) { return _m.find(key); }

    inline const_iterator find(const key_type &key) const { return _m.exists(key); }

    inline size_t count(const key_type &key) const { return _m.count(key); }

    inline iterator lower_bound(const key_type &key) {
        return _m.lower_bound(key);
    }

    inline const_iterator lower_bound(const key_type &key) const {
        return _m.lower_bound(key);
    }

    inline iterator upper_bound(const key_type &key) {
        return _m.upper_bound(key);
    }

    inline const_iterator upper_bound(const key_type &key) const {
        return _m.upper_bound(key);
    }

    inline std::pair<iterator, bool> insert(const std::pair<key_type, value_type> &x){
        return _m.insert(x.first, x.second);
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
    /*
    inline std::pair<iterator, iterator> equal_range(const key_type &key){
        return _m.equal_range(key);
    }

    inline std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const{
        return _m.equal_range(key);
    }
    */

    #ifdef AEX_DEBUG_MSG
    void set_debug_level(int level){
        _m.debug_level = level;
    }

    inline bool debug_error(){
        return _m.debug_error();
    }

    #endif
};

} // namespace aex
