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


//template<typename _Key,
//        typename _Val,
//        typename traits>
//class aex_allocator{
//public:
//
//    typedef _Key key_type;
//
//    typedef _Val value_type;
//
//    typedef aex_node_base<_Key, _Val, traits> node;
//    
//    typedef node* node_ptr;
//
//    //memory_config _config;
//
//    std::vector<char*> lt;
//
//    inline void* _allocate(size_t size){
//        /* 
//        *   TODO: memory pool
//        */
//        #ifdef AEX_DEBUG
//            ++alloc_cnt;
//            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
//        #endif
//        return static_cast<void*>(malloc(size));
//    }
//
//    inline key_type* allocate_key_buffer(size_t size){
//        /* 
//        *   TODO: memory pool
//        */
//        #ifdef AEX_DEBUG
//            ++alloc_cnt;
//            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
//        #endif
//        return static_cast<key_type*>(malloc(size * sizeof(_Key)));
//    }
//
//
//    inline node_ptr* allocate_nodeptr_buffer(size_t size){
//        /* 
//         *   TODO: memory pool
//        */
//        #ifdef AEX_DEBUG
//            ++alloc_cnt;
//            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
//        #endif
//        return static_cast<node_ptr*>(malloc(size * sizeof(node_ptr*)));
//    }
//
//    inline void _free(void* p){
//        /* 
//        *   TODO: memory pool
//        */
//        #ifdef AEX_DEBUG
//            ++free_cnt;
//            AEX_FORMAT("free_cnt=%d, pointer=%p", free_cnt, p);
//        #endif
//        if (p != nullptr)
//            free(p);
//    }
//};
//

template<typename _Key, 
        typename _Val,
        typename traits>
class aex_node_allocator{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    //typedef aex_allocator<_Key, _Val, traits> allocator;

    typedef aex_node_allocator<_Key, _Val, traits> self;

    typedef aex_node_base<key_type, value_type, traits> base_node;

    typedef base_node* node_ptr;

    typedef aex_inner_node<key_type, value_type, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model Model;

    typedef aex_data_node<key_type, value_type, traits> data_node;

    typedef data_node* data_node_ptr;

    typedef typename traits::size_type size_type;

    typedef typename traits::version_type version_type;

    typedef typename aex_bitmap_impl<traits>::bitmap bitmap;

    /*

    static std::vector<data_node_ptr> data_node_pool[traits::MEMORY_POOL_LEVEL];

    static std::vector<inner_node_ptr> inner_node_pool[traits::MEMORY_POOL_LEVEL];

    inline static void init(){
        for (size_type )

        for (size_type slot_size = traits::MIN_DATA_NODE_SLOT_SIZE, j = 0; ) {
            allocate_data_node_pool(slot_size);


        for (size_type slot_size = traits::MIN_INNER_NODE_SLOT_SIZE, j = 0; j < 2; i <<= 1){
            size_type one_node_memory = INNER_NODE_MEMORY_USED();
        }
            
    }

    inline static void allocate_data_node_pool(size_type slot_size){
        if (slot_size / traits::MIN_DATA_NODE_SLOT_SIZE > (1 << traits::MEMORY_POOL_LEVEL)){
            size_type memory_used = DATA_MEMORY_USED(slot_size);
            data_node_ptr* pool = allocator::_allocate(traits::MEMORY_POOL_NODE_SIZE);
            for (size_type i = 0; i < traits::MEMORY_POOL_NODE_SIZE; ++i)
                

        }
    }

    inline static data_node_ptr allocate_data_node_from_pool(size_type slot_size){
        if (slot_size / traits::MIN_DATA_NODE_SLOT_SIZE > (1 << traits::MEMORY_POOL_LEVEL)){
            return allocator::_allocate(DATA_MEMORY_USED(slot_size));
        }
        else{
            size_type level = 0, p = slot_size / traits::MIN_DATA_NODE_SLOT_SIZE;
            while (p > 1){
                ++level;
                p >>= 1;
            }
            if (data_node_pool[p].empty())
                allocate_data_node_pool(p);
            data_node_ptr node = data_node_pool[p].back();
            data_node_pool[p].pop_back();
            return node;
        }
    }
    */

   inline void* _allocate(size_type size){
        /* 
        *   TODO: memory pool
        */
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
        #endif
        return static_cast<void*>(malloc(size));
    }

   inline key_type* allocate_key_buffer(size_type size){
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
        #endif
        return static_cast<key_type*>(malloc(size * sizeof(_Key)));
    }


    inline node_ptr* allocate_nodeptr_buffer(size_type size){
        #ifdef AEX_DEBUG
            ++alloc_cnt;
            AEX_FORMAT("alloc_cnt=%d", alloc_cnt);
        #endif
        return static_cast<node_ptr*>(malloc(size * sizeof(node_ptr*)));
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
    inline size_type DATA_MEMORY_USED(size_type slot_size){
        return align_8bytes((slot_size) * sizeof(value_type));
    }
    
    inline static size_type INNER_NODE_MEMORY_USED(size_type slot_size){ 
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + \
        // + (traits::AllowMultiThread) * (MUTEX_MEMORY_USED(real_slot_size) + VERSION_MEMORY_USED(real_slot_size)) + 
        align_8bytes(sizeof(inner_node));
    }

    inline static size_type DATA_NODE_MEMORY_USED(size_type slot_size){
        return align_8bytes(sizeof(data_node)) + KEY_MEMORY_USED(slot_size) + DATA_MEMORY_USED(slot_size);
    }

    inline static size_type MUTEX_MEMORY_USED(size_type slot_size){
        return align_8bytes(sizeof(aex_spinlock) * slot_size / traits::ERROR_BOUND);
    }

    inline size_type VERSION_MEMORY_USED(size_type slot_size){
        return align_8bytes(sizeof(version_type) * slot_size / traits::ERROR_BOUND);
    }

    inline inner_node_ptr allocate_inner_node(size_type slot_size, bool ml_node_flag=true){
        /*
        *   TODO: memory pool
        */
       
        AEX_FORMAT("ALLOCATE INNER NODE");
        size_type real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        
        real_slot_size += traits::ERROR_BOUND;
        size_type memory_used = INNER_NODE_MEMORY_USED(real_slot_size);

        //if (slot_size / traits::MIN_INNER_NODE_SLOT_SIZE)

        inner_node_ptr node = static_cast<inner_node_ptr>(this->_allocate(memory_used));

        node->slot_size = real_slot_size;
        node->prop = node->size = node->base_stats.write_times = node->base_stats.train_times = 0;
        node->m_stats.rewired_cnt = static_cast<size_type>(traits::INIT_REWIRED_CNT * log(slot_size));

        // offset: meta data
        node->key_ptr = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)));

        // offset: meta data + key array
        node->child_ptr = reinterpret_cast<node_ptr*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)) + KEY_MEMORY_USED(node->slot_size));

        // offset: meta data + key array + pointer array
        node->bitmap_ptr = reinterpret_cast<bitmap>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(inner_node)) + 
                            KEY_MEMORY_USED(node->slot_size) + PTR_MEMORY_USED(node->slot_size));
                        
        memset(node->key_ptr, 0, node->slot_size);

        if (real_slot_size > traits::MIN_ML_INNER_NODE_SLOT_SIZE && ml_node_flag){
            node->prop |= node_property::ML_NODE;
            node->clear_bitmap();
        }

        AEX_FORMAT("node=%p", node);
        
        return node;
    }
    
    inline data_node_ptr allocate_data_node(size_t slot_size, bool ml_node_flag=true){
        AEX_FORMAT("ALLOCATE DATA NODE");
        ++data_node_nums;
        size_type real_slot_size = traits::MIN_INNER_NODE_SLOT_SIZE;
        while (real_slot_size < slot_size) real_slot_size <<= 1;
        slot_size = real_slot_size;
        
        size_type memory_used = DATA_MEMORY_USED(slot_size);
        data_node_ptr node = static_cast<data_node_ptr>(this->_allocate(memory_used));
        //data_node_ptr node = request_data_node(slot_size);

        // offset: metadata
        node->key = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(data_node)));

        // offset: metadata + key array
        node->data = reinterpret_cast<key_type*>(align_8bytes(reinterpret_cast<size_t>(node) + sizeof(data_node)) + KEY_MEMORY_USED(node->slot_size));
        
        node->size = 0;
        node->slot_size = slot_size;
        node->level = 1;
        node->prev = node->next = nullptr;
        node->prop = node_property::LEAF;
        node->base_stats.write_times = node->base_stats.train_times = 0;
        
        if (ml_node_flag == true && node->slot_size > traits::MIN_ML_DATA_NODE_SLOT_SIZE){
            node->prop |= node_property::ML_NODE;
            //if (node->slot_size > traits::MIN_COMPLEX_ML_DATA_NODE_SLOT_SIZE)
            //    node->prop |= COMPLEX_MODEL;
        }

        //node->model.complex_model = nullptr;
        
        AEX_FORMAT("node=%p", node);
        return node;
    }

    inline void deallocate(void* p){
        ++free_cnt;
        if (p != nullptr)
            free(p);
    }

    inline void free_node(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
       AEX_FORMAT("FREE INNER NODE");
       --inner_node_nums;
        if (p != nullptr)
            free(p);
    }

    inline void free_node(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        AEX_FORMAT("FREE DATA NODE");
        --data_node_nums;
        if (p != nullptr){
            p->free();
            free(p);
        }
    }

    inline void free_node(node_ptr p){      
        if (p != nullptr){
            if (p->prop & node_property::LEAF) free_node(static_cast<data_node_ptr>(p));
            else free_node(static_cast<inner_node_ptr>(p));
        }
    }

private:
    unsigned long long inner_node_nums, data_node_nums, free_cnt, alloc_cnt;


};

} // namespace name
