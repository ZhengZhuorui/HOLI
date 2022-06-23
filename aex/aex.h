
#pragma once
#include "aex/aex_traits.h"
#include "aex/aex_model.h"
#include "aex/aex_node.h"
#include "aex/aex_allocator.h"
#include "aex/aex_iterator.h"

#define AEX_DEBUG

#ifdef AEX_DEBUG

#define AEX_PRINT(x)  do { std::cout << x << std::endl; }while(0)

#define AEX_ASSERT(x) do { assert(x); } while(0)

#else

#define AEX_PRINT(x) do { } while(0)

#define AEX_ASSERT(x) do { } while(0)

#endif

namespace aex{

template<typename _Key, typename _Val,
        typename _aex_traits=aex_traits<_Key, _Val> >
class aex{
public:

    typedef _aex_traits traits;
    typedef typename traits::key_type key_type;
    typedef typename traits::value_type value_type;
    typedef typename traits::used_as_set used_as_set_type;
    typedef typename traits::AllowMultiKey AllowMultiKey;
    typedef typename traits::Model Model;

    typedef size_t size_type;

    typedef aex self;
    typedef aex_base_iterator<_Key, _Val> iterator;
    typedef aex_base_const_iterator<_Key, _Val> const_iterator;
    typedef aex_base_reverse_iterator<_Key, _Val> reverse_iterator;
    typedef aex_base_const_reverse_iterator<_Key, _Val> const_reverse_iterator;
    
    typedef aex_inner_node<_Key, _Val> inner_node;
    typedef inner_node* inner_node_ptr;
    typedef aex_data_node<_Key, _Val> data_node;
    typedef data_node* data_node_ptr;

    typedef aex_node_base<_Key, _Val> node;
    typedef node* node_ptr;
    
    typedef aex_allocator allocator;
    typedef aex_node_allocator node_allocator;
    typedef aex_bitmap_impl bitmap_impl;
    typedef typename inner_node::bitmap bitmap;

    
    #define AEX_CONSTRUCT do { \
        root = _node_allocator::allocate_data_node(); \
        head_leaf = root; \
        tail_leaf = root; \
        \
        _max_ml_inner_slot_size[0] = _max_ml_inner_slot_size[1] = traits::MIN_ML_slot_size;\
        for (i = 2; i < 7; ++i)\
        if (_max_ml_inner_slot_size[i] < 0x3ffffff) \
            _max_ml_inner_slot_size[i] = _max_ml_inner_slot_size[i - 1] * _max_ml_inner_slot_size[i - 1]\
        else _max_ml_inner_slot_size[i] =  _max_ml_inner_slot_size[i - 1];\
    } while(1); \
    level = 1; \

    aex() : _m_allocator(){
        AEX_CONSTRUCT
    }

    template<typename _InputIterator>
    aex(_InputIterator __first, _InputIterator __last){
        AEX_CONSTRUCT
        /* TODO */
    }

    aex(const self& _index){
        construct(_index.root, root);
        _size = _index.size;
        head_leaf = find_head(root);
        tail_leaf = find_tail(root);
        _size = _index._size;
        level = _index.level;
        memcpy(_max_ml_inner_slot_size, &_index._max_ml_inner_slot_size, traits::MAX_LEVEL * sizeof(size_type));
    }
    aex(self&& _index){
        //AEX_CONSTRUCT
        this->root = _index.root;
        _index.root = nullptr;
        this->head_leaf = _index.head_leaf;
        _index.head_leaf = nullptr;
        this->tail_leaf = _index.tail_leaf;
        _size = _index->_size;
        level = _level;
        memcpy(_max_ml_inner_slot_size, &_index._max_ml_inner_slot_size, traits::MAX_LEVEL * sizeof(size_type));
    }

    ~aex(){
        
    }

    iterator insert(const std::pair<_Key, _Val> const &x){
        node *_stack[traits::MAX_LEVEL], *now_node = root, *new_node, *old_node;
        bool new_child = false;
        size_type _stack_top = 0, num_buf = 0, new_num_buf;
        key_type *key_buf = allocator::allocate<key_type>(4), *new_key_buf = allocator::allocate<key_type>(4);
        node **child_buf = allocator::allocate<node_ptr>(4), **new_child_buf = allocator::allocate<node_ptr>(4);
        Model _m;
        int level;

        /* find the path*/
        while (!(now_node->prop & LEAF)){
            _stack[_stack_top++] = now_node;
            now_node = find_lower(now_node, x.first);
        }

        /* if data node is full, split the node */
        if (isfull(now_node)){
            old_node = now_node;
            new_node = node_allocator::allocate_data_node();
            split(now_node, new_node);
            key_type new_key = static_cast<data_node_ptr>(new_node)->key[new_node->size - 1];
            key_type old_key = static_cast<data_node_ptr>(now_node)->key[now_node->size - 1];
            if (x.first < old_key)
                _insert(old_node, x);
            else 
                _insert(new_node, x);
            new_child = true;
            key_buf[0] = x;
            child_buf[0] = old_node;
            num_buf = 1;
        }
        else
            _insert(now_node, x);

        level = 1;

        // recursive insert in inner node
        while (new_child){
            now_node = nullptr;
            new_child = false;

            if (_stack_top >= 0)
                now_node = _stack[stack_top--];
            
            /* if now node is nullptr, create a new root node */
            if (now_node == nullptr){
                now_node = node_allocator.allocate_inner_node(num_buf, level);
                static_cast<inner_node_ptr>(now_node)->construct(key_buf, child_buf, num_buf);
                _insert(static_cast<inner_node_ptr>now_node, old_node);
                _insert(static_cast<inner_node_ptr>now_node, new_node);
                root = now_node;
            }
            else{
                /* the node is full after insert*/
                if (isfull(static_cast<inner_node_ptr>(now_node), num_buf)){
                    if (check_expand(static_cast<inner_node_ptr>(now_node)) && 
                        check_rewired(static_cast<inner_node_ptr>(now_node), key_buf, _m)){
                        rewired(static_cast<inner_node_ptr>(now_node), _m);
                        for (size_type i = 0; i < num_buf; ++i)
                            _insert(static_cast<inner_node_ptr>(now_node), std::pair(key_buf[i], child_buf[i]));
                    }
                    else {
                        new_child = true;
                        insert_split(static_cast<inner_node_ptr>(now_node), key_buf, child_buf, num_buf, new_key_buf, new_child_buf, new_num_buf);
                        node_allocator::free(now_node);
                    }
                }
                else{
                    for (size_type i = 0; i < num_buf; ++i){
                        if (check_insert(now_node, key_buf[i])){
                            _insert(now_node, std::pair<key_type, node_ptr>(key_buf[i], child_buf[i]));
                        }
                        else{
                            new_child = true;
                            insert_split(now_node, key_buf + i, child_buf + i, num_buf - i, new_key_buf, new_child_buf, num_buf);
                            node_allocator::free(now_node);
                        }
                    }
                }
            }
            /* swap buffer */
            std::swap(key_buf, new_key_buf);
            std::swap(child_buf, new_child_buf);
            std::swap(num_buf, new_num_buf);
            ++level;
        }
        _stack.clear();
        ++_size;

        allocator::free(key_buf);
        allocator::free(child_buf);
        allocator::free(new_key_buf);
        allocator::free(new_child_buf);
    }

    iterator find(key_type &x){
        iterator it = find_lower(x);
        if (it->key() != x) return end();
        return it;
    }

    bool exists(key_type &x){
        iterator it = find_lower(x);
        if (it->key() != x) return false;
        return it;
    }

    iterator lower_bound(key_type &x){
        return find_lower(x);
    }

    iterator upper_bound(key_type &x){
        return find_upper(x);
    }

    void bulk_load(const std::pair<key, value>* const data, const int nums){
        
        size_type n = 0;
        /* 
        * TODO: use buffer instead of vector
        */
        key_type *key_buf, *new_key_buf;
        data_node_ptr data_node;
        inner_node_ptr inner_node;
        size_type size, slot_size, cnt = 0;
        int level = 1, bit = 3;
        key_buf = allocator::allocate(nums / 8);
        for (size_type i = 0, cnt = 0; i < nums; i += 8){
            size_type max_j = max(i + 8, nums);
            data_node = node_allocator::allocate_data_node();
            for (size_type j = i; j < max_j; ++j){
                data_node->key[j & traits::DATA_NODE_SLOT_SIZE_BIT] = data[j].first;
                data_node->data[j & traits::DATA_NODE_SLOT_SIZE_BIT] = data[j].second;
                ++data_node->size;
            }
            key_buf[cnt++] = data_node->key[data_node->size - 1];
        }
        while (cnt > 1){
            slot_size = max_ml_inner_slot_size();

            ++level;
        }
    }

    void bulk_load(const std::pair<key_type, value_type>* data, const size_type n){
        bulk_load(data, data + n);
    }
    

    /* erase one key*/
    bool erase(const key_type const &x){
        node *_stack[traits::MAX_LEVEL], *left[traits::MAX_LEVEL], *right[traits::MAX_LEVEL];
        int top;
        if (!_erase_find(x, left, _stack, right, top)) return false;
        _erase(left, _stack, right, top);
    }
    bool erase(const iterator const iter){
        node *_stack[traits::MAX_LEVEL], *left[traits::MAX_LEVEL], *right[traits::MAX_LEVEL];
        int top;
        if (!_erase_find(iter->key(), left, _stack, right, top)) return false;
        _erase(left, _stack, right, top);
    }

    inline iterator begin(){
        return iterator(head_leaf, 0);
    }

    inline const_iterator begin() const {
        return iterator(head_leaf, 0);
    }

    inline iterator end() {
        return iterator(tail_leaf, DATA_slot_size + 1);
    }

    inline const_iterator end() const {
        return iterator(tail_leaf, DATA_slot_size + 1);
    }

    inline reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    inline const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    inline reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    inline size_type size() {
        return _size;
    }

    inline bool empty() const {
        return _size == 0;
    }

protected:

private:
    node_ptr root;

    node_ptr head_leaf;

    node_ptr tail_leaf;

    size_type _size, level;

    size_type  _max_ml_inner_slot_size[7];
    
    traits::used_as_set used_as_set;
    
    static void construct(node_ptr node, node_ptr &new_node){
        if (node->prop&LEAF){
            new_node = node_allocator::allocate_data_node();
            static_cast<data_node>(new_node)->copy(node);
        }
        else{
            new_node = node_allocator::allocate_inner_node(node->slot_size, node->level);
            static_cast<inner_node_ptr>(new_node)->copy(node);
            bitmap bm = static_cast<inner_node_ptr>(new_node)->bitmap_ptr();
            node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr();
            node_ptr* new_child = static_cast<inner_node_ptr>(new_node)->child_ptr();
            if (node->prop & ML_NODE){
                size_type prev = 0;
                for (size_type i = 0; i < node->size; ++i)
                if (bitmap_impl::at(bm, i)){
                    construct(child[i], new_child[i]);
                    memcpy(prev, new_child[i], (i - prev) * sizeof(node_ptr));
                    prev = i;
                }
            }
            else{
                for (size_type i = 0; i < node->size; ++i){
                    construct(child[i], new_child[i]);
                }
            }
        }
    }

    static void deconstruct(node_ptr node){
        if (node->prop & LEAF){
            node_allocator::deallocate(static_cast<data_node_ptr>(node));
        }
        else{
            node_ptr* child = static_cast<inner_node_ptr>(node)->child_ptr();
            if (node->prop & ML_NODE){
                bitmap bm = static_cast<inner_node_ptr>(node)->bitmap_ptr();
                for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->slot_size; ++i)
                if (bitmap_impl(bm, i)){
                    deconstruct(child[i]);
                }
            }
            else{
                for (size_type i = 0; i < static_cast<inner_node_ptr>(node)->size; ++i){
                    deconstruct(child[i]);
                }
            }
            node_allocator::deallocate(static_cast<inner_node_ptr>(node))
        }
    }

    static node_ptr find_head(node_ptr node){
        if (node->prop & LEAF) return node;
        return find_head(static_cast<inner_node_ptr>(node)->first());
    }

    static node_ptr find_tail(node_ptr node){
        if (node->prop & LEAF) return node;
        return find_tail(static_cast<inner_node_ptr>(node)->last());
    }

    size_type find_lower_pos(const inner_node_ptr const node, const key_type const &x){
        key_type* key = node->key_ptr();
        if (node->prop & ML_NODE){
            size_type pos = node->model->predict(x.first);
            for (size_type i = pos; i < pos + traits::ERROR_BOUND; ++i)
            if (key[i] >= x)
                return i;
            return node->slot_size;
        }
        else{
            size_type L = 0, R = node->size - 1, ret = node->slot_size, mid;
            while (L <= R){
                mid = (L + R) >> 1;
                if (key[(L + R) >> 1] >= x) {ret = mid; L = mid + 1};
                else {R = mid - 1;}
            }
            return ret;
        }
    }

    size_type find_upper_pos(const inner_node_ptr const node, const key_type const &x){
        key_type* key = node->key_ptr();
        if (node->prop & ML_NODE){
            size_type pos = node->model->predict(x.first);
            for (size_type i = pos; i < pos + traits::ERROR_BOUND; ++i)
            if (key[i] > x)
                return i;
            return node->slot_size;
        }
        else{
            size_type L = 0, R = node->size - 1, ret = node->slot_size, mid;
            while (L <= R){
                mid = (L + R) >> 1;
                if (key[(L + R) >> 1] > x) {ret = mid; L = mid + 1};
                else {R = mid - 1;}
            }
            return ret;
        }
    }

    inline node_ptr find_lower(const inner_node_ptr const node, const key_type const &x){
        node_ptr* child = node->child_ptr();
        size_type pos = find_lower_pos(node, x);
        return (pos == node->slot_size) ? nullptr : child[pos];
    }

    iterator find_lower(data_node_ptr p, key_type &key){
        for (size_type i = 0; i < p->size; ++i)
        if (p->key[i] >= key)
            return iterator(p, i);
        return end();
    }

    iterator find_lower(const key_type const &key){
        node_ptr node = root;
        while (!(node & LEAF)){
            node = find_lower(static_cast<inner_node_ptr>(node), key);
        }
        return find_lower(static_cast<data_node_ptr>(node), key);
    }

    node_ptr find_upper(inner_node_ptr p, key_type &x){
        node_ptr* child = node->child_ptr();
        size_type pos = find_upper_pos(node, x);
        return (pos == node->slot_size) ? nullptr : child[pos];
    }

    iterator find_upper(data_node_ptr p, key_type &key){
        for (size_type i = 0; i < p->size; ++i)
        if (p->key[i] > key)
            return iterator(p, i);
        return end();
    }

    iterator find_upper(const key_type const &key){
        node_ptr node = root;
        while (!(node & LEAF)){
            node = find_upper(static_cast<inner_node_ptr>(node), key);
        }
        return find_upper(static_cast<data_node_ptr>(node), key);
    }


    void split(data_node_ptr old_node, data_node_ptr new_node){
        new_node->next = old_node;
        if (old_node->prev != nullptr) new_node->prev->next = new_node;
        old_node->prev = new_node->prev;
        if (tail_leaf == old_node) tail_leaf = new_node;

        size_type mid = traits::DATA_NODE_SLOT_SIZE >> 1;
        memcpy(new_node->key, old_node->key + mid, (traits::DATA_NODE_SLOT_SIZE - mid) * sizeof(key_type));
        memcpy(new_node->data, old_node->data + mid, (traits::DATA_NODE_SLOT_SIZE - mid) * sizeof(value_type));
        old_node->size = mid;
        new_node->size = traits::DATA_NODE_SLOT_SIZE - mid;
    }



    void insert_split(const inner_node_ptr const node, const key_type* const key, const node_ptr* const child, 
               const int n,
               key_type *new_key, node_ptr* new_child, int &new_n){
        key_type* key_buf = allocator::allocate<key_type*>((node->size + n));
        key_type* node_key = node->key_ptr();
        node_ptr* child_buf = allocator::allocate<node_ptr*>((node->size + n));
        node_ptr* node_child = node->child_ptr();
        bitmap bm = node->bitmap_ptr();
        Model model;
        inner_node_ptr new_node;

        size_type j = 0, start, n_slot = 0, end=node->size + n, slot_size = old->slot_size + n, slot_size;
        size_type max_slot_size;
        bool flag;

        /* merge key_buffer and node */
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                while (j < n && key[j] < node_key[i]){
                    key_buf[n_slot] = key[j];
                    child_buf[n_slot] = child[k];
                    n_slot++;j++;
                }
                key_buf[n_slot] = node_key[i];
                child_buf[n_slot] = node_child[i];
                n_slot++;
            }
        }
        else{
            for (size_type i = 0; i < node->slot_size; ++i){
                while (j < n && key[j] < node_key[i]){
                    key_buf[n_slot] = key[j];
                    child_buf[n_slot] = child[k];
                    n_slot++;j++;
                }
                key_buf[n_slot] = node_key[i];
                child_buf[n_slot] = node_child[i];
                n_slot++;
            }
        }

        if (j < n){
            memcpy(key_buf + start, key + j, (n - j) * sizeof(key_type));
            memcpy(child_buf + start, child + j, (n - j) * sizeof(node_ptr));
        }

        /* split */
        new_n = 0;
        while (start < end){
            max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
            while (max_slot_size < (end - start)) max_slot_size <<= 1;
            max_slot_size = std::min(max_slot_size, max_inner_node_slot_size(node->level));
            for (slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1)
            if (check_rewired(key + start, slot_size, model)){
                new_node = node_allocator::allocate_inner_node(slot_size, node->level);
                new_node->_m = model;

                for (size_type i = start; i < start + slot_size; ++i)
                    _insert(new_node, std::pair(key_buf[i], child[i]));

                new_key[new_n] = key[start + slot_size - 1];
                new_child[new_n] = new_node;
                new_n++;
                start += slot_size;
            }
        }
        allocator::free(key_buf);
        allocator::free(child_buf);
    }

    void erase_split(const inner_node_ptr const node, key_type* new_key_buf, node_ptr* new_child_buf, int &num_bug){
        key_type* key_buf = allocator::allocate<key_type>(node->size);
        key_type* key = node->key_ptr();
        node_ptr* child_buf = allocator::allocate<node_ptr>(node->size);
        node_ptr* child = node->child_ptr();
        size_type start, max_slot_size;
        inner_node_ptr new_node;
        Model model;
        
        copy_to_buffer(node, key_buf, child_buf);
        while (start < node->size){
            max_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
            while (max_slot * traits::INNER_NODE_FULL_RATIO < (end - start)) max_slot_size <<= 1;
            for (size_type slot_size = max_slot_size; slot_size >= traits::MIN_INNER_NODE_SLOT_SIZE; slot_size >>= 1)
            if (check_rewired(key + start, slot_size, model){
                new_node = node::allocator::allocate_inner_node(slot_size, node->level);
                new_node->_m = model;
                for (size_type i = start; i < start + slot_size; ++i)
                    _insert(new_node, std::pair(key[i], child[i]));

                new_key_buf[num_buf] = key[start];
                new_child_buf[num_buf] = new_node;
                num_buf++;
                start += slot_size;
            }
        }
        allocator::free(key);
        allocator::free(child);
    }

    bool update_key(inner_node_ptr parent, const key_type &key, const inner_node_ptr const node){
        node_ptr child = parent->child_ptr();
        node_ptr new_child;
        size_type pos = parent->at(node), new_pos;
        bitmap bm = parent->bitmap_ptr();
        bool ret = false;
        if (parent->prop & ML_NODE){
            if (parent->last() == pos) ret = true;
            new_pos = node->model->predict(key);
            for (size_type i = new_pos; i < new_pos + traits::ERROR_BOUND; ++i)
            if (bitmap_impl::at(bm, i) == false || pos == new_pos){
                new_pos = i;
                break;
            }
            if (pos < new_pos){
                for (size_type i = pos; i < new_pos; ++i)
                    child[i] = child[new_pos];
            }
            else{
                if (pos < slot_size - 1)
                for (size_type i = new_pos; i < pos; ++i)
                    child[i] = child[pos + 1];
            }
            return ret;
        }
        else{
            parent->key_ptr()[pos] = key;
            if (pos == parent->size - 1) return true;
            return false;
        }
    }

    inline void _insert(data_node_ptr node, key_type& key){
        u_int8_t pos=node->size;
        for (size_type i = 0; i < traits::DATA_NODE_SLOT_SIZE; ++i)
        if (key < node->key[i]){
            pos = i;
            break;
        }
        memmove(node->key + pos, node->key + pos + 1, (node->size - pos) * sizeof(key_type));
    }

    void find_lower(const key_type& const key){
        node_ptr now = root;
        while (!(now.prop & LEAF)){
            if ((now.prop & MLNODE)) now = this->find_lower(static_cast<inner_node_ptr>(now), x);
            else now = this->find_lower(static_cast<data_node_ptr>(now), x);
            if (now == nullptr) return end();
        }
        return this->find_lower(static_cast<data_node_ptr>(now), x);
    }

    void _insert(data_node_ptr node, const std::pair<key_type, data_type> &x){
        key_type* key_ptr = node->key;
        size_type pos = node->size - 1;

        for (size_type i = 0; i < node->size - 1; ++i)
        if (x.first <= key_ptr[i]){
            pos = i;
            break;
        }
        memmove(key_ptr + pos + 1, key_ptr + pos, node->size - pos);
        data_memmove(node->data + pos + 1, node->data + pos, node->size - pos);
        key_ptr[pos] = x.first;
        node->data[pos] = x.second;
    }

    void _insert(inner_node_ptr node, const std::pair<key_type, node_ptr> &x){
        size_type pos = node->model->predict(x.first);
        size_type empty_slot;
        key_type* key = node->key_ptr();
        node_ptr* child = node->child_ptr();
        for (size_type i = pos; i < pos + traits::ERROR_BOUND; ++i)
        if (key[i] < x.first){
            pos = i;
            break;
        }
        empty_slot = node->bm->next_empty_slot(pos);
        memmove(key + pos + 1, key + pos, empty_slot - pos);
        memmove(child + pos + 1, child + pos, empty_slot - pos);
        bitmap_impl::set_one(node->bitmap_ptr(), empty_slot);
    }
    
    bool check_rewired(inner_node_ptr node, Model &_m){
        if (!(node->prop & ML_NODE)) return true;
        bitmap bm = node->bitmap_ptr();
        key_type* key = node->key_ptr();
        key_type* key_buffer = allocator::allocate<key_type>(node->size);
        size_type start, pos;
        copy_to_buffer(node, key_buffer);
        _m.train(key_buffer, node->size, node->real_slot_size());
        for (size_type i = 0, start = 0; i < node->size; ++i){
            pos = node->model->predict(key_buffer[i]);
            start = std::max(start + 1, pos);
            if (start - pos >= traits::ERROR_BOUND || start > node->slot_size) return false;
        }
        return true;
    }

    bool check_rewired(const key_type* const key, const int size, const int slot_size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SLOT_SIZE)
            return true;
        size_type start, pos;
        Model _m;
        _m.train(key, size, slot_size);
        for (size_type i = 0, start=0; i < size; ++i){
            pos = _m->predict(key[i]);
            start = std::max(start + 1, pos);
            if (start - pos >= traits::ERROR_BOUND) return false;
        }
        return true;
    }


    void rewired(inner_node_ptr node){
        if (!(node->prop & ML_NODE)) return;
        key_type* key = node->key_ptr();
        node_ptr* child = node->child_ptr();
        key_type* new_key = allocator::allocate<key_type>(node->size);
        node_ptr* new_child = allocator::allocate<node_ptr>(node->size);
        size_type start, pos;

        copy_to_buffer(node, new_key, new_child);
        node->model.train(new_key, node->size, node->slot_size);
        
        for (size_type i = 0, start = 0; i < ; ++i){
            pos = node->model->predict(k[i]);
            start = std::max(start + 1, pos);
            key[start] = new_key[i];
            child[start] = new_child[i];
        }
        memcpy(key, new_key, node->slot_size * sizeof(key_type));
        memcpy(child, new_child, node->slot_size * sizeof(value_type));

        allocator::free(new_key);
        allocator::free(new_child);
    }

    void rewired(inner_node_ptr node, inner_node_ptr new_node){
        if (!(node->prop & ML_NODE) && !(new_node->prop & ML_NODE)){
            memcpy(new_node->key_ptr(), node->key_ptr(), node->size * sizeof(key_type));
            memcpy(new_node->child_ptr(), node->child_ptr(), node->size * sizeof(node_ptr));
            return;
        }
        else if ((node->prop & ML_NODE) && (new_node->prop & ML_NODE)){
            key_type* key = node->key_ptr();
            node_ptr* child = node->child_ptr();
            key_type* new_key = new_node->key_ptr();
            node_ptr* new_child = new_node->child_ptr();
            key_type* key_buffer = allocator::allocate<key_type>(node->slot_size);
            bitmap bm = new_node->bitmap_ptr();
            copy_to_buffer(node, key_buffer);
            size_type start, pos;
            Model::train(key_buffer, node->size, new_node->slot_size, new_node->model);
            for (size_type i = 0, start = 0; i < ; ++i){
                pos = new_node->model->predict(key[i]);
                start = std::max(start + 1, pos);
                new_key[start] = key[i];
                new_child[start] = child[i];
                bitmap_impl::set_one(bm, start);
                ++start;
            }
            allocator::free(key_buffer);
        }
        else if ((node->prop & ML_NODE) && !(node->prop & ML_NODE)){
            size_type start = 0;
            bitmap bm = node->bitmap_ptr();
            key_type* key = node->key_ptr();
            node_ptr* child = node->child_ptr();
            key_type* new_key = new_node->key_ptr();
            node_ptr* new_child = new_node->child_ptr();
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                new_key[start] = key[i];
                new_child[start] = child[i];
                ++start;
            }
            return new_node;
        }
        else if (!(node->prop & ML_NODE) && (node->prop & ML_NODE)){
            key_type* k = node->key_ptr();
            node_ptr* p = node->child_ptr();
            key_type* new_k = new_node->key_ptr();
            node_ptr* new_p = new_node->child_ptr();
            size_type start, pos;
            Model::train(k, node->size, new_node->slot_size, new_node->model);
            node->_bm->clear();
            for (size_type i = 0, start = 0; i < ; ++i){
                pos = new_node->model->predict(k[i]);
                start = std::max(start + 1, pos);
                new_k[start] = k[i];
                new_p[start] = p[i];
            }
            memcpy(k, new_k, node->slot_size * sizeof(value_type));
            memcpy(p, new_p, node->slot_size * sizeof(value_type));
        }
    }

    void rewired(inner_node_ptr node, const Model const &m){
        if (!(node->prop & ML_NODE)){
            return;
        }
        key_type* k = node->key_ptr();
        node_ptr* p = node->child_ptr();
        key_type* new_k = allocator::allocate<key_type*>(node->slot_size);
        node_ptr* new_p = allocator::allocate<node_ptr*>(node->slot_size);
        size_type start, pos;
        new_k = ptr.first;
        new_p = ptr.second;
        node->_m = m;
        node->_bm->clear();
        for (size_type i = 0, start = 0; i < ; ++i){
            pos = m->predict(k[i]);
            start = std::max(start + 1, pos);
            new_k[start] = k[i];
            new_p[start] = p[i];
        }
        memcpy(k, new_k, node->slot_size * sizeof(value_type));
        memcpy(node->value, new_p, node->slot_size * sizeof(value_type));
        allocator::free(new_k);
        allocator::free(new_p);
    }

    bool check_rewired(inner_node_ptr node){
        if (!(node->prop & ML_NODE)) return true;
        key_type* k = node->key;
        size_type start, pos;
        Model* m = node->model;
        Model::train(node->key, node->size, node->slot_size, *m);
        for (size_type i = 0, start = 0; i < node->slot_size; ++i)
        if (node->_bm->at(i) ){
            pos = m->predict(k[i]);
            start = std::max(start + 1, pos);
            if (start - pos >= traits::ERROR_BOUND) return false;
        }
        return true;
    }

    void merge_leaf(data_node_ptr left_node, data_node_ptr right_node, inner_node_ptr parent){
        memmove(right_node->key + left_node->size, right_node->key, left_node->size * sizeof(key_type));
        data_memmove(right_node->data + left_node->size, right_node->key, right_node->size * sizeof(value_type));
        memmove(right_node->key, left_node->key, left_node->size * sizeof(key_type));
        data_memmove(right_node->data, left_node->data, right_node->size * sizeof(value_type));
        if (parent != nullptr){
            size_type pos = parent->at(left_node);
            size_type prev_pos = parent->at(pos);
            node_ptr child = parent->child_ptr();
            for (size_type i = prev_pos + 1; i < pos; ++i)
                child[i] = child[pos];
        }
    }

    void expand(inner_node_ptr &node){
        size_type new_slot_size = node->real_slot_size() * traits::EXPAND_RATIO;
        inner_node_ptr new_node = node_allocator.allocate(new_slot_size, );
        rewired(node, new_node);
        node = new_node;
    }

    inline bool check_expand(const inner_node_ptr const node){
        return node->slot_size * traits::MIN_INNER_NODE_SLOT_SIZE <= max_inner_node_slot_size(node->level);
    }

    inline void narrow(inner_node_ptr &node){
        size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
        inner_node_ptr new_node = node_allocator::allocate_inner_node(new_slot_size, node->level);
        rewired(node, new_node);
    }

    bool check_narrow(inner_node_ptr &node){
        size_type new_slot_size = node->slot_size * traits::NARROW_RATIO;
        key_type* key_buf = (key_type*)allocater::allocate((node->size)), node_k = node->key_ptr();
        bool flag;

        bitmap bm = node->bitmap_ptr();
        for (size_type i = 0, j = 0; i < node->slot_size; ++i)
        if (bitmap_impl::at(bm, i)) key_buf[j++] = node_l[i];
        flag = check_rewired(key_buf, node->size, new_slot_size);
        allocater::free(key_buf);
        return flag;
    }


    void shift_to_left_leaf(data_node_ptr left_node, data_node_ptr right_node){
        left_node->key[left_node->size] = right_node->key[right_node->size - 1];
        left_node->data[left_node->size] = right_node->data[right_node->size - 1];
        ++left_node->size;

        memmove(right_node->key, right_node->key + 1, (right_node->size - 1) * sizeof(key_type));
        data_memmove(right_node->data, right_node->data + 1, (right_node->size - 1) * sizeof(valute_type));
        --right_node->size;        
    }

    void shift_to_right_leaf(data_node_ptr left_node, data_node_ptr right_node){
        memmove(right_node->key + 1, right_node->key, (right_node->size + 1) * sizeof(key_type));
        data_memmove(right_node->data, right_node->data + 1);
        right_node->key[0] = left_node->key[left_node->size - 1];
        right_node->data[0] = left_node->data[keft_node->size - 1];
        ++right_node->size;
        --left_node->size;
    }
    

    template<typename _Tp>
    inline void data_memmove(_Tp* dest, const _Tp* const src, const size_type n, std::__true_type f){}

    template<typename _Tp>
    inline void data_memmove(_Tp* dest, const _Tp* const src, const size_type n, std::__false_type f){
        memmove(dest, src, n * sizeof(_Tp));
    }

    template<typename _Tp>
    inline void data_memmove(_Tp* dest, const _Tp* const src, const size_type n){
        data_memmove(_Tp* dest, _Tp* src, size_type n, traits::used_as_set);
    }

    inline size_type max_inner_node_slot_size(size_type level){
        return (level < 7)?_max_ml_inner_slot_size[level]:_max_ml_inner_slot_size[6];
    }

    inline bool isfull(const data_node_ptr const node){
        return node->size >= traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FULL_RATIO;
    }

    inline bool isfew(const data_node_ptr const node){
        return node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO;
    }

    inline bool isfull(const inner_node_ptr const node){
        return (node->prop & ML_NODE) ? (node->size >= node->slot_size * traits::INNER_NODE_FULL_RATIO) : (node->size >= node->slot * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node_ptr const node){
        return (node->prop & ML_NODE) ? (node->size < node->slot_size * traits::INNER_NODE_FEW_RATIO) : (node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO);
    }

    inline bool isfull(const inner_node_ptr const node, const size_type bias){
        return (node->prop & ML_NODE) ? (node->size + bias >= node->slot_size * traits::INNER_NODE_FULL_RATIO) : (node->size >= node->slot * traits::DATA_NODE_FULL_RATIO);
    }
    
    inline bool isfew(const inner_node_ptr const node, const size_type bias){
        return (node->prop & ML_NODE) ? (node->size + bias < node->slot_size * traits::INNER_NODE_FEW_RATIO) : (node->size < traits::DATA_NODE_SLOT_SIZE * traits::DATA_NODE_FEW_RATIO);
    }

    void copy_to_buffer(const inner_node_ptr const node, const key_type* key_buf, const node_ptr* const child_buf){
        key_type* key = node->key_ptr();
        node_ptr* child = node->child_ptr();
        bitmap bm = node->bitmap_ptr();
        size_type n_slot = 0;
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                key_buf[n_slot] = key[i];
                child_buf[n_slot] = child[i];
                n_slot++;
            }
        }
        else{
            memcpy(key_buf, key, node->size * sizeof(key_type));
            memcpy(child_buf, child, node->size * sizeof(node_ptr));
        }
    }

    void copy_to_buffer(const inner_node_ptr const node, const key_type* key_buf){
        key_type* key = node->key_ptr();
        bitmap bm = node->bitmap_ptr();
        size_type n_slot = 0;
        if (node->prop & ML_NODE){
            for (size_type i = 0; i < node->slot_size; ++i)
            if (bitmap_impl::at(bm, i)){
                key_buf[n_slot] = key[i];
                n_slot++;
            }
        }
        else{
            memcpy(key_buf, key, node->size * sizeof(key_type));
        }
    }

    bool _erase_find(const key_type* const key, node_ptr* left, node_ptr* _stack, node_ptr* right, int &top){
        /* recursive find data */
        node_ptr node = root;
        size_type pos;
        while (!(node->prop & LEAF)){
            _stack[top] = node;
            top++;
            pos = find_lower_pos(static_cast<inner_node_ptr>(node), x);
            left[top] = static_cast<inner_node_ptr>(node)->prev(pos) == -1;
            if (left[top] == nullptr){
                if (left[top - 1] != nullptr)
                    left[top] = static_cast<inner_node_ptr>(left[top-1])->last();
                else left[top] = nullptr;
            }
            right[top] = static_cast<inner_node_ptr>(node)->next(pos);
            if (right[top] == nullptr){
                if (right[top - 1] != nullptr)
                    right[top] = static_cast<inner_node_ptr>(right[top - 1])->first();
                else right[top] = nullptr;
            }
            node = node->child_ptr()[pos];
        }
        if (find_lower(static_cast<data_node_ptr>(node, x)) == end()) 
            return false;
        _stack[top] = node;
        left[top] = node->prev;
        right[top] = node->next;
        top++;
        return true;
    }

    bool _erase(const node* const _stack, const node_ptr* const left, const node_ptr* const _stack, 
                const node_ptr* const right, const int top){
        node *node;
        inner_node_ptr parent;
        inner_node_ptr update_node;
        key_type* key_buf;
        size_type pos;
        node_ptr *child_buf;
        iterator iter;
        char update_key_flag = 0, new_update_key_flag = 0;
        bool merge_flag=false;

        if (iter == end()) return false;
        _erase(node, x);

        /* if data node is few, shift the data first, otherwise merge the near leaf */
        if (isfew(static_cast<data_node_ptr>(node), x)){
            if (node->prev != nullptr && !isfew(static_cast<data_node_ptr>(node->prev))){
                update_key_flag = 1;
                last_key = node->prev->data[node->prev->size - 1];
                shift_left_leaf(node, node->prev, _stack[top-1]);
            }
            else if (node->next != nullptr && !isfew(static_cast<data_node_ptr>(node->next))){
                update_key_flag = 2;
                last_key = node->data[node->size - 1];
                shift_right_leaf(node, node->next, _stack[top-1]);
            }
            else{
                /* merge the left leaf to right leaf */
                merge_flag = true;
                if (node->prev != nullptr){
                    update_key_flag = 1;
                    merge_leaf(node->prev, node, parent);
                    node_allocator::free(node->prev);
                }
                else if (node->next != nullptr){
                    update_key_flag = 2;
                    merge_leaf(node, node->next, parent);
                    node_allocator::free(node);
                }
            }
        }

        /* if the key is update */
        while (top > 1 && !update_key_flag){
            --top;
            node = _stack[top];
            parent = _stack[top - 1];

            /* if the left node update */
            if (update_key_flag == 1){
                if (parent->at(left[top]) != -1) {new_update_key_flag = 2; update_node = parent};
                    else {new_update_key_flag = 1; update_node = left[top - 1]};
            }
            /* if the now node update */
            else if (update_key_flag == 2){
                new_update_key_flag = 2;
                update_node = parent;
            }

            /* if no merge, means the node max key update */
            if (merge_flag == false){
                if (update_key_flag == 1){
                    update_key_flag = update_key(update_node, left[top]) ? 0 : update_key_flag;
                }
                else if (update_key == 2){
                    update_key_flag = update_key(update_node, _stack[top]) ? 0 : update_key_flag;
                }
            }
            /* otherwise erase the node */
            else{
                merge_flag = false;
                if (update_key_flag == 1){
                    update_key_flag = _erase(update_node, left[top]) ? 0 : update_key_flag;
                }
                else {
                    update_key_flag = _erase(update_node, parent) ? 0 : update_key_flag;
                }
            }

            /* if the node is too few */
            if (is_few(update_node)){
                if (check_narrow(update_node)){
                    update_key_flag = narrow(update_node) ? 0 : update_key_flag;
                }
                else{
                    node_ptr new_node = node_allocator::allocate_inner_node(update_node->size, update_node->size, false);
                    rewired(update_node, new_node);
                    update_node = new_node;
                }
            }

            update_key_flag = new_update_key_flag;
        }
        if (merge_flag){
            /* if the root has one child, change the root*/
            if (root->size == 1 && !(root->prop & LEAF)){
                root = root->child[0];
                node_allocator::free(root);
            }
        }
        
    }

    bool _erase(inner_node_ptr parent, inner_node_ptr node){
        size_type pos = parent->at(node);
        key_type* key = parent->key_ptr();
        node_ptr* child = parent->child_ptr();
        node_allocator::free(node);
        --parent->size;
        if (parent & ML_NODE){
            size_type prev = parent->prev(pos);
            bitmap_impl::set_zero(parent->bitmap_ptr(), pos);
            if (pos != parent->slot_size + 1)
            for (size_type i = prev + 1; i <= pos; ++i){
                key[i] = key[pos + 1];
                child[i] = child[pos + 1];
            }
            else
            for (size_type i = prev + 1; i <= pos; ++i){
                child[i] = nullptr;
            }
            if (pos == parent->last()) return true;
            return false;
        }
        else{
            memmove(key + pos, key + pos + 1, (parent->size - pos - 1) * sizeof(key_type));
            memmove(child + pos, child + pos + 1, (parent->size - pos - 1) * sizeof(node_ptr));
            if (pos == parent->size - 1) return true;
            return false;
        }
    }


};

}

}