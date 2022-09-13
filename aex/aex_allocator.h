#pragma once

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

#ifdef AEX_DEBUG
static int alloc_cnt = 0;
static int free_cnt = 0;
#endif

template<typename _Key,
        typename _Val,
        typename traits>
class aex_allocator{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef aex_node_base<_Key, _Val, traits> node;
    
    typedef node* node_ptr;

    static memory_config _config;

    static char* data_node_pool;

    std::vector<char *> lt;


    static inline void* _allocate(size_t size){
        /* 
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_PRINT("alloc_cnt=" << alloc_cnt);
        #endif
        return static_cast<void*>(malloc(size));
    }

    static inline key_type* allocate_key_buffer(size_t size){
        /* 
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_PRINT("alloc_cnt=" << alloc_cnt);
        #endif
        return static_cast<key_type*>(malloc(size * sizeof(_Key)));
    }


    static inline node_ptr* allocate_nodeptr_buffer(size_t size){
        /* 
         *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_PRINT("alloc_cnt=" << alloc_cnt);
        #endif
        return static_cast<node_ptr*>(malloc(size * sizeof(node_ptr*)));
    }

    static inline void _free(void* p){
        /* 
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++free_cnt;
            AEX_PRINT("free_cnt=" << free_cnt << " pointer=" << p);
        #endif
        if (p != nullptr)
            free(p);
    }

};

template<typename _Key, 
        typename _Val,
        typename traits>
class aex_node_allocator{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef aex_allocator<_Key, _Val, traits> allocator;

    typedef aex_node_allocator<_Key, _Val, traits> self;

    typedef aex_node_base<key_type, value_type, traits> node;

    typedef node* node_ptr;

    typedef aex_inner_node<key_type, value_type, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model Model;

    typedef aex_data_node<key_type, value_type, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef typename traits::size_type size_type;

    typedef typename aex_bitmap_impl<traits>::bitmap bitmap;

    inline static inner_node_ptr allocate_inner_node(size_t slot_size, int level, bool ml_node_flag=true){
        /*
        *   TODO: memory pool
        */

        AEX_PRINT("ALLOCATE INNER NODE");
        size_t real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        inner_node_ptr node;

        if (real_slot_size > traits::MIN_ML_INNER_NODE_SLOT_SIZE && ml_node_flag){
            AEX_PRINT("ALLOCATE ML INNER NODE");
            real_slot_size += traits::ERROR_BOUND;
            size_t memory_used = inner_node::ML_INNER_NODE_MEMORY_USED(real_slot_size);
            
            node = static_cast<inner_node_ptr>(allocator::_allocate(memory_used));
            node->slot_size = real_slot_size;
            node->prop = node->size = 0;
            node->prop |= ML_NODE;
            node->level = level;
            //bm = node->bitmap_ptr();
            node->clear_bitmap();
            
            memset(node->key_ptr(), 0, inner_node::KEY_MEMORY_USED(real_slot_size));

        }
        else{
            size_t memory_used = inner_node::ML_INNER_NODE_MEMORY_USED(real_slot_size);
            AEX_PRINT("memory used=" << memory_used);
            node = static_cast<inner_node_ptr>(allocator::_allocate(memory_used));
            AEX_PRINT("node=" << node);
            node->prop = node->size = 0;
            node->level = level;
            node->slot_size = real_slot_size;
            memset(node->key_ptr(), 0, inner_node::KEY_MEMORY_USED(real_slot_size));
        }

        AEX_PRINT("node=" << node);
        
        return node;
    }
    
    inline static data_node_ptr allocate_data_node(){
        AEX_PRINT("ALLOCATE DATA NODE");
        data_node_ptr node = static_cast<data_node_ptr>(allocator::_allocate(sizeof(data_node)));
        node->size = 0;
        node->level = 1;
        node->prev = node->next = nullptr;
        node->prop = LEAF;
        AEX_PRINT("node=" << node);
        return node;
    }

    inline static void free(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        if (p != nullptr)
            allocator::_free(p);
    }

    inline static void free(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_PRINT("FREE NODE");
        if (p != nullptr)
            allocator::_free(p);
    }

    inline static void free(node_ptr p){
        if (p != nullptr){
            if (p->prop & LEAF) self::free(static_cast<data_node_ptr>(p));
            else self::_free(static_cast<inner_node_ptr>(p));
        }
    }

};

} // namespace name
