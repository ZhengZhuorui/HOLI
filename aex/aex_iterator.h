#pragma once


namespace aex{
template<typename _Key, typename _Val, typename traits> class aex_tree;
template<typename _Key, typename _Val, typename traits> class aex_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_iterator;
template<typename _Key, typename _Val, typename traits> class aex_reverse_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_reverse_iterator;

template<typename _Key,
        typename _Val,
        typename traits>
class aex_iterator{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef typename traits::pos_type pos_type;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef aex_iterator<_Key, _Val, traits> self;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    inline aex_iterator() : _M_node(nullptr), offset(0){}
    
    inline aex_iterator(data_node* ptr, pos_type _offset):_M_node(ptr), offset(_offset){}
    
    //inline aex_iterator(const aex_reverse_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}        
    
    reference operator*(){
        return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator++(){
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return *this;
    }

    self& operator++(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return tmp;
    }

    self& operator--(){
        if (offset > 0) --offset;
        else{
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return *this;
    }
    self& operator--(int){
        self tmp = *this;
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return tmp;
    }

    bool operator==(const self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    bool operator!=(const self& x) const{ 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }

    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline value_type& data() const {
        return _M_node->data[offset];
    }

    inline data_node_ptr get_node(){
        return _M_node;
    }

#ifndef AEX_DEBUG
protected:

private:
#endif
    friend class aex_const_iterator<_Key, _Val, traits>;

    //friend class aex_reverse_iterator<_Key, _Val, traits>;
    
    friend class aex_const_reverse_iterator<_Key, _Val, traits>;

    friend class aex_tree<_Key, _Val, traits>;
    
    data_node_ptr _M_node;

    pos_type offset;

};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_const_iterator{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef typename traits::pos_type pos_type;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef aex_const_iterator<_Key, _Val, traits> self;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    inline aex_const_iterator() : _M_node(nullptr), offset(0){}
    
    inline aex_const_iterator(const data_node* ptr, pos_type _offset):_M_node(ptr), offset(_offset){}
    
    inline aex_const_iterator(const aex_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    
    
    //inline aex_const_iterator(const aex_reverse_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    
    
    //inline aex_const_iterator(const aex_const_reverse_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    

    
    inline reference operator*() const{
        //return static_cast<_Link_ptr>(_M_node->item[offset]);
        return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator++(){
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return *this;
    }

    self& operator++(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return tmp;
    }
    
    self& operator--(){
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return *this;
    }
    self& operator--(int){
        self tmp = *this;
        if (offset > 0)--offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return tmp;
    }

    bool operator==(const self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    bool operator!=(const self& x) const{ 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline const value_type& data() const {
        return _M_node->data[offset];
    }

    inline data_node_ptr get_node(){
        return _M_node;
    }

#ifndef AEX_DEBUG
protected:

private:
#endif
    friend class aex_iterator<_Key, _Val, traits>;

    //friend class aex_reverse_iterator<_Key, _Val, traits>;
    
    //friend class aex_const_reverse_iterator<_Key, _Val, traits>;

    friend class aex_tree<_Key, _Val, traits>;

    data_node_ptr _M_node;

    pos_type offset;

};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_reverse_iterator{
public:

    typedef _Key key_type;
    
    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef typename traits::pos_type pos_type;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef aex_reverse_iterator<_Key, _Val, traits> self;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    inline aex_reverse_iterator() : _M_node(nullptr), offset(0){}
    
    inline aex_reverse_iterator(const data_node* ptr, pos_type _offset):_M_node(ptr), offset(_offset){}
    
    inline aex_reverse_iterator(const aex_iterator<key_type, value_type, traits> &it) : _M_node(it._M_node), offset(it.offset){}    
    
    reference operator*(){
        return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator--(){
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return *this;
    }

    self& operator--(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return tmp;
    }

    self& operator++(){
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return *this;
    }
    self& operator++(int){
        self tmp = *this;
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return tmp;
    }

    inline bool operator==(const self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    inline bool operator!=(const self& x) const { 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline value_type& data() const {
        return _M_node->data[offset];
    }

    inline data_node_ptr get_node() const{
        return _M_node;
    }

#ifndef AEX_DEBUG
protected:

private:
#endif

    friend class aex_iterator<_Key, _Val, traits>;

    friend class aex_const_iterator<_Key, _Val, traits>;
    
    friend class aex_const_reverse_iterator<_Key, _Val, traits>;

    friend class aex_tree<_Key, _Val, traits>;

    data_node_ptr _M_node;

    pos_type offset;
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_const_reverse_iterator{
public:

    typedef _Key key_type;
    
    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef typename traits::pos_type pos_type;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef aex_const_reverse_iterator<_Key, _Val, traits> self;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;

    inline aex_const_reverse_iterator() : _M_node(nullptr), offset(0){}
    
    inline aex_const_reverse_iterator(const data_node* ptr, pos_type _offset):_M_node(ptr), offset(_offset){}
    
    inline aex_const_reverse_iterator(const aex_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    

    inline aex_const_reverse_iterator(const aex_const_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    

    inline aex_const_reverse_iterator(const aex_reverse_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    
    
    reference operator*(){
        return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator--(){
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return *this;
    }

    self& operator--(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            if (_M_node->next != nullptr){
                offset = 0;
                _M_node = static_cast<data_node_ptr>(_M_node->next);
            }
            else{
                offset = _M_node->size;
            }
        }
        return tmp;
    }

    self& operator++(){
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return *this;
    }
    self& operator++(int){
        self tmp = *this;
        if (offset > 0) --offset;
        else {
            if (_M_node != nullptr){
                _M_node = static_cast<data_node_ptr>(_M_node->prev);
                offset = _M_node->size - 1;
            }
            else{
                offset = 0;
            }
        }
        return tmp;
    }

    inline bool operator==(const self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    inline bool operator!=(const self& x) const { 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline const value_type& data() const{
        return _M_node->data[offset];
    }

    inline data_node_ptr get_node() const{
        return _M_node;
    }

#ifndef AEX_DEBUG
protected:

private:
#endif

    friend class aex_iterator<_Key, _Val, traits>;

    friend class aex_const_iterator<_Key, _Val, traits>;

    friend class aex_reverse_iterator<_Key, _Val, traits>;

    friend class aex_tree<_Key, _Val, traits>;

    data_node_ptr _M_node;

    pos_type offset;

};

}