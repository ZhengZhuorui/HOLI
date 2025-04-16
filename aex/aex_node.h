#pragma once
#include "aex_utils.h"
namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_base{
public:
    typedef _Key key_type;
    typedef _Val value_type;
    typedef aex_node_base<key_type, value_type, traits> self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename traits::slot_type slot_type;
    typedef typename base_tree::components      components;
    typedef typename components::size_type      size_type;
    typedef typename components::version_type   version_type;
    typedef typename components::ref_count_type ref_count_type;
    typedef typename components::inner_node     inner_node;
    typedef typename components::hash_node      hash_node;
    typedef typename components::dense_node     dense_node;
    typedef typename components::data_node      data_node;
    typedef typename components::inner_node_ptr inner_node_ptr;
    typedef typename components::hash_node_ptr  hash_node_ptr;
    typedef typename components::dense_node_ptr dense_node_ptr;
    typedef typename components::data_node_ptr  data_node_ptr;
    typedef typename components::RWLock         RWLock;
    typedef typename components::Lock           Lock;

    explicit aex_node_base(NodeType _type) :  type(_type)          , node_lock(), size(0){}//, version(_version){}
    aex_node_base(aex_node_base &other_node): type(other_node.type), node_lock(), size(0){}//, version(0){}
    aex_node_base(aex_node_base &&other_node):type(other_node.type), node_lock(), size(0){}//, version(0){}

    aex_node_base& operator = (aex_node_base &other_node) {
        this->type = other_node.type;
        return *this;
    }

    aex_node_base& operator = (aex_node_base &&other_node) {
        this->type = other_node.type;
        return *this;
    }

    // size: the number of child nodes(inner node); the number of data(data node)
    NodeType               type;
    RWLock                 node_lock;
    size_type              size;
};


template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef aex_inner_node<_Key, _Val, traits> self;
    typedef _Key key_type;
    typedef _Val value_type;
    typedef typename traits::slot_type             slot_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components         components;
    typedef typename components::base_node         base_node;
    typedef typename components::RWLock            RWLock;
    //aex_inner_node(NodeType _type):base_node(_type), meta_lock(){}
    aex_inner_node(NodeType _type):base_node(_type){}
    ~aex_inner_node() = default;
    aex_inner_node(self &other) = delete;
    aex_inner_node& operator = (aex_inner_node &other) = delete;

    //RWLock meta_lock;
};


template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_node : public aex_inner_node<_Key, _Val, traits>{
public:
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_hash_node<_Key, _Val, traits>      self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename traits::slot_type             slot_type;
    typedef typename base_tree::components         components;
    //typedef aex_default_components<traits>     components;
    typedef typename components::Allocator         Allocator;
    typedef typename components::InnerNodeModel    Model;
    typedef typename traits::bitmap                bitmap;
    typedef typename traits::bitmap_base           bitmap_base;
    typedef typename traits::hash_type             hash_type;
    typedef typename components::base_node         base_node;
    typedef typename components::inner_node        inner_node;
    typedef typename components::hash_node         hash_node;
    typedef typename components::bitmap_impl       bitmap_impl;
    typedef typename components::node_ptr          node_ptr;
    typedef typename components::size_type         size_type;
    typedef typename components::Lock              Lock;
    typedef typename components::ID_type           ID_type;
    typedef typename components::version_type      version_type;

    //aex_hash_node(slot_type slot_size):inner_node(slot_size, NodeType::HashNode), bitmap_ptr(nullptr){
    //    init();
    //}
//
    //~aex_hash_node(){
    //    clear();
    //}   
    aex_hash_node():inner_node(NodeType::HashNode){};
    ~aex_hash_node(){};
    aex_hash_node(aex_hash_node &other){
        memcpy(this, &other, sizeof(self));
    }

    aex_hash_node& operator = (aex_hash_node &other){
        memcpy(this, &other, sizeof(self));
        return *this;
    }

    inline void clear(){
        if (this->bitmap_ptr != nullptr){
            if (this->slot_size / 64 + 3 > (slot_type)((Allocator::MAX_INNER_NODE_SIZE() - sizeof(hash_node)) / 8))
                delete[] this->bitmap_ptr;
            this->bitmap_ptr = nullptr;
        }
    }

    inline void init(){
        this->size = 0;
        if (this->slot_size / 64 + 3 > (slot_type)((Allocator::MAX_INNER_NODE_SIZE() - sizeof(hash_node)) / 8))
            this->bitmap_ptr = new bitmap_base[this->slot_size / 64 + 1]();
        else{
            this->bitmap_ptr = reinterpret_cast<bitmap>(reinterpret_cast<char*>(this) + sizeof(hash_node));
            memset(this->bitmap_ptr, 0, (this->slot_size / 64 + 1) * sizeof(bitmap_base));
        }
    }

    inline slot_type predict(const key_type key) const {
        //return std::min(std::max(static_cast<slot_type>(0), model.predict(key)), this->slot_size - 1);
        return std::max(0LL, static_cast<slot_type>(std::min(model.predict(key), static_cast<long double>(this->slot_size - 1))));
    }

    inline bool is_occupied(const slot_type x) const {
        return bitmap_impl::at(this->bitmap_ptr, x);
    }

    //inline bool is_occupied_con(const slot_type x) const {
    //    return bitmap_impl::at(this->bitmap_ptr, x);
    //}

    inline void add_size(){
        ++this->size;
    }

    inline void sub_size(){
        --this->size;
    }

    inline void set_one(const slot_type x) {
        bitmap_impl::set_one(this->bitmap_ptr, x);
        
    }

    inline void set_zero(const slot_type x) {
        bitmap_impl::set_zero(this->bitmap_ptr, x);
    }

    //inline slot_type prev_item_find(slot_type x) const {
    //    if (x <= 0)
    //        return x;
    //    bitmap text = bitmap_ptr + (x >> 6);
    //    bitmap_base base = (*text) << (63 - (x & 63));
    //    slot_type y = x & (~(traits::SLOT_PER_SHORTCUT - 1));
    //    x -= (base == 0) ? ((x & 63) + 1) : __builtin_clzll(base);
    //    while (base == 0 && x > 0){
    //        --text;
    //        base = *text;
    //        x -= __builtin_clzll(base);
    //    }
    //    return x;
    //}

    inline slot_type prev_item_find(slot_type x) const {
        const slot_type y = x & (~511);
        if (x <= 0)
            return x;
        bitmap text = bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) << (63 - (x & 63));
        if (base != 0)
            //return x - __builtin_clzll(base);
            return x - _lzcnt_u64(base);
        x = (x & (~63)) - 1;
        if (x < y)
            return y;
        --text;
        while (x - 64 > y && (*text) == 0){
            --text;
            x -= 64;
        }
        //x -= __builtin_clzll(*text) - ((*text) == 0);
        x -= (*text == 0) ? 63 : _lzcnt_u64(*text);
        return x;
    }

    inline slot_type prev_item_find_con(slot_type x, version_type &version) const {return prev_item_find(x);}

    inline slot_type prev_item(slot_type x) const {
        if (x <= 0)
            return x;
        bitmap text = bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) << (63 - (x & 63));
        if (base != 0)
            return x - __builtin_clzll(base);
        x -= (x & 63) + 1;
        //text[0] != 0
        --text;
        while ((*text) == 0){
            --text;
            x -= 64;
        }
        x -= __builtin_clzll(*text);
        return x;
    }

    inline slot_type next_item(slot_type x) const {
        if (x >= this->slot_size)
            return this->slot_size;
        bitmap text = bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) >> (x & 63);
        if (base != 0)
            return x + __builtin_ctzll(base);
        x += 64 - (x & 63);
        ++text;
        while (x < this->slot_size && (*text) == 0){
            ++text;
            x += 64;
        }
        if (x < this->slot_size)
            x += __builtin_ctzll((*text));
        return x;        
    }

    inline slot_type prev_item_con(slot_type x, version_type &version) const {return prev_item(x); }
    inline slot_type next_item_con(slot_type x, version_type &version) const {return next_item(x); }
    
    inline void array_lock(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_unlock(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_lock_shared(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_unlock_shared(const slot_type l_pos, const slot_type r_pos) const {}
    inline bool try_array_upgrade_lock(const slot_type l_pos, const slot_type r_pos) const {return true;}
    inline void array_downgrade_lock(const slot_type l_pos, const slot_type r_pos) const {}
    inline slot_type array_lock_shared_until_next_item(const slot_type prev_pos, const slot_type pos) const {return next_item(pos);}
    inline slot_type try_array_lock_shared_until_prev_item(const slot_type pos, bool &restart) const {return prev_item(pos);}
    inline void versionUpdate(const slot_type x){}
    inline void arrayCheckOrRestart(const slot_type start, const slot_type end, const version_type tot_version, bool &need_restart) const {}
    inline void arrayCheckOrRestart(const slot_type pos, const version_type version, bool &need_restart) const{}

    bitmap       bitmap_ptr;
    Model        model;
    slot_type    slot_size;
    ID_type      id;
    node_ptr     tail_node;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_dense_node : public aex_inner_node<_Key, _Val, traits>{
public:
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_dense_node<_Key, _Val, traits>     self;
    typedef typename traits::slot_type             slot_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components         components;
    //typedef aex_default_components<traits>     components;
    typedef typename components::node_ptr          node_ptr;
    typedef typename components::inner_node_ptr    inner_node_ptr;

    aex_dense_node() = delete;
    ~aex_dense_node() = delete;
    aex_dense_node(aex_dense_node &other) = delete;
    aex_dense_node& operator = (aex_dense_node &other) = delete;

    void init(){
        this->size = 0;
        this->is_parent = false;
    }

    key_type key_ptr[traits::DENSE_NODE_SLOT_SIZE];
    node_ptr child_ptr[traits::DENSE_NODE_SLOT_SIZE];
    bool is_parent;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_static_data_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef _Key key_type;
    typedef _Val value_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components components;
    //typedef aex_default_components<traits>     components;
    typedef typename components::base_node     base_node;
    typedef typename components::DataNodeModel Model;
    typedef typename components::version_type  version_type;
    typedef typename components::data_node     data_node;
    typedef base_node* node_ptr;
    typedef data_node* data_node_ptr;
    //typedef typename traits::slot_type slot_type;

    aex_static_data_node() : base_node(NodeType::LeafNode), next(nullptr){}//, is_sorted(true){}

    void init(){
        AEX_ASSERT(this->type == NodeType::LeafNode);
        this->next = nullptr;
        this->size = 0;
    }

    ~aex_static_data_node() {}

    aex_static_data_node(aex_static_data_node &other_node) :base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
    }

    aex_static_data_node(aex_static_data_node &&other_node) :base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
    }

    aex_static_data_node& operator = (aex_static_data_node &other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
        return *this;
    }

    aex_static_data_node& operator = (aex_static_data_node &&other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
        return *this;
    }

    inline void construct(const key_type *_key, const value_type *_data, int nums){
        std::copy(_key, _key + nums, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
    }

    inline void construct(const std::pair<key_type, value_type> *_data, int nums){
        AEX_ASSERT(nums >= traits::DATA_NODE_SLOT_SIZE / 2);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (int i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }


    // insert a item
    inline void insert(const key_type x, const value_type &value){
        int pos = this->find_lower_pos(x);
        insert(x, value, pos);
    }

    // insert a item in position
    inline void insert(const key_type x, const value_type &value, const int pos){
        AEX_ASSERT(this->size < traits::DATA_NODE_SLOT_SIZE);
        std::copy_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::copy_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = value;
        this->size++;
    }

    inline bool erase(const key_type x){
        int pos = find_lower_pos(x);
        if (pos >= this->size || key[pos] != x)
            return false;
        std::copy(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::copy(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->size--;
        return true;
    }

    inline int find(const key_type x) const {
        int pos;
        if constexpr (sizeof(key_type) == 8 && traits::DATA_NODE_SLOT_SIZE == 16){
            pos = cmp_eq_epi64x16(this->key, x);
        }
        else{
            //pos = find_lower_pos(x);
            int _size = std::min((int)this->size, traits::DATA_NODE_SLOT_SIZE);
            pos = std::lower_bound(this->key, this->key + _size, x) - this->key;
        }
        if (pos >= this->size || this->key[pos] != x)
            return this->size;
        return pos;
    }

    // if no item greater than or equal x, return slot_size
    inline int find_lower_pos(const key_type x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::lower_bound(this->key, this->key + this->size, x, this->key) - this->key;
        //return aex::linear_search_lower_bound(this->key, this->key + this->size, x) - this->key;
        //if constexpr (std::is_same_v<key_type, ULL>)
        //    return std::lower_bound(this->key, this->key + this->size, x) - this->key;
        //else
        return aex::linear_search_lower_bound_avx512<traits::DATA_NODE_SLOT_SIZE, key_type>(this->key, (int)this->size, x);
        //return std::lower_bound(this->key, this->key + this->size, x) - this->key;
    }

    inline int find_upper_pos(const key_type x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::upper_bound(this->key, this->key + this->size, x, this->key) - this->key;
        //return std::upper_bound(this->key, this->key + this->size, x) - this->key;
        return aex::linear_search_upper_bound_avx512<traits::DATA_NODE_SLOT_SIZE, key_type>(this->key, (int)this->size, x);
    }
    key_type      key[traits::DATA_NODE_SLOT_SIZE];
    value_type    data[traits::DATA_NODE_SLOT_SIZE];
    data_node_ptr next;
};

}
