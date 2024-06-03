#pragma once

#include "aex/aex_def.h"

namespace aex
{

enum memory_prop{
    MAX_POOL_MEMORY=0x400000,
    MAX_POOL_NODE=0x20,
    DEFAULT_DATA_NODE_SIZE=0x8,
};

struct memory_config{
    std::vector<size_t> node_size_vec;
};

template<typename _Key, 
        typename _Val,
        typename traits>
class aex_allocator{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef aex_allocator<_Key, _Val, traits> self;

    typedef aex_tree<_Key, _Val, traits> base_tree;

    typedef typename base_tree::components components;

    typedef typename components::base_node base_node;

    typedef base_node* node_ptr;

    typedef typename components::inner_node inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename components::InnerNodeModel InnerNodeModel;

    typedef typename components::data_node data_node;
    
    typedef data_node* data_node_ptr;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef typename components::bitmap_impl bitmap_impl;
    
    typedef typename traits::bitmap_base bitmap_base;
    typedef typename traits::bitmap bitmap;

    //const int node_buffer_size = 20;

    #ifdef AEX_DEBUG
    aex_allocator():inner_node_nums(0), 
                    data_node_nums(0), 
                    free_cnt(0), 
                    alloc_cnt(0), 
                    max_node_id(0), 
                    _memory_used(0), 
                    static_key_buf_used(0), 
                    static_nodeptr_buf_used(0){
        //allocate_data_node_buffer();
    }
    #else
    aex_allocator():_memory_used(0){}
    #endif

    ~aex_allocator(){
    }

    #ifdef AEX_DEBUG
    void clear(){
        inner_node_nums = data_node_nums = free_cnt = alloc_cnt = 0;
        _memory_used = max_node_id = 0;
    }
    #else
    void clear(){}
    #endif

    inline void* _allocate(size_type size){
        /* 
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++alloc_cnt;
        #endif
        return static_cast<void*>(malloc(size));
    }

    inline key_type* allocate_key_buffer(size_type size){
        #ifdef AEX_DEBUG
        //    ++alloc_cnt;
        AEX_ASSERT(this->static_key_buf_used == 0);
        ++static_key_buf_used;
        //AEX_PRINT("static_key_buf_used=" << this->static_key_buf_used);
        #endif
        static_key_buf.reserve(size);
        //return static_cast<key_type*>(malloc(size * sizeof(key_type)));
        return static_key_buf.data();
    }


    inline node_ptr* allocate_nodeptr_buffer(size_type size){
        #ifdef AEX_DEBUG
        //    ++alloc_cnt;
            AEX_ASSERT(this->static_nodeptr_buf_used == 0);
            ++static_nodeptr_buf_used;
        #endif
        static_nodeptr_buf.reserve(size);
        //return static_cast<node_ptr*>(malloc(size * sizeof(node_ptr)));
        return static_nodeptr_buf.data();
    }

    inline value_type* allocate_data_buffer(size_type size){
        #ifdef AEX_DEBUG
            ++alloc_cnt;
        #endif
        return static_cast<value_type*>(malloc(size * sizeof(value_type)));
    }

    // used memory size of key array, align 8 bytes
    inline static size_type KEY_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(key_type));
    }

    // used memory size of pointer array, align 8 bytes
    inline static size_type PTR_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(node_ptr));
    }

    // used memory size of bitmap, align 8 bytes
    inline static size_type BITMAP_MEMORY_USED(size_type slot_size){
        return align_8bytes(((slot_size >> 6) + ((slot_size & 63) > 0)) * sizeof(typename traits::bitmap_base));
    }

    // used memory size of data array, align 8 bytes
    inline static size_type DATA_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(value_type));
    }
    
    inline static size_type INNER_NODE_MEMORY_USED(size_type slot_size){ 
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + \
        align_8bytes(sizeof(inner_node));
    }

    inline static size_type STATIC_DATA_NODE_MEMORY_USED(){
        return sizeof(data_node);
    }

    inline static size_type DYNAMIC_DATA_NODE_MEMORY_USED(size_type slot_size){
        return sizeof(data_node) + sizeof(key_type) * slot_size + sizeof(value_type) * slot_size;
    }

    //inline static size_type MUTEX_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(aex_spinlock) * slot_size / traits::ERROR_BOUND);
    //}
//
    //inline size_type VERSION_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(version_type) * slot_size / traits::ERROR_BOUND);
    //}

    inline inner_node_ptr allocate_inner_node(slot_type real_slot_size, bool ml_node_flag){
        /*
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
        ++inner_node_nums;
        ++alloc_cnt;
        #endif

        slot_type slot_size = real_slot_size;
        //AEX_PRINT("slot_size=" << slot_size);
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);

        slot_size += (real_slot_size >= traits::MIN_ML_INNER_NODE_SIZE) * traits::ERROR_BOUND;

        size_type memory_used = INNER_NODE_MEMORY_USED(slot_size);
        this->_memory_used += memory_used;
        inner_node_ptr node = new inner_node(slot_size);
        //node->real_slot_size = real_slot_size;

        #ifdef AEX_DEBUG
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        if (ml_node_flag){
            SET_FLAG(node, node_property::ML_NODE);
            node->clear_bitmap();
        }

        return node;
    }

    //inline void allocate_data_node_buffer(){
    //    //data_node_ptr* p = static_cast<data_node_ptr*>(malloc(sizeof(data_node) * 128));
    //    for (int i = 0; i < 128; ++i)
    //        data_node_buffer.push_back(new data_node());
    //}

    inline data_node_ptr allocate_static_data_node(){
        this->_memory_used += STATIC_DATA_NODE_MEMORY_USED();
        //if (data_node_buffer.size() == 0)
        //    allocate_data_node_buffer();
        //data_node_ptr node = data_node_buffer[data_node_buffer.size() - 1];
        //data_node_buffer.pop_back();
        data_node_ptr node = new data_node();
        SET_FLAG(node, node_property::LEAF);
        SET_FLAG(node, node_property::STATIC_NODE);

        #ifdef AEX_DEBUG
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        return node;
    }
    
    inline data_node_ptr allocate_dynamic_data_node(slot_type slot_size, bool ml_node_flag){
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        this->_memory_used += DYNAMIC_DATA_NODE_MEMORY_USED(slot_size);
        data_node_ptr node = new data_node(slot_size);
        SET_FLAG(node, node_property::LEAF);
        AEX_ASSERT(IS_LEAF_NODE(node));
        
        #ifdef AEX_DEBUG
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif
        
        if (ml_node_flag == true && node->slot_size > traits::MIN_ML_DATA_NODE_SLOT_SIZE)
            SET_FLAG(node, node_property::ML_NODE);

        return node;
    }

    inline data_node_ptr allocate_data_node(){
        #ifdef AEX_DEBUG
        ++alloc_cnt;
        ++data_node_nums;
        #endif
        return allocate_static_data_node();
    }

    inline data_node_ptr allocate_data_node(slot_type slot_size, bool ml_node_flag){
        return allocate_dynamic_data_node(slot_size, ml_node_flag);
    }

    inline void reallocate(inner_node_ptr node, slot_type new_slot_size){
        new_slot_size += (new_slot_size >= traits::MIN_ML_INNER_NODE_SIZE) * traits::ERROR_BOUND;

        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) \
                              - PTR_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(new_slot_size) \
                              - BITMAP_MEMORY_USED(node->slot_size) + BITMAP_MEMORY_USED(new_slot_size);
        AEX_ASSERT(node->key_ptr != nullptr);
        node->key_ptr = static_cast<key_type*>(realloc(node->key_ptr, KEY_MEMORY_USED(new_slot_size)));
        node->child_ptr = static_cast<node_ptr*>(realloc(node->child_ptr, PTR_MEMORY_USED(new_slot_size)));
        node->bitmap_ptr = static_cast<bitmap>(realloc(node->bitmap_ptr, BITMAP_MEMORY_USED(new_slot_size)));
        node->slot_size = new_slot_size;
    }

    inline void reallocate_and_copy(inner_node_ptr node, slot_type new_slot_size){
        new_slot_size += (new_slot_size >= traits::MIN_ML_INNER_NODE_SIZE) * traits::ERROR_BOUND;

        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) \
                              - PTR_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(new_slot_size) \
                              - BITMAP_MEMORY_USED(node->slot_size) + BITMAP_MEMORY_USED(new_slot_size);
        key_type *new_key_ptr = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
        node_ptr *new_child_ptr = static_cast<node_ptr*>(malloc(PTR_MEMORY_USED(new_slot_size)));
        bitmap new_bitmap_ptr = static_cast<bitmap>(malloc(BITMAP_MEMORY_USED(new_slot_size)));
        AEX_ASSERT(node->key_ptr != nullptr);
        base_tree::copy_to_buffer(node, new_key_ptr, new_child_ptr);
        free(node->key_ptr);
        free(node->child_ptr);
        free(node->bitmap_ptr);
        node->key_ptr = new_key_ptr;
        node->child_ptr = new_child_ptr;
        node->bitmap_ptr = new_bitmap_ptr;
        node->slot_size = new_slot_size;
    }

    inline void reallocate_and_save(inner_node_ptr node, slot_type new_slot_size){
        new_slot_size += (new_slot_size > traits::MIN_ML_INNER_NODE_SIZE) * traits::ERROR_BOUND;

        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) \
                              - PTR_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(new_slot_size) \
                              - BITMAP_MEMORY_USED(node->slot_size) + BITMAP_MEMORY_USED(new_slot_size);
        key_type *new_key_ptr = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
        node_ptr *new_child_ptr = static_cast<node_ptr*>(malloc(PTR_MEMORY_USED(new_slot_size)));
        bitmap new_bitmap_ptr = static_cast<bitmap>(malloc(BITMAP_MEMORY_USED(new_slot_size)));
        AEX_ASSERT(node->key_ptr != nullptr);
        free(node->bitmap_ptr);
        node->key_ptr = new_key_ptr;
        node->child_ptr = new_child_ptr;
        node->bitmap_ptr = new_bitmap_ptr;
        node->slot_size = new_slot_size;
    }

    inline void reallocate(data_node_ptr node, slot_type new_slot_size){
        if constexpr(traits::AllowDynamicDataNode){
            this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) -
                                            DATA_MEMORY_USED(node->slot_size) + DATA_MEMORY_USED(new_slot_size);
            key_type *new_key = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
            value_type *new_data = static_cast<value_type*>(malloc(DATA_MEMORY_USED(new_slot_size)));
            node->slot_size = new_slot_size;
            std::copy(node->key, node->key + node->size, new_key);
            std::copy(node->data, node->data + node->size, new_data);
            if (node->key != nullptr){
                free(node->key);
                free(node->data);
            }
            node->key = new_key;
            node->data = new_data;
        }
    }

    inline std::vector<key_type>& allocate_dynamic_key_buf(unsigned int n){
        //#ifdef AEX_DEBUG
        //AEX_ASSERT(dynamic_key_buf_used == 0);
        //++dynamic_key_buf_used;
        //#endif
        dynamic_key_buf[n].clear();
        return dynamic_key_buf[n];
    }

    inline std::vector<node_ptr>& allocate_dynamic_nodeptr_buf(unsigned int n){
        //#ifdef AEX_DEBUG
        //AEX_ASSERT(dynamic_key_buf_used == 0);
        //++dynamic_key_buf_used;
        //#endif
        dynamic_nodeptr_buf[n].clear();
        return dynamic_nodeptr_buf[n];
    }

    inline void deallocate_key_buffer(key_type* p){
        #ifdef AEX_DEBUG
        //++free_cnt;
        --static_key_buf_used;
        static_key_buf.clear();
        //AEX_PRINT("static_key_buf_used=" << this->static_key_buf_used);
        #endif
        //if (p != nullptr)
        //    free(p);
    }

    inline void deallocate_nodeptr_buffer(node_ptr* p){
        #ifdef AEX_DEBUG
        //++free_cnt;
        --static_nodeptr_buf_used;
        static_nodeptr_buf.clear();
        #endif
        //if (p != nullptr)
        //    free(p);
    }

    //inline void deallocate(std::vector<key_type> &p){
    //    #ifdef AEX_DEBUG
    //    AEX_ASSERT(dynamic_key_buf_used < 4);
    //    #endif
    //}
//
    //inline void deallocate(std::vector<node_ptr> &p){
    //    #ifdef AEX_DEBUG
    //    AEX_ASSERT(dynamic_nodeptr_buf_used < 4);
    //    --dynamic_nodeptr_buf_used;
    //    #endif
    //}

    inline void free_node(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_ASSERT(p != nullptr);
        if (p->key_ptr != nullptr)
            _memory_used -= INNER_NODE_MEMORY_USED(p->slot_size);
        else
            _memory_used -= sizeof(inner_node);
        #ifdef AEX_DEBUG
        --inner_node_nums;
        #endif
        delete p;
    }

    inline void free_node(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_ASSERT(p != nullptr);
        if constexpr(traits::AllowDynamicDataNode)
            _memory_used -= DYNAMIC_DATA_NODE_MEMORY_USED(p->slot_size);
        else
            _memory_used -= STATIC_DATA_NODE_MEMORY_USED();
        #ifdef AEX_DEBUG
        --data_node_nums;
        #endif
        delete p;
    }

    inline void free_node(node_ptr p){      
        if (p != nullptr){
            if (IS_LEAF_NODE(p)) 
                free_node(static_cast<data_node_ptr>(p));
            else 
                free_node(static_cast<inner_node_ptr>(p));
        }
    }

    #ifdef AEX_DEBUG
    inline void print_stats(){
        AEX_IMPORTANT("[Allocator]: memory used=" << _memory_used << " bytes, =" << _memory_used / 1024 / 1024 << "MB, inner_node_nums=" << inner_node_nums << ", data_node_nums=" << data_node_nums <<
                    ", allocator count=" << alloc_cnt << ", free count=" << free_cnt);
    }
    #else
    inline void print_stats(){}
    #endif


#ifdef AEX_DEBUG
public:
    size_type inner_node_nums, data_node_nums, free_cnt, alloc_cnt;
    std::map<node_ptr, size_type> node_id;
    std::vector<node_ptr> id_node;
    size_type max_node_id;
#endif

public:
    // status
    size_type _memory_used;

#ifndef AEX_DEBUG
private:
#endif
    std::vector<key_type> dynamic_key_buf[2], static_key_buf;
    std::vector<node_ptr> dynamic_nodeptr_buf[2], static_nodeptr_buf;
    //size_type dynamic_key_buf_used, dynamic_child_buf_used;
#ifdef AEX_DEBUG
    size_type static_key_buf_used, static_nodeptr_buf_used;
#endif
};


} // namespace name
