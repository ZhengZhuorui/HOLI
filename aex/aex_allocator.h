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

    typedef aex_data_node<key_type, value_type, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef typename traits::version_type version_type;

    typedef typename aex_bitmap_impl<traits>::bitmap bitmap;

    aex_node_allocator():inner_node_nums(0), data_node_nums(0), free_cnt(0), alloc_cnt(0), timer(), _memory_used(0){
        #ifdef AEX_EXPERIMENT
        max_node_id = 0;
        #endif
    }

    ~aex_node_allocator(){
        #ifdef AEX_DEBUG
        AEX_HINT("allocate inner node used time=" << timer.allocate_inner_node_time << "ms");
        AEX_HINT("allocate data node used time=" << timer.allocate_data_node_time << "ms");
        #endif
    }

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
        return align_8bytes((slot_size) * sizeof(char*));
    }

    // used memory size of bitmap, align 8 bytes
    inline static size_type BITMAP_MEMORY_USED(size_type slot_size){
        return align_8bytes(((slot_size >> 6) + ((slot_size & 63) > 0)) * sizeof(bitmap));
    }

    // used memory size of data array, align 8 bytes
    inline static size_type DATA_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(value_type));
    }
    
    inline static size_type INNER_NODE_MEMORY_USED(size_type slot_size){ 
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + \
        align_8bytes(sizeof(inner_node));
    }

    inline static size_type DATA_NODE_MEMORY_USED(size_type slot_size){
        return align_8bytes(sizeof(data_node)) + KEY_MEMORY_USED(slot_size) + DATA_MEMORY_USED(slot_size);
    }

    //inline static size_type MUTEX_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(aex_spinlock) * slot_size / traits::ERROR_BOUND);
    //}
//
    //inline size_type VERSION_MEMORY_USED(size_type slot_size){
    //    return align_8bytes(sizeof(version_type) * slot_size / traits::ERROR_BOUND);
    //}

    inline inner_node_ptr allocate_inner_node(size_type slot_size, bool ml_node_flag=true){
        /*
        *   TODO: memory pool
        */
        #ifdef AEX_EXPERIMENT
        std::chrono::system_clock::time_point t1, t2;
        t1 = std::chrono::high_resolution_clock::now();
        #endif
        ++inner_node_nums;
        size_type real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        
        real_slot_size += traits::ERROR_BOUND;
        size_type memory_used = INNER_NODE_MEMORY_USED(real_slot_size);
        this->_memory_used += memory_used;
        //if (slot_size / traits::MIN_INNER_NODE_SLOT_SIZE)

        //inner_node_ptr node = static_cast<inner_node_ptr>(this->_allocate(memory_used));
        inner_node_ptr node = new inner_node();
        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        node->slot_size = real_slot_size;
        node->prop = node->size = 0;
        node->balance_stats = aex_node_balance_stats<typename traits::AllowBalance>();

        // offset: meta data
        node->key_ptr = static_cast<key_type*>(malloc(KEY_MEMORY_USED(node->slot_size)));

        // offset: meta data + key array
        node->child_ptr = static_cast<node_ptr*>(malloc(PTR_MEMORY_USED(node->slot_size)));

        // offset: meta data + key array + pointer array
        node->bitmap_ptr = static_cast<bitmap>(malloc(BITMAP_MEMORY_USED(node->slot_size)));
                        
        memset(node->key_ptr, 0, node->slot_size);

        if (real_slot_size > traits::MIN_ML_INNER_NODE_SLOT_SIZE && ml_node_flag){
            node->prop |= node_property::ML_NODE;
            node->clear_bitmap();
        }

        //AEX_FORMAT("node=%p", node);
        #ifdef AEX_EXPERIMENT
        t2 = std::chrono::high_resolution_clock::now();
        timer.allocate_inner_node_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        #endif
        return node;
    }
    
    inline data_node_ptr allocate_data_node(size_type slot_size, bool ml_node_flag=true){
        //AEX_FORMAT("ALLOCATE DATA NODE");
        #ifdef AEX_EXPERIMENT
        std::chrono::system_clock::time_point t1, t2;
        t1 = std::chrono::high_resolution_clock::now();
        #endif
        ++data_node_nums;
        size_type real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        slot_size = real_slot_size;

        this->_memory_used += sizeof(data_node) + KEY_MEMORY_USED(slot_size) + DATA_MEMORY_USED(slot_size);
        data_node_ptr node = new data_node();
        
        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif
        
        node->size = 0;
        node->slot_size = slot_size;

        node->level = 1;
        node->prev = node->next = nullptr;
        node->prop = node_property::LEAF;
        node->balance_stats = aex_node_balance_stats<typename traits::AllowBalance>();

        //node->key = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(data_node)));
        node->key = static_cast<key_type*>(malloc(KEY_MEMORY_USED(node->slot_size + 1)));

        // offset: metadata + key array
        node->data = static_cast<value_type*>(malloc(DATA_MEMORY_USED(node->slot_size + 1)));
        
        if (ml_node_flag == true && node->slot_size > traits::MIN_ML_DATA_NODE_SLOT_SIZE){
            node->prop |= node_property::ML_NODE;
        }

        #ifdef AEX_EXPERIMENT
        t2 = std::chrono::high_resolution_clock::now();
        timer.allocate_data_node_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        #endif
        return node;
    }

    inline void reallocate(inner_node_ptr node, slot_type new_slot_size){
        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) \
                              - PTR_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(new_slot_size) \
                              - BITMAP_MEMORY_USED(node->slot_size) + BITMAP_MEMORY_USED(new_slot_size);
        key_type *new_key_ptr = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
        node_ptr *new_child_ptr = static_cast<node_ptr*>(malloc(PTR_MEMORY_USED(new_slot_size)));
        bitmap new_bitmap_ptr = static_cast<bitmap>(malloc(BITMAP_MEMORY_USED(new_slot_size)));
        base_tree::copy_to_buffer(node, new_key_ptr, new_child_ptr);
        node->slot_size = new_slot_size;
        free(node->key_ptr);
        free(node->child_ptr);
        free(node->bitmap_ptr);
        node->key_ptr = new_key_ptr;
        node->child_ptr = new_child_ptr;
        node->bitmap_ptr = new_bitmap_ptr;
    }

    inline void reallocate(data_node_ptr node, slot_type new_slot_size){
        this->_memory_used += - KEY_MEMORY_USED(node->slot_size) + KEY_MEMORY_USED(new_slot_size) - \
                                        DATA_MEMORY_USED(node->slot_size) + DATA_MEMORY_USED(new_slot_size);
        key_type *new_key = static_cast<key_type*>(malloc(KEY_MEMORY_USED(new_slot_size)));
        value_type *new_data = static_cast<value_type*>(malloc(DATA_MEMORY_USED(new_slot_size)));
        node->slot_size = new_slot_size;
        std::copy(node->key, node->key + node->size, new_key);
        std::copy(node->data, node->data + node->size, new_data);
        free(node->key);
        free(node->data);
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
        _memory_used -= INNER_NODE_MEMORY_USED(p->slot_size);
        --inner_node_nums;
        if (p != nullptr)
            delete p;
    }

    inline void free_node(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        _memory_used -= INNER_NODE_MEMORY_USED(p->slot_size);
        --data_node_nums;
        if (p != nullptr){
            delete p;
        }
    }

    inline void free_node(node_ptr p){      
        if (p != nullptr){
            if (p->prop & node_property::LEAF) free_node(static_cast<data_node_ptr>(p));
            else free_node(static_cast<inner_node_ptr>(p));
        }
    }


#ifndef AEX_EXPERIMENT
private:
#endif
    size_type inner_node_nums, data_node_nums, free_cnt, alloc_cnt;

    unsigned char sta_buffer_flag, key_buffer_flag, nodeptr_buffer_flag;

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
