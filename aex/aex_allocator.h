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

    // used memory size of key array, align 8 bytes
    inline static size_type KEY_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(key_type));
    }

    // used memory size of pointer array, align 8 bytes
    inline static size_type PTR_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(char*));
    }

    // used memory size of bitmap, align 8 bytes
    inline static size_t BITMAP_MEMORY_USED(size_type slot_size){
        return align_8bytes(((slot_size >> 6) + ((slot_size & 63) > 0)) * sizeof(unsigned long long));
    }

    // used memory size of data array, align 8 bytes
    inline static size_type DATA_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(value_type));
    }
    
    inline static size_type INNER_NODE_MEMORY_USED(size_type slot_size){ 
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) +
        align_8bytes(sizeof(inner_node)) ;
    }

    inline static size_type DATA_NODE_MEMORY_USED(size_type slot_size){
        return align_8bytes(sizeof(data_node)) + KEY_MEMORY_USED(slot_size) + DATA_MEMORY_USED(slot_size);
    }

    //enum {node_pool_num = 20;}
    //enum {inner_node_level = 5;}

    //union _obj{
    //    union _obj* _next;
    //    char data[1];
    //};
    //static _obj* inner_node_free_list[inner_node_level];
    //static _obj* data_node_free_list;


    //template <_Tp>
    //inline static _obj* _M_refill(){
    //    _obj* node_pool = (_obj*)malloc(node_pool_num * align_8bytes(sizeof(_Tp)));
    //    _obj* ret = node_pool;
    //    for (int i = 0; i < node_pool_num; ++i){
    //        node_pool->next = reinterpret_cast<_obj*>(reinterpret_cast<char*>(node_pool) + align_8bytes(sizeof(_Tp)));
    //    }
    //    return ret;
    //}

    inline static inner_node_ptr allocate_inner_node(size_t slot_size, bool ml_node_flag=true){
        /*
        *   TODO: memory pool
        */
       
        AEX_PRINT("ALLOCATE INNER NODE");
        size_t real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        
        real_slot_size += traits::ERROR_BOUND;
        size_t memory_used = INNER_NODE_MEMORY_USED(real_slot_size + traits::ERROR_BOUND);   
        inner_node_ptr node = static_cast<inner_node_ptr>(allocator::_allocate(memory_used));
        node->slot_size = real_slot_size;
        node->prop = node->size = 0;

        // offset: meta data
        node->key_ptr = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)));

        // offset: meta data + key array
        node->child_ptr = reinterpret_cast<node_ptr*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)) + KEY_MEMORY_USED(node->slot_size));

        // offset: meta data + key array + pointer array
        node->bitmap_ptr = reinterpret_cast<bitmap>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)) + 
                            KEY_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(node->slot_size));
                        
        memset(node->key_ptr, 0, node->slot_size);

        if (real_slot_size > traits::MIN_ML_NODE_SLOT_SIZE && ml_node_flag){
            node->prop |= ML_NODE;
            node->clear_bitmap();
        }

        AEX_PRINT("node=" << node);
        
        return node;
    }
    
    inline static data_node_ptr allocate_data_node(size_t slot_size, bool ml_node_flag=true, bool complex_model=false){
        AEX_PRINT("ALLOCATE DATA NODE");
        ++data_node_nums;
        //data_node_ptr node;
        //if (data_node_free_list == nullptr){
        //    data_node_free_list = _M_refill();
        //}
        
        //node = reinterpret_cast<data_node_ptr>(data_node_free_list);
        //data_node_free_list = data_node_free_list->_next;
        size_type memory_used = DATA_MEMORY_USED(slot_size);   
        data_node_ptr node = static_cast<data_node_ptr>(allocator::_allocate(memory_used));

        // offset: metadata
        node->key = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(data_node)));

        // offset: metadata + key array
        node->data = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(data_node)) + KEY_MEMORY_USED(node->slot_size));

        // offset:
        
        node->size = 0;
        node->slot_size = slot_size;
        node->level = 1;
        node->prev = node->next = nullptr;
        node->prop = LEAF;
        node->prop |= ML_NODE;
        if (node->slot_size > traits::MIN_COMPLEX_ML_DATA_NODE_SLOT_SIZE)
            node->prop |= COMPLEX_MODEL;

        node->model.complex_model = nullptr;
        
        AEX_PRINT("node=" << node);
        return node;
    }

    inline static void free(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
       --inner_node_nums;
        if (p != nullptr)
            allocator::_free(p);
    }

    inline static void free(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_PRINT("FREE NODE");
        --data_node_nums;
        if (p != nullptr)
            allocator::_free(p);
    }

    inline static void free(node_ptr p){      
        if (p != nullptr){
            if (p->prop & LEAF) self::free(static_cast<data_node_ptr>(p));
            else self::free(static_cast<inner_node_ptr>(p));
        }
    }

private:
    static unsigned long long inner_node_nums, data_node_nums;


};

} // namespace name
