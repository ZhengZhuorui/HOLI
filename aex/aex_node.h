#pragma once

namespace aex{

enum NODE_INSERT_CODE{
    SUCCESS,
    LEFT_BUFFER_OVERFLOW,
    RIGHT_BUFFER_OVERFLOW,
    HOTSPOT_CONFLICT,
    INNER_NODE_CONFLICT,
    NONE
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_node_base{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;
    
    typedef aex_node_base<key_type, value_type, traits> self;

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef typename base_tree::components components;
    
    typedef typename components::inner_node inner_node;
    typedef typename components::data_node data_node;
    typedef typename components::node_balance_stats node_balance_stats;
    typedef typename components::node_split_stats node_split_stats;
    typedef typename components::NodeMutex NodeMutex;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;

    typedef self node;

    typedef self* node_ptr;

    node_ptr prev, next;

    // size: the child node of the node(inner node); the data of the node(data node)
    slot_type size; 

    unsigned int level, prop;

    NodeMutex node_mutex;

    aex_node_base():prev(nullptr), next(nullptr), size(0), level(0), prop(0){}

    aex_node_base(aex_node_base &other_node):prev(other_node.prev), next(other_node.next), size(other_node.size), level(other_node.level), prop(other_node.prop){}
    aex_node_base(aex_node_base &&other_node):prev(other_node.prev), next(other_node.next), size(other_node.size), level(other_node.level), prop(other_node.prop){}

    aex_node_base& operator = (aex_node_base &other_node) {
        this->prev = other_node.prev;this->next = other_node.next;this->size = other_node.size;this->level = other_node.level;
        this->prop = other_node.prop;
        return *this;
    }

    aex_node_base& operator = (aex_node_base &&other_node) {
        this->prev = other_node.prev;this->next = other_node.next;this->size = other_node.size;this->level = other_node.level;
        this->prop = other_node.prop;
        return *this;
    }

};


template<typename _Key,
        typename _Val,
        typename traits>
struct aex_dynamic_node_base: public aex_node_base<_Key, _Val, traits>{
public:
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef typename base_tree::components components;

    typedef typename components::base_node base_node;

    typedef aex_dynamic_node_base<key_type, value_type, traits> self;

    typedef typename components::inner_node inner_node;

    typedef typename components::data_node data_node;


    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef base_node* node_ptr;

    typedef typename components::node_balance_stats node_balance_stats;

    typedef inner_node* inner_node_ptr;

    typedef data_node* data_node_ptr;

    // size: the child node of the node(inner node); the data of the node(data node)
    // slot_size: the slot of the node
    // slot_type slot_size, real_slot_size;
    slot_type slot_size;
    
    node_balance_stats balance_stats;

    aex_dynamic_node_base():base_node(), slot_size(0), balance_stats(){}

    explicit aex_dynamic_node_base(slot_type _slot_size, int _level): base_node(), slot_size(_slot_size), balance_stats(){
        this->level = _level;
    }

    aex_dynamic_node_base(aex_dynamic_node_base &other_node):base_node(other_node), slot_size(other_node.slot_size), balance_stats(other_node.balance_stats){}

    aex_dynamic_node_base(aex_dynamic_node_base &&other_node):base_node(other_node), slot_size(other_node.slot_size), balance_stats(other_node.balance_stats){}

    aex_dynamic_node_base& operator = (aex_dynamic_node_base &other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        this->slot_size = other_node.slot_size;//this->real_slot_size = other_node.real_slot_size;
        this->prop = other_node.prop;this->balance_stats = other_node.balance_stats;
        return *this;
    }

    aex_dynamic_node_base& operator = (aex_dynamic_node_base &&other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        this->slot_size = other_node.slot_size;//this->real_slot_size = other_node.real_slot_size;
        this->prop = other_node.prop;this->balance_stats = other_node.balance_stats;
        return *this;
    }

};


/*
    memory layout:
    meta(const size): size, prop, level, Model, slot size
    key array(variable size):
    dense array: [key_1, key_2, ..., key_(n-1)]
    gap array: [x, key_1, x, key_2, ..., key_(n-1), x, max()]
    pointer array(variable size)
    bitmap array(variable size)
*/

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_inner_node : public aex_dynamic_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef aex_tree<key_type, value_type, traits> base_tree;

    typedef typename base_tree::components components;

    typedef typename components::base_node base_node;

    typedef typename components::base_dynamic_node base_dynamic_node;

    typedef typename components::inner_node inner_node;

    typedef typename components::data_node data_node;

    typedef typename components::Allocator Allocator;

    typedef base_node* node_ptr;

    typedef base_dynamic_node* dynamic_node_ptr;

    typedef typename components::bitmap_impl bitmap_impl;

    typedef typename bitmap_impl::bitmap_base bitmap_base;

    typedef typename bitmap_impl::bitmap bitmap;

    //typedef PDM_AVX<key_type, traits> Model;
    typedef typename components::InnerNodeModel Model;

    typedef typename components::node_split_stats node_split_stats;

    typedef data_node* data_node_ptr;
    
    typedef inner_node* inner_node_ptr;

    explicit aex_inner_node(slot_type _slot_size, int _level) :base_dynamic_node(_slot_size, _level), split_stats(){
        this->key_ptr = static_cast<key_type*>(malloc(Allocator::KEY_MEMORY_USED(this->slot_size)));
        this->child_ptr = static_cast<node_ptr*>(malloc(Allocator::PTR_MEMORY_USED(this->slot_size)));
        this->bitmap_ptr = static_cast<bitmap>(malloc(Allocator::BITMAP_MEMORY_USED(this->slot_size)));
    }

    ~aex_inner_node(){
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->child_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
    }

    aex_inner_node(inner_node &other_node) :base_dynamic_node(other_node), model(other_node.model){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node.bitmap_ptr, Allocator::BITMAP_MEMORY_USED(other_node.slot_size));
    }

    aex_inner_node(inner_node &&other_node) :base_dynamic_node(other_node), model(other_node.model){
        this->key_ptr = other_node.key_ptr;
        this->child_ptr = other_node.child_ptr;
        this->bitmap_ptr = other_node.bitmap_ptr;
        other_node.key_ptr = nullptr;
        other_node.child_ptr = nullptr;
        other_node.bitmap_ptr = nullptr;
    }

    aex_inner_node& operator = (aex_inner_node &other_node) {
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        *static_cast<dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        std::copy(other_node.key_ptr, other_node.key_ptr + other_node.slot_size, this->key_ptr);
        std::copy(other_node.child_ptr, other_node.child_ptr + other_node.slot_size, this->child_ptr);
        memcpy(this->bitmap_ptr, other_node.bitmap_ptr, Allocator::BITMAP_MEMORY_USED(other_node.slot_size));
        return *this;
    }

    aex_inner_node& operator = (aex_inner_node &&other_node) {
        *static_cast<dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        if (this->key_ptr != nullptr)
            free(this->key_ptr);
        if (this->child_ptr != nullptr)
            free(this->child_ptr);
        if (this->bitmap_ptr != nullptr)
            free(this->bitmap_ptr);
        this->key_ptr = other_node.key_ptr;
        this->child_ptr = other_node.child_ptr;
        this->bitmap_ptr = other_node.bitmap_ptr;
        other_node.key_ptr = nullptr;
        other_node.child_ptr = nullptr;
        other_node.bitmap_ptr = nullptr;
        return *this;
    }

    //void swap(aex_inner_node &other_node){
    //    aex_inner_node tmp;
    //    std::swap(this->key_ptr, other_node.key_ptr);
    //    std::swap(this->key_ptr, other_node.key_ptr);
    //    std::swap(this->key_ptr, other_node.key_ptr);
    //    static_cast<base_dynamic_node>(tmp) = *static_cast<dynamic_node_ptr>(other_node);
    //    *static_cast<dynamic_node_ptr>(other_node) = *static_cast<dynamic_node_ptr>(this);
    //    *static_cast<dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(tmp);
    //}

    //inline slot_type real_slot_size() const {return (this->slot_size >= traits::MIN_ML_INNER_NODE_SIZE) ? this->slot_size - traits::ERROR_BOUND : this->slot_size;}
    inline slot_type real_slot_size() const {return this->slot_size - traits::EXTERN_BUFFER_SIZE * (this->slot_size >= traits::MIN_ML_INNER_NODE_SIZE + traits::EXTERN_BUFFER_SIZE);}

    // clear bitmap
    inline void clear_bitmap(){
        memset(this->bitmap_ptr, 0, Allocator::BITMAP_MEMORY_USED(this->slot_size));
    }

    inline void clear(){
        clear_bitmap();
        std::fill(this->key_ptr, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        std::fill(this->child_ptr, this->child_ptr + this->slot_size, nullptr);
    }

    // Construct a node with key array, don't check model is fit. 
    inline void construct(const key_type* const key, node_ptr* child, const slot_type n){
        //AEX_ASSERT(IS_ML_NODE(this) == false);
        this->size = n;
        if (!IS_ML_NODE(this)){
            std::copy(key, key + n - 1, this->key_ptr);
            std::copy(child, child + n, this->child_ptr);
            std::fill(this->key_ptr + n - 1, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        }
        else{
            this->clear_bitmap();
            this->gap_array_construct(key, child, n);
        }
    }

    // construct a node with key array and model
    inline void construct(const key_type* const key, node_ptr* child, const slot_type n, const Model &m){
        AEX_ASSERT(n > 0);
        this->clear_bitmap();
        this->size = n;
        this->model = m;
        this->gap_array_construct(key, child, n);
    }

    inline void gap_array_construct(const key_type* const key, node_ptr* child, const slot_type n){
        bitmap bm = this->bitmap_ptr;
        AEX_ASSERT(IS_ML_NODE(this) == true);
        slot_type start = 0, his_pos = 0;
        for (slot_type i = 0; i < n - 1; ++i){
            slot_type pos = this->predict(key[i]);
            start = std::max(start, pos);
            bitmap_impl::set_one(bm, start);
            AEX_ASSERT(start - pos < traits::ERROR_BOUND);
            AEX_ASSERT(start < this->slot_size - 1);
            std::fill(this->key_ptr + his_pos, this->key_ptr + start + 1, key[i]);
            std::fill(this->child_ptr + his_pos, this->child_ptr + start + 1, child[i]);
            ++start;
            his_pos = start;
        }
        std::fill(this->key_ptr + his_pos, this->key_ptr + this->slot_size, std::numeric_limits<key_type>::max());
        std::fill(this->child_ptr + his_pos, this->child_ptr + this->slot_size, child[n - 1]);
    }

    inline void inplace_construct(slot_type n){
        if (IS_ML_NODE(this))
            construct(this->key_ptr + this->slot_size - n, this->child_ptr + this->slot_size - n, n, this->model);
        else{
            std::move(this->key_ptr + this->slot_size - n, this->key_ptr + this->slot_size, this->key_ptr);
            std::move(this->child_ptr + this->slot_size - n, this->child_ptr + this->slot_size, this->child_ptr);
        }
    }

    //inline NODE_INSERT_CODE insert_and_replace(const key_type &key, node_ptr old_node, node_ptr new_node){
    //    if (!IS_ML_NODE(this)) {
    //        AEX_ASSERT(this->size < this->slot_size);
    //        slot_type pos = this->find(key);
    //        AEX_ASSERT(pos < this->size);
    //        std::move_backward(this->key_ptr + pos, this->key_ptr + this->size - 1, this->key_ptr + this->size);
    //        std::move_backward(this->child_ptr + pos + 1, this->child_ptr + this->size, this->child_ptr + this->size + 1);
    //        this->key_ptr[pos] = key;
    //        this->child_ptr[pos + 1] = child;
    //        ++this->size;
    //        return NODE_INSERT_CODE::SUCCESS;
    //    }
    //    else{
    //    }
    //}

    // insert a node
    inline NODE_INSERT_CODE insert(const key_type &key, node_ptr child){
        if (!IS_ML_NODE(this)) {
            AEX_ASSERT(this->size < this->slot_size);
            slot_type pos = this->find(key);
            AEX_ASSERT(pos < this->size);
            std::move_backward(this->key_ptr + pos, this->key_ptr + this->size - 1, this->key_ptr + this->size);
            std::move_backward(this->child_ptr + pos, this->child_ptr + this->size, this->child_ptr + this->size + 1);
            this->key_ptr[pos] = key;
            this->child_ptr[pos] = child;
            ++this->size;
            return NODE_INSERT_CODE::SUCCESS;
        }
        else{
            slot_type pred_pos = this->predict(key);
            slot_type inserted_pos = pred_pos, upper_bound = std::min(this->slot_size - 1, pred_pos + traits::ERROR_BOUND);
            for (; inserted_pos < upper_bound && key > this->key_ptr[inserted_pos]; ++inserted_pos);
            // the distance between insert position of inserted item and the predict position should be less than ERROR_BOUND
            if (inserted_pos >= this->slot_size - 1 || inserted_pos - pred_pos >= traits::ERROR_BOUND)
                //goto insert_fail;
                return this->insert_fail(key, child, pred_pos);

            // the distance between insert position of shift item and the predict position should be less than ERROR_BOUND
            slot_type max_slot = std::min(inserted_pos + traits::ERROR_BOUND, this->slot_size - 1);
            slot_type empty_slot = bitmap_impl::next_empty_slot(this->bitmap_ptr, inserted_pos);
            if (empty_slot - inserted_pos >= traits::ERROR_BOUND || empty_slot >= this->slot_size - 1)
                return this->insert_fail(key, child, pred_pos);
            
            //AEX_PRINT("size=" << this->size << ", slot_size=" << this->slot_size << ", pred_pos=" << pred_pos << ", inserted_pos=" << inserted_pos << ", empty_slot=" << empty_slot << ", key=" << key << ", ori_key=" << this->key_ptr[inserted_pos] << ", is occ?=" << (bitmap_impl::at(this->bitmap_ptr, empty_slot) > 0) << ", is ori occ?" << (bitmap_impl::at(this->bitmap_ptr, inserted_pos) > 0) );
            //AEX_ASSERT(bitmap_impl::at(this->bitmap_ptr, empty_slot) == 0);
            //if (empty_slot != inserted_pos){
            //    AEX_ASSERT(bitmap_impl::at(this->bitmap_ptr, inserted_pos));
            //}
              
            std::move_backward(this->key_ptr + inserted_pos, this->key_ptr + empty_slot, this->key_ptr + empty_slot + 1);
            std::move_backward(this->child_ptr + inserted_pos, this->child_ptr + empty_slot, this->child_ptr + empty_slot + 1);
            slot_type prev_pos = bitmap_impl::prev_occ_slot(this->bitmap_ptr, inserted_pos - 1);
            std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + inserted_pos + 1, key);
            std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + inserted_pos + 1, child);
            //slot_type j = inserted_pos;
            //do{
            //    this->key_ptr[j] = key;
            //    this->child_ptr[j] = child;
            //    --j;
            //}while(j >= 0 && !bitmap_impl::at(this->bitmap_ptr, j));
            //if (prev_pos != j)
            //    AEX_PRINT(prev_pos << ", " << j);
            //AEX_ASSERT(prev_pos == j);
            bitmap_impl::set_one(this->bitmap_ptr, empty_slot);
            ++this->size; 
            this->split_stats.update(1);
            //if (!this->test()){
            //    AEX_PRINT("size=" << this->size << ", slot_size=" << this->slot_size << ", pred_pos=" << pred_pos << ", inserted_pos=" << inserted_pos << ", empty_slot=" << empty_slot << ", key=" << key << ", ori_key=" << this->key_ptr[inserted_pos] << ", is occ?=" << (bitmap_impl::at(this->bitmap_ptr, empty_slot) > 0) << ", is ori occ?" << (bitmap_impl::at(this->bitmap_ptr, inserted_pos) > 0) );
            //}
            //AEX_ASSERT(this->test());
            return NODE_INSERT_CODE::SUCCESS;
        }
    }

    inline NODE_INSERT_CODE insert_fail(const key_type &key, node_ptr child, slot_type pred_pos){
        slot_type i, start;
        AEX_ASSERT(IS_ML_NODE(this) == true);
        if (pred_pos == 0){
            for (i = start = 0; i < traits::LEFT_BUFFER_SIZE; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i)) ++start;
            if (start == traits::LEFT_BUFFER_SIZE)
                return NODE_INSERT_CODE::LEFT_BUFFER_OVERFLOW;
            for (i = start = 0; i < traits::LEFT_BUFFER_SIZE; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i)){
                this->key_ptr[start] = this->key_ptr[i];
                this->child_ptr[start] = this->child_ptr[i];
                ++start;
            }
            slot_type pos = aex::linear_search_lower_bound(this->key_ptr, this->key_ptr + start, key) - this->key_ptr;
            std::move_backward(this->key_ptr + pos, this->key_ptr + start, this->key_ptr + start + 1);
            std::move_backward(this->child_ptr + pos, this->child_ptr + start, this->child_ptr + start + 1);
            this->key_ptr[pos] = key;
            this->child_ptr[pos] = child;
            ++start;
            for (i = 0; i < start; ++i)
                bitmap_impl::set_one(this->bitmap_ptr, i);
            for (i = start; i < traits::LEFT_BUFFER_SIZE; ++i){
                this->key_ptr[i] = this->key_ptr[traits::LEFT_BUFFER_SIZE];
                this->child_ptr[i] = this->child_ptr[traits::LEFT_BUFFER_SIZE];
                bitmap_impl::set_zero(this->bitmap_ptr, i);
            }
            ++this->size;
            return NODE_INSERT_CODE::SUCCESS;
        }
        else if (pred_pos >= this->real_slot_size() + traits::LEFT_BUFFER_SIZE){
            slot_type RIGHT_BUFFER_OFFSET = this->real_slot_size() + traits::LEFT_BUFFER_SIZE;
            for (i = start = RIGHT_BUFFER_OFFSET; i < this->slot_size - 1; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i)) ++start;
            if (start == this->slot_size - 1)
                return NODE_INSERT_CODE::RIGHT_BUFFER_OVERFLOW;

            for (i = start = RIGHT_BUFFER_OFFSET; i < this->slot_size - 1; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i)){
                this->key_ptr[start] = this->key_ptr[i];
                this->child_ptr[start] = this->child_ptr[i];
                ++start;
            }
            slot_type pos = aex::linear_search_lower_bound(this->key_ptr + RIGHT_BUFFER_OFFSET, this->key_ptr + start, key) - this->key_ptr;
            std::move_backward(this->key_ptr + pos, this->key_ptr + start, this->key_ptr + start + 1);
            std::move_backward(this->child_ptr + pos, this->child_ptr + start, this->child_ptr + start + 1);
            this->key_ptr[pos] = key;
            this->child_ptr[pos] = child;
            ++start;
            for (i = RIGHT_BUFFER_OFFSET; i < start; ++i)
                bitmap_impl::set_one(this->bitmap_ptr, i);
            for (i = start; i < this->slot_size - 1; ++i){
                this->key_ptr[i] = std::numeric_limits<key_type>::max();
                this->child_ptr[i] = this->child_ptr[this->slot_size - 1];
                bitmap_impl::set_zero(this->bitmap_ptr, i);
            }
            ++this->size;
            //{
            //    slot_type sz = 0;
            //    for (slot_type i = 0; i < this->slot_size; ++i)
            //    if (bitmap_impl::at(this->bitmap_ptr, i)) ++sz;
            //    AEX_ASSERT(sz + 1 == this->size);
            //}
            //AEX_ASSERT(this->test());
            return NODE_INSERT_CODE::SUCCESS;           
        }
        else {
            this->split_stats.update(-this->real_slot_size() * base_tree::inner_node_full_ratio[this->level]);
            return NODE_INSERT_CODE::INNER_NODE_CONFLICT;
        }
    }

    inline bool test(){
        #ifdef AEX_DEBUG
        if (IS_ML_NODE(this)){
            for (slot_type i = 0; i < this->slot_size - 1; ++i)
            if (this->key_ptr[i] > this->key_ptr[i + 1]){
                AEX_ERROR("!!!");
                return false;
            }
        }
        return true;
        #endif
    }

    inline void erase(slot_type pos){
        if (IS_ML_NODE(this)){
            AEX_ASSERT(bitmap_impl::at(this->bitmap_ptr, pos) != 0);
            if (pos == this->slot_size - 1){
                if (this->size > 1){
                    slot_type prev_pos = this->prev_item(pos);
                    bitmap_impl::set_zero(this->bitmap_ptr, prev_pos);
                    slot_type prev_prev_pos = this->prev_item(prev_pos);
                    std::fill(this->key_ptr + prev_prev_pos + 1, this->key_ptr + prev_pos + 1, std::numeric_limits<key_type>::max());
                    std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + pos + 1, this->child_ptr[prev_pos]);
                }
            }
            else{
                bitmap_impl::set_zero(this->bitmap_ptr, pos);
                slot_type prev_pos = this->prev_item(pos);
                std::fill(this->key_ptr + prev_pos + 1, this->key_ptr + pos + 1, this->key_ptr[pos + 1]);
                std::fill(this->child_ptr + prev_pos + 1, this->child_ptr + pos + 1, this->child_ptr[pos + 1]);
            }
        }
        else{
            //if (pos < this->size - 1){
            std::move(this->key_ptr + pos + 1, this->key_ptr + this->size - 1, this->key_ptr + pos);
            std::move(this->child_ptr + pos + 1, this->child_ptr + this->size, this->child_ptr + pos);
            //}
            this->key_ptr[this->size - 1] = std::numeric_limits<key_type>::max();
        }
        --this->size;
    }

    // erase a node
    inline void erase(node_ptr node){
        slot_type pos = this->at(node);
        this->erase(pos);
    }

    // return the slot of child node
    inline slot_type at(const node_ptr node) const {
        AEX_ASSERT(node != nullptr);
        slot_type pred_pos;
        if (IS_ML_NODE(this)){
            if (node == this->child_ptr[this->slot_size - 1])
                return this->slot_size - 1;

            key_type node_key;
            if (IS_LEAF_NODE(node)) {
                if (node->size == 0)
                    goto at_search;
                node_key = static_cast<data_node_ptr>(node)->key[node->size - 1];
            }
            else {
                if (node->size <= 1)
                    goto at_search;
                node_key = static_cast<inner_node_ptr>(node)->key_ptr[0];
            }
            pred_pos = this->predict(node_key);
            for (slot_type i = pred_pos; i < this->slot_size; ++i)
            if (bitmap_impl::at(this->bitmap_ptr, i) && this->child_ptr[i] == node) 
                return i;
            //AEX_WARNING("IS LEAF NODE?" << IS_LEAF_NODE(node) << ", IS_ML_NODE?" << IS_ML_NODE(node));
            //AEX_WARNING("node=" << node << ", node->size=" << node->size << ", last_key=" << last_key << ", " << this->child_ptr[this->slot_size - 1]);
            AEX_ASSERT(0 == 1);
            return this->slot_size;
            at_search:
            {
                for (slot_type i = 0; i < this->slot_size; ++i)
                if (bitmap_impl::at(this->bitmap_ptr, i) && this->child_ptr[i] == node) 
                    return i;
                AEX_ASSERT(0 == 1);
                return this->slot_size;
            }
        }
        else{
            slot_type pos = std::find(this->child_ptr, this->child_ptr + this->size, node) - this->child_ptr;
            AEX_ASSERT(pos < this->size);
            return pos;
        }
    }

    // return the prev item position. If none, return slot_size
    inline slot_type prev_item(slot_type pos) const {
        if (pos == 0) return -1;
        if (IS_ML_NODE(this)){
            return bitmap_impl::prev_occ_slot(this->bitmap_ptr, pos - 1);
        }
        else 
            return pos - 1;
    }

    // return the next item position. If none, return slot_size
    inline slot_type next_item(slot_type pos) const {
        bitmap bm = this->bitmap_ptr;
        if (IS_ML_NODE(this)){
            for (slot_type i = pos + 1; i < this->slot_size; ++i)
            if (bitmap_impl::at(bm, i))
                return i;
            return this->slot_size;
        }
        else 
            return (pos >= this->size) ? this->slot_size : pos + 1;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline slot_type predict(const key_type& key) const {
        //AEX_ASSERT(model.predict(key) < 1.0 + 1e-6);
        //return std::max((slot_type)0, static_cast<slot_type>(model.predict(key) * this->real_slot_size()));
        double pred_pos = model.predict(key);
        return (pred_pos >= 0) * static_cast<slot_type>(pred_pos * this->real_slot_size() + traits::LEFT_BUFFER_SIZE);
    }

    // find key pos in which slot. If not, return last item(node->slot_size)
    inline slot_type find(const key_type& x) const{
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
                return traits::SearchClass::lower_bound(this->key_ptr, this->key_ptr + this->slot_size, x, this->key_ptr + pred_pos) - this->key_ptr;
            //for (slot_type i = pred_pos; i < this->slot_size; ++i)
            //if (x <= key_ptr[i]){
            //    return i;
            //}
            slot_type res = linear_search_lower_bound(this->key_ptr + pred_pos, this->key_ptr + this->slot_size, x) - this->key_ptr;
            return res;
            //slot_type res = std::lower_bound
            //return this->slot_size - 1;
        }
        else{
            if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
                return traits::SearchClass::lower_bound(this->key_ptr, this->key_ptr + this->size - 1, x, this->key_ptr) - this->key_ptr;
            slot_type pos = std::lower_bound(this->key_ptr, this->key_ptr + this->size - 1, x) - this->key_ptr;
            return pos;
        }
    }

    inline slot_type last(){return IS_ML_NODE(this) ? (this->slot_size - 1) : (this->size - 1);}

public:

    Model model;

    key_type* key_ptr;

    node_ptr* child_ptr;
    
    bitmap bitmap_ptr;

    node_split_stats split_stats;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_data_node : public aex_dynamic_node_base<_Key, _Val, traits>{
public:

    typedef aex_tree<_Key, _Val, traits> base_tree;

    typedef typename base_tree::components components;

    typedef typename components::Allocator Allocator;

    typedef typename components::base_node base_node;

    typedef aex_data_node<_Key, _Val, traits> data_node;

    typedef typename components::base_dynamic_node base_dynamic_node;
    
    typedef base_node* node_ptr;
    
    typedef base_dynamic_node* base_dynamic_node_ptr;

    typedef data_node* data_node_ptr;
    
    typedef _Key key_type;

    typedef _Val value_type;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef linear_model<_Key, traits> Model;

    Model model;
    
    key_type *key;

    value_type *data;

    bool is_dirty;

    //typedef linear_model<key_type, traits> Model;

    aex_data_node() {}

    explicit aex_data_node(slot_type _slot_size)  :base_dynamic_node(_slot_size){
        this->key = static_cast<key_type*>(malloc(Allocator::KEY_MEMORY_USED(_slot_size)));
        this->data = static_cast<value_type*>(malloc(Allocator::KEY_MEMORY_USED(_slot_size)));
        std::fill(this->key, this->key + _slot_size, std::numeric_limits<key_type>::max());
    }

    ~aex_data_node(){
        if (key != nullptr)
            free(this->key);
        if (data != nullptr)
            free(this->data);
    }

    aex_data_node(aex_data_node &other_node)  :base_dynamic_node(other_node), model(other_node.model){
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key, other_node.key + other_node.size, this->key);
        std::copy(other_node.data, other_node.data + other_node.size, this->data);
    }

    aex_data_node(aex_data_node &&other_node)  :base_dynamic_node(other_node), model(other_node.model){
        if (this->key != nullptr)
            free(this->key);
        if (this->data != nullptr)
            free(this->data);
        this->key = other_node.key;
        this->data = other_node.data;
        other_node.key = nullptr;
        other_node.data = nullptr;
    }

    aex_data_node& operator = (aex_data_node &other_node) {
        AEX_ASSERT(this->slot_size == other_node.slot_size);
        *static_cast<base_dynamic_node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        std::copy(other_node.key, other_node.key + other_node.size, this->key);
        std::copy(other_node.data, other_node.data + other_node.size, this->data);
        return *this;
    }

    aex_data_node& operator = (aex_data_node &&other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_dynamic_node>(other_node);
        model = other_node.model;
        if (this->key != nullptr)
            free(this->key);
        if (this->data != nullptr)
            free(this->data);
        this->key = other_node.key;
        this->data = other_node.data;
        other_node.key = nullptr;
        other_node.data = nullptr;
        return *this;
    }

    // only node_property::ML_NODE can use it. check if node_property::ML_NODE first
    // position range [0, slot_size)
    inline slot_type predict(const key_type& key) const {
        return std::max((slot_type)0, std::min(static_cast<slot_type>(model.predict(key) * this->size), this->size - 1));
    }

    void construct(const key_type *_key, const value_type *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        std::copy(_key, _key + nums - 1, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
    }

    void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    void construct(const key_type *_key, const value_type *_data, slot_type nums, Model &m){
        AEX_ASSERT(IS_ML_NODE(this) == true);
        std::move(_key, _key + nums, this->key);
        std::move(_data, _data + nums, this->data);
        this->size = nums;
        this->model = m;
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_lower_pos(x);
        AEX_ASSERT(x < key[pos]);
        insert(x, data, pos);
        return pos;
    }

    // insert a item in position
    inline void insert(const key_type &x, const value_type &data, const slot_type pos){
        AEX_ASSERT(this->size == this->slot_size);
        std::move_backward(this->key + pos, this->key + this->size, this->key + this->size + 1);
        std::move_backward(this->data + pos, this->data + this->size, this->data + this->size + 1);
        this->key[pos] = x;
        this->data[pos] = data;
        this->size++;
    }

    inline void erase(const slot_type pos){
        AEX_ASSERT(pos < this->size);
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->key[this->size - 1] = std::numeric_limits<key_type>::max();
        this->size--;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x){
        slot_type pos;
        if (IS_ML_NODE(this)){
            slot_type pred_pos = this->predict(x);
            slot_type upper_bound = std::min(pred_pos + traits::DATA_NODE_ERROR_BOUND + 1, this->size);
            for (slot_type i = std::max(0, pred_pos); i < upper_bound; ++i)
            if (x <= key[pos])
            pos = aex::exponential_search_lower_bound(this->key, this->key + this->size, this->key + pred_pos, x) - this->key;
        }
        else{
            return std::lower_bound(this->key, this->key + this->size, x) - this->key;
        }
        return pos;
    }

    inline double RMSE(){
        if (IS_ML_NODE(this)){
            return this->model.RMSE(this->key, this->size);
        }
        else return 0;
    }
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

    typedef typename components::base_node base_node;

    //typedef linear_model<key_type, traits> Model;
    typedef typename components::DataNodeModel Model;

    typedef aex_static_data_node<_Key, _Val, traits> data_node;
    
    typedef base_node* node_ptr;

    typedef data_node* data_node_ptr;
    
    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;
    
    key_type key[traits::MIN_DATA_NODE_SLOT_SIZE];

    value_type data[traits::MIN_DATA_NODE_SLOT_SIZE];


    aex_static_data_node() :base_node(){

    }

    //explicit aex_static_data_node(slot_type _slot_size):base_node(_slot_size){
    //}

    ~aex_static_data_node() {
    }

    aex_static_data_node(aex_static_data_node &other_node) :base_node(other_node){
        //AEX_ASSERT(this->slot_size == other_node.slot_size);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
    }

    aex_static_data_node(aex_static_data_node &&other_node) :base_node(other_node){
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
    }

    aex_static_data_node& operator = (aex_static_data_node &other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        return *this;
    }

    aex_static_data_node& operator = (aex_static_data_node &&other_node) {
        *static_cast<node_ptr>(this) = static_cast<base_node>(other_node);
        std::copy(other_node.key, other_node.key + traits::MIN_DATA_NODE_SLOT_SIZE, this->key);
        std::copy(other_node.data, other_node.data + traits::MIN_DATA_NODE_SLOT_SIZE, this->data);
        return *this;
    }

    inline void construct(const key_type *_key, const value_type *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        //AEX_ASSERT(nums >= traits::MIN_DATA_NODE_SLOT_SIZE / 2);
        std::copy(_key, _key + nums, this->key);
        std::copy(_data, _data + nums, this->data);
        this->size = nums;
        std::fill(this->key + nums, this->key + traits::MIN_DATA_NODE_SLOT_SIZE, std::numeric_limits<key_type>::max());
    }

    inline void construct(const std::pair<key_type, value_type> *_data, slot_type nums){
        AEX_ASSERT(IS_ML_NODE(this) == false);
        AEX_ASSERT(nums >= traits::MIN_DATA_NODE_SLOT_SIZE / 2);
        std::vector<key_type> _key(nums);
        std::vector<value_type> _value(nums);
        for (slot_type i = 0; i < nums; ++i){
            _key[i] = _data[i].first;
            _value[i] = _data[i].second;
        }
        this->construct(_key.data(), _value.data(), nums);
    }

    inline void construct(const key_type *_key, const value_type *_data, slot_type nums, Model &m){
        AEX_ASSERT(false == true);
    }

    // insert a item
    inline slot_type insert(const key_type &x, const value_type &data){
        slot_type pos = this->find_lower_pos(x);
        //slot_type pos = this->find_upper_pos(x) - 1;
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

    inline void erase(const slot_type pos){
        AEX_ASSERT(pos < this->size);
        std::move(this->key + pos + 1, this->key + this->size, this->key + pos);
        std::move(this->data + pos + 1, this->data + this->size, this->data + pos);
        this->key[this->size - 1] = std::numeric_limits<key_type>::max();
        this->size--;
    }

    // if no item greater than or equal x, return slot_size
    inline slot_type find_lower_pos(const key_type &x){
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::lower_bound(this->key, this->key + this->size, x, this->key) - this->key;
        return std::lower_bound(this->key, this->key + this->size, x) - this->key;
    }

    inline slot_type find_upper_pos(const key_type &x){
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::upper_bound(this->key, this->key + this->size, x, this->key) - this->key;
        return std::upper_bound(this->key, this->key + this->size, x) - this->key;
    }

};

//template<typename _Node>
//std::swap(_Node a, _Node b){
//    a.swap(b);
//}

}