#pragma once
#include "aex/aex_utils.h"
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
    typedef typename base_tree::components components;
    typedef typename components::size_type size_type;
    typedef typename components::inner_node inner_node;
    typedef typename components::hash_node  hash_node;
    typedef typename components::dense_node dense_node;
    typedef typename components::data_node  data_node;
    typedef typename components::inner_node_ptr inner_node_ptr;
    typedef typename components::hash_node_ptr  hash_node_ptr;
    typedef typename components::dense_node_ptr dense_node_ptr;
    typedef typename components::data_node_ptr  data_node_ptr;
    typedef typename components::RWLock RWLock;
    typedef typename components::Lock   Lock;

    explicit aex_node_base(NodeType _type) :  size(0), type(_type), node_lock(){}
    aex_node_base(aex_node_base &other_node): size(other_node.size), type(other_node.type), node_lock(){}
    aex_node_base(aex_node_base &&other_node):size(other_node.size), type(other_node.type), node_lock(){}

    aex_node_base& operator = (aex_node_base &other_node) {
        this->size = other_node.size;
        this->type = other_node.type;
        return *this;
    }

    aex_node_base& operator = (aex_node_base &&other_node) {
        this->size = other_node.size;
        this->type = other_node.type;
        return *this;
    }


    // size: the number of child nodes(inner node); the number of data(data node)
    size_type      size;
    NodeType       type;
    mutable RWLock node_lock;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef aex_inner_node<_Key, _Val, traits> self;
    typedef _Key key_type;
    typedef _Val value_type;
    typedef typename traits::slot_type slot_type;

    aex_inner_node(slot_type _slot_size, NodeType _type) : slot_size(_slot_size){}
    ~aex_inner_node() = default;
    aex_inner_node(self &other) = delete;
    aex_inner_node& operator = (aex_inner_node &other) = delete;

    slot_type slot_size;

};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_node : public aex_inner_node<_Key, _Val, traits>{
public:
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_inner_node<_Key, _Val, traits>     inner_node;
    typedef aex_hash_node<_Key, _Val, traits>      self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename traits::slot_type             slot_type;
    typedef typename base_tree::components         components;
    typedef typename components::InnerNodeModel    Model;
    typedef typename traits::bitmap                bitmap;
    typedef typename traits::bitmap_base           bitmap_base;
    typedef typename components::bitmap_impl       bitmap_impl;
    typedef typename components::node_ptr          node_ptr;
    typedef typename components::size_type         size_type;
    typedef typename components::Lock              Lock;

    //aex_hash_node(slot_type slot_size):inner_node(slot_size, NodeType::HashNode), bitmap_ptr(nullptr){
    //    init();
    //}
//
    //~aex_hash_node(){
    //    clear();
    //}   
    aex_hash_node() = delete;
    ~aex_hash_node() = delete;
    aex_hash_node(aex_hash_node &other) = delete;
    aex_hash_node& operator = (aex_hash_node &other) = delete;

    void clear(){
        if (this->bitmap_ptr != nullptr){
            delete[] this->bitmap_ptr;
            this->bitmap_ptr = nullptr;
        }
    }

    void init(){
        AEX_ASSERT(this->bitmap_ptr == nullptr);
        this->size = 0;
        this->bitmap_ptr = new bitmap_base[this->slot_size / sizeof(bitmap_base) + 1]();
    }

    inline slot_type predict(const key_type &key) const {
        //return std::min(std::max(static_cast<slot_type>(0), model.predict(key)), this->slot_size - 1);
        return std::max(0LL, static_cast<slot_type>(std::min(model.predict(key), static_cast<long double>(this->slot_size - 1))));
    }

    inline slot_type is_occupied(slot_type x) const {
        return bitmap_impl::at(this->bitmap_ptr, x);
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
        if (x <= 0)
            return 0;
        //slot_type y = x & (~(traits::SLOT_PER_SHORTCUT - 1));
        bitmap_base base = (bitmap_ptr[x >> 6]) << (63 - (x & 63));
        x -= (base == 0) ? (x & 63) : __builtin_clzll(base);
        return x;
    }

    //inline slot_type next_item_find(slot_type x) const {
    //    if (x >= slot_size)
    //        return x;
    //    bitmap_base base = (bitmap_ptr[x >> 6]) >> (x & 63);
    //    x += (base == 0) ? (64 - (x & 63)) : __builtin_ctzll(base);
    //    return x & (~63);
    //}

    inline slot_type next_item(slot_type x) const {
        if (x >= this->slot_size)
            return x;
        bitmap text = bitmap_ptr + (x >> 6);
        bitmap_base base = (*text) >> (x & 63);
        x += (base == 0) ? (64 - (x & 63)) : __builtin_ctzll(base);
        while (base == 0 && x < this->slot_size){
            ++text;
            base = *text;
            x += __builtin_ctzll(base);
        }
        return x;
    }

    inline slot_type prev_item(slot_type x) const {
        if (x <= 0)
            return x;
        bitmap text = bitmap_ptr + (x >> 6);
        bitmap_base base = (*text) << (63 - (x & 63));
        x -= (base == 0) ? ((x & 63) + 1) : __builtin_clzll(base);
        while (base == 0 && x > 0){
            --text;
            base = *text;
            x -= __builtin_clzll(base);
        }
        return x;
    }

    inline void array_lock(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_unlock(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_lock_shared(const slot_type l_pos, const slot_type r_pos) const {}
    inline void array_unlock_shared(const slot_type l_pos, const slot_type r_pos) const {}
    inline bool try_array_upgrade_lock(const slot_type l_pos, const slot_type r_pos) const {return true;}
    inline void array_downgrade_lock(const slot_type l_pos, const slot_type r_pos) const {}
    inline slot_type array_lock_shared_until_next_item(const slot_type prev_pos, const slot_type pos) const {return next_item(pos);}
    inline slot_type try_array_lock_shared_until_prev_item(const slot_type pos, bool &restart) const {return prev_item(pos);}
    bitmap       bitmap_ptr;
    Model        model;
    mutable Lock meta_lock;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_dense_node : public aex_inner_node<_Key, _Val, traits>{
public:
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_inner_node<_Key, _Val, traits>     inner_node;
    typedef aex_dense_node<_Key, _Val, traits>     self;
    typedef typename traits::slot_type             slot_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components         components;
    typedef typename components::node_ptr          node_ptr;

    //aex_dense_node(slot_type slot_size):inner_node(slot_size, NodeType::DenseNode), try_learn(false){init();}
    //~aex_dense_node(){clear();}
    aex_dense_node() = delete;
    ~aex_dense_node() = delete;
    aex_dense_node(aex_dense_node &other) = delete;
    aex_dense_node& operator = (aex_dense_node &other) = delete;

    void clear(){
        if (key_ptr != nullptr){
            delete[] key_ptr;
            key_ptr = nullptr;
        }
        if (child_ptr != nullptr){
            delete[] child_ptr;
            child_ptr = nullptr;
        }
    }

    void init(){
        AEX_ASSERT(this->key_ptr == nullptr);
        AEX_ASSERT(this->child_ptr == nullptr);
        this->size = 0;
        key_ptr   = new key_type[this->slot_size]();
        child_ptr = new node_ptr[this->slot_size]();
    }

    key_type *key_ptr;
    node_ptr *child_ptr;
};


template<typename _Key,
        typename _Val,
        typename traits>
struct aex_static_data_node : public aex_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;
    typedef _Val value_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef aex_static_data_node<_Key, _Val, traits> data_node;
    typedef typename base_tree::components components;
    typedef typename components::base_node     base_node;
    typedef typename components::DataNodeModel Model;
    typedef typename components::version_type  version_type;
    typedef base_node* node_ptr;
    typedef data_node* data_node_ptr;
    typedef typename traits::slot_type slot_type;

    aex_static_data_node(version_type _version) : base_node(NodeType::LeafNode), next(nullptr), version(_version){}

    ~aex_static_data_node() {}

    aex_static_data_node(aex_static_data_node &other_node) :base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
    }

    aex_static_data_node(aex_static_data_node &&other_node) :base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
    }

    aex_static_data_node& operator = (aex_static_data_node &other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
        return *this;
    }

    aex_static_data_node& operator = (aex_static_data_node &&other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        this->next = other_node.next;
        return *this;
    }

    inline void construct(const key_type *_key, const value_type *_data, slot_type nums){
        std::copy(_key, _key + nums, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
    }

    inline void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        AEX_ASSERT(nums >= traits::MIN_DATA_NODE_SLOT_SIZE / 2);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_lower_pos(x);
        insert(x, data, pos);
        return pos;
    }

    // insert a item in position
    inline void insert(const key_type &x, const value_type &data, const slot_type pos){
        AEX_ASSERT(this->size < traits::MIN_DATA_NODE_SLOT_SIZE);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
    }

    inline bool erase(const key_type &x){
        slot_type pos = find_lower_pos(x);
        if (pos >= this->size || key[pos] != x)
            return false;
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->size--;
        return true;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::lower_bound(this->key, this->key + this->size, x, this->key) - this->key;
        //return aex::linear_search_lower_bound(this->key, this->key + this->size, x) - this->key;
        return std::lower_bound(this->key, this->key + this->size, x) - this->key;
        //return std::lower_bound(this->key, this->key + this->size, x) - this->key;
    }

    inline slot_type find_upper_pos(const key_type &x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::upper_bound(this->key, this->key + this->size, x, this->key) - this->key;
        return std::upper_bound(this->key, this->key + this->size, x) - this->key;
    }

    inline bool exists(const key_type &x) const {
        slot_type pos = find_lower_pos(x);
        if (pos < this->size && key[pos] == x)
            return true;
        return false;
    }

    key_type      key[traits::DATA_NODE_SLOT_SIZE];
    value_type    data[traits::DATA_NODE_SLOT_SIZE];
    data_node_ptr next;
    version_type  version;
};

}
