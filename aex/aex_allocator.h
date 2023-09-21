#pragma once

#include "aex/aex_def.h"
#include "aex/aex_balance.h"

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
class aex_node_allocator{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    //typedef aex_allocator<_Key, _Val, traits> allocator;

    typedef aex_node_allocator<_Key, _Val, traits> self;

    typedef aex_tree<_Key, _Val, traits> base_tree;

    typedef aex_node_base<key_type, value_type, traits> base_node;

    typedef base_node* node_ptr;

    typedef aex_inner_node<key_type, value_type, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model Model;

    //typedef aex_data_node<key_type, value_type, traits> data_node;

    typedef typename base_tree::dynamic_data_node dynamic_data_node;
    typedef typename base_tree::dynamic_data_node_ptr dynamic_data_node_ptr;
    typedef typename base_tree::static_data_node static_data_node;
    typedef typename base_tree::static_data_node_ptr static_data_node_ptr;

    typedef typename base_tree::data_node data_node;
    
    //static_assert(traits::AllowDynamicDataNode::value != std::is_same<aex_static_data_node<_Key, _Val, traits>, data_node>::value);

    typedef data_node* data_node_ptr;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef typename traits::version_type version_type;

    typedef typename aex_bitmap_impl<traits>::bitmap bitmap;
    
    typedef typename aex_bitmap_impl<traits>::bitmap_base bitmap_base;

    const int node_buffer_size = 20;

    #ifdef AEX_EXPERIMENT
    aex_node_allocator():inner_node_nums(0), data_node_nums(0), free_cnt(0), alloc_cnt(0), timer(), max_node_id(0), _memory_used(0){
        //inner_node_buffer.resize(node_buffer_size);
    }
    #else
    aex_node_allocator():inner_node_nums(0), data_node_nums(0), free_cnt(0), alloc_cnt(0), _memory_used(0){}
    #endif

    ~aex_node_allocator(){
    }

    #ifdef AEX_EXPERIMENT
    void clear(){
        inner_node_nums = data_node_nums = free_cnt = alloc_cnt = 0;
        timer = Timer();
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
            //AEX_FORMAT("alloc_cnt=%lld", alloc_cnt);
        #endif
        return static_cast<void*>(malloc(size));
    }

   inline key_type* allocate_key_buffer(size_type size){
        #ifdef AEX_DEBUG
            ++alloc_cnt;
        #endif
        return static_cast<key_type*>(malloc(size * sizeof(key_type)));
    }


    inline node_ptr* allocate_nodeptr_buffer(size_type size){
        #ifdef AEX_DEBUG
            ++alloc_cnt;
        #endif
        return static_cast<node_ptr*>(malloc(size * sizeof(node_ptr)));
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
        return sizeof(static_data_node);
    }

    inline static size_type DYNAMIC_DATA_NODE_MEMORY_USED(size_type slot_size){
        return sizeof(dynamic_data_node);
    }

    //inline static size_type MUTEX_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(aex_spinlock) * slot_size / traits::ERROR_BOUND);
    //}
//
    //inline size_type VERSION_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(version_type) * slot_size / traits::ERROR_BOUND);
    //}

    inline inner_node_ptr allocate_inner_node(slot_type slot_size, bool ml_node_flag){
        /*
        *   TODO: memory pool
        */
        #ifdef AEX_EXPERIMENT
        std::chrono::system_clock::time_point t1, t2;
        t1 = std::chrono::high_resolution_clock::now();
        #endif
        ++inner_node_nums;
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);

        size_type real_slot_size = slot_size;

        if (ml_node_flag == true)
            AEX_ASSERT(slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE);

        if (real_slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE) 
            real_slot_size += traits::ERROR_BOUND;

        size_type memory_used = INNER_NODE_MEMORY_USED(real_slot_size);
        this->_memory_used += memory_used;
        inner_node_ptr node = new inner_node(real_slot_size);
        //inner_node_ptr node;// = new inner_node(real_slot_size);
        //if (node_buffer.size() == 0){
        //    inner_node_ptr* node_buffer = ;
        //    for (inner_node_ptr i = 0; i < ; ++i)
        //        
        //}

        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        if (ml_node_flag){
            SET_FLAG(node, node_property::ML_NODE);
            node->clear_bitmap();
        }

        #ifdef AEX_EXPERIMENT
        t2 = std::chrono::high_resolution_clock::now();
        timer.allocate_inner_node_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        #endif

        return node;
    }

    inline static_data_node_ptr allocate_static_data_node(){
        ++data_node_nums;
        this->_memory_used += STATIC_DATA_NODE_MEMORY_USED();
        data_node_ptr node = new static_data_node();
        SET_FLAG(node, node_property::LEAF);
        SET_FLAG(node, node_property::STATIC_NODE);
        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif
        return node;
    }
    
    inline dynamic_data_node_ptr allocate_dynamic_data_node(slot_type slot_size, bool ml_node_flag){
        #ifdef AEX_EXPERIMENT
        std::chrono::system_clock::time_point t1, t2;
        t1 = std::chrono::high_resolution_clock::now();
        #endif
        ++data_node_nums;
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);

        this->_memory_used += DYNAMIC_DATA_NODE_MEMORY_USED(slot_size);
        dynamic_data_node_ptr node = new dynamic_data_node(slot_size);
        SET_FLAG(node, node_property::LEAF);
        AEX_ASSERT(IS_LEAF_NODE(node));
        
        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif
        
        if (ml_node_flag == true && node->slot_size > traits::MIN_ML_DATA_NODE_SLOT_SIZE)
            SET_FLAG(node, node_property::ML_NODE);

        #ifdef AEX_EXPERIMENT
        t2 = std::chrono::high_resolution_clock::now();
        timer.allocate_data_node_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        #endif

        return node;
    }

    inline void reallocate(inner_node_ptr node, slot_type new_slot_size){
        if (new_slot_size >= traits::MIN_ML_INNER_NODE_SLOT_SIZE)
            new_slot_size += traits::ERROR_BOUND;

        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) \
                              - PTR_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(new_slot_size) \
                              - BITMAP_MEMORY_USED(node->slot_size) + BITMAP_MEMORY_USED(new_slot_size);
        key_type *new_key_ptr = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
        node_ptr *new_child_ptr = static_cast<node_ptr*>(malloc(PTR_MEMORY_USED(new_slot_size)));
        bitmap new_bitmap_ptr = static_cast<bitmap>(malloc(BITMAP_MEMORY_USED(new_slot_size)));
        base_tree::copy_to_buffer(node, new_key_ptr, new_child_ptr);
        if (node->key_ptr != nullptr){
            free(node->key_ptr);
            free(node->child_ptr);
            free(node->bitmap_ptr);
        }
        node->key_ptr = new_key_ptr;
        node->child_ptr = new_child_ptr;
        node->bitmap_ptr = new_bitmap_ptr;
        std::fill(node->key_ptr + node->size, node->key_ptr + node->slot_size, std::numeric_limits<key_type>::max());
        std::fill(node->child_ptr + node->size, node->child_ptr + node->slot_size, node->child_ptr[last_pos]);
        node->slot_size = new_slot_size;
    }

    inline void reallocate(dynamic_data_node_ptr node, slot_type new_slot_size){
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

    inline void deallocate(key_type* p){
        #ifdef AEX_DEBUG
        ++free_cnt;
        #endif
        if (p != nullptr)
            free(p);
    }

    inline void deallocate(node_ptr* p){
        #ifdef AEX_DEBUG
        ++free_cnt;
        #endif
        if (p != nullptr)
            free(p);
    }


    inline void deallocate(void* p){
        #ifdef AEX_DEBUG
        ++free_cnt;
        #endif
        if (p != nullptr)
            free(p);
    }

    inline void free_node(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_ASSERT(p != nullptr);
        _memory_used -= INNER_NODE_MEMORY_USED(p->slot_size);
        --inner_node_nums;
        delete p;
    }

    inline void free_node(dynamic_data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_ASSERT(p != nullptr);
        _memory_used -= DYNAMIC_DATA_NODE_MEMORY_USED(p->slot_size);
        --data_node_nums;
        delete p;
    }

    inline void free_node(static_data_node_ptr p){
        AEX_ASSERT(p != nullptr);
        _memory_used -= STATIC_DATA_NODE_MEMORY_USED();
        --data_node_nums;
        delete p;
    }

    inline void free_node(node_ptr p){      
        if (p != nullptr){
            if (IS_LEAF_NODE(p)) {
                if constexpr (std::is_same<data_node, static_data_node>::value) 
                    free_node(static_cast<static_data_node_ptr>(p));
                else free_node(static_cast<dynamic_data_node_ptr>(p));
            }
            else free_node(static_cast<inner_node_ptr>(p));
        }
    }

    #ifdef AEX_EXPERIMENT
    inline void print_stats(){
        AEX_IMPORTANT("[Allocator]: memory used=" << _memory_used << " bytes, inner_node_nums=" << inner_node_nums << ", data_node_nums=" << data_node_nums <<
                    ", allocate inner node used time=" << timer.allocate_inner_node_time << "ms" <<
                    ", allocate data node used time=" << timer.allocate_data_node_time << "ms"
                    ", allocator count=" << alloc_cnt << ", free count=" << free_cnt);
        AEX_HINT("allocate inner node used time=" << timer.allocate_inner_node_time << "ms"
                << ", allocate data node used time=" << timer.allocate_data_node_time << "ms");
    }
    #else
    inline void print_stats(){}
    #endif


#ifndef AEX_EXPERIMENT
private:
#endif
    size_type inner_node_nums, data_node_nums, free_cnt, alloc_cnt;
    //std::vector<inner_node_ptr> inner_node_buffer;

    #ifdef AEX_EXPERIMENT
    struct Timer{
        Timer():allocate_inner_node_time(0), allocate_data_node_time(0){}
        double allocate_inner_node_time, allocate_data_node_time;
    }timer;
    std::map<node_ptr, size_type> node_id;
    std::vector<node_ptr> id_node;
    size_type max_node_id;
    #endif
public:
    // status
    size_type _memory_used;

};

} // namespace name
