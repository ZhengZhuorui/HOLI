#pragma once

#include "aex_def.h"

namespace aex
{


template<typename _Key, 
        typename _Val,
        typename traits>
class aex_allocator{
public:
    typedef _Key                                key_type;
    typedef _Val                                value_type;
    typedef aex_allocator<_Key, _Val, traits>   self;
    typedef aex_tree<_Key, _Val, traits>        base_tree;
    typedef typename base_tree::components      components;
    //typedef aex_default_components<traits>         components;
    typedef typename components::base_node      base_node;
    typedef typename components::node_ptr       node_ptr;
    typedef typename components::inner_node     inner_node;
    typedef typename components::inner_node_ptr inner_node_ptr;
    typedef typename components::hash_node      hash_node;
    typedef typename components::hash_node_ptr  hash_node_ptr;
    typedef typename components::dense_node     dense_node;
    typedef typename components::dense_node_ptr dense_node_ptr;
    typedef typename components::data_node      data_node;
    typedef typename components::data_node_ptr  data_node_ptr;
    typedef typename components::InnerNodeModel InnerNodeModel;
    typedef typename components::bitmap_impl    bitmap_impl;
    typedef typename components::HashTable      HashTable;
    typedef typename components::RWLock         RWLock;
    typedef typename components::Lock           Lock;
    typedef typename components::version_type   version_type;
    typedef typename components::ID_type        ID_type;

    typedef typename traits::slot_type slot_type;
    
    typedef typename traits::bitmap_base bitmap_base;
    typedef typename traits::bitmap bitmap;

    aex_allocator(){
        //if constexpr (traits::AllowConcurrency && traits::AllowMemoryPool){
        //    for (int i = 0; i < traits::MEMORY_POOL; ++i){
        //        data_node* pool_back = new data_node[traits::HASH_TABLE_BLOCK_SIZE];
        //        for (int i = 0; i < traits::HASH_TABLE_BLOCK_SIZE; ++i)
        //            data_node_pool[i].push_back(pool_back + i);
        //        //data_node_pool_his[i].push_back(pool_back);
        //    }
        //    for (int i = 0; i < traits::MEMORY_POOL; ++i){
        //        char* pool_back = malloc(MAX_INNER_NODE_SIZE() * traits::HASH_TABLE_BLOCK_SIZE);
        //        for (int i = 0; i < traits::HASH_TABLE_BLOCK_SIZE; ++i)
        //            inner_node_pool[i].push_back(pool_back + i * MAX_INNER_NODE_SIZE());
        //        //inner_node_pool_his[i].push_back(pool_back);
        //    }
        //}
    }

    ~aex_allocator(){
        /*if constexpr (traits::AllowConcurrency && traits::AllowMemoryPool){
            for (int i = 0; i < traits::MEMORY_POOL; ++i)
                while (!data_node_pool_his.empty()){
                    for (int i = 0; i < data_node_pool_his.size(); ++i)
                        delete[] data_node_pool_his[i];
                }

            for (int i = 0; i < traits::MEMORY_POOL; ++i)
                while (!inner_node_pool_his.empty()){
                    for (int i = 0; i < inner_node_pool_his.size(); ++i)
                        free(inner_node_pool_his[i]);
                }
            
        }*/
    }


    static constexpr ULL MAX_INNER_NODE_SIZE(){return std::max(sizeof(hash_node), sizeof(dense_node)) + traits::AllowConcurrency * 8;}

    // used memory size of key array, align 8 bytes
    inline static ULL KEY_MEMORY_USED(ULL slot_size){
        return align_8bytes((slot_size) * sizeof(key_type));
    }

    // used memory size of pointer array, align 8 bytes
    inline static ULL PTR_MEMORY_USED(ULL slot_size){
        return align_8bytes((slot_size) * sizeof(node_ptr));
    }

    // used memory size of bitmap, align 8 bytes
    inline static ULL BITMAP_MEMORY_USED(ULL slot_size){
        return align_8bytes(((slot_size >> 6) + 1) * sizeof(typename traits::bitmap_base));
    }

    // used memory size of data array, align 8 bytes
    inline static ULL DATA_MEMORY_USED(ULL slot_size){
        return align_8bytes((slot_size) * sizeof(value_type));
    }

    inline static ULL STATIC_DATA_NODE_MEMORY_USED(){
        return sizeof(data_node);
    }

    inline static ULL HASH_NODE_MEMORY_USED(ULL slot_size){
        return (slot_size / 64 + 1 > (slot_type)((MAX_INNER_NODE_SIZE() - sizeof(hash_node)) / 8)) * BITMAP_MEMORY_USED(slot_size) + MAX_INNER_NODE_SIZE();
    }

    inline static ULL DENSE_NODE_MEMORY_USED(){
        //return KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + MAX_INNER_NODE_SIZE();
        return MAX_INNER_NODE_SIZE();
    }

    inline hash_node_ptr allocate_hash_node(const slot_type slot_size, const ID_type id){
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        //AEX_WARNING(sizeof(aex_hash_node<key_type, value_type, traits>) << ", " << sizeof(aex_hash_node_con<key_type, value_type, traits>) << ", " << MAX_INNER_NODE_SIZE() << ", " << sizeof(base_node) << ", " << sizeof(InnerNodeModel) << ", " << sizeof(bitmap) << ", " << sizeof(slot_type) << ", " << sizeof(Lock));
        //exit(0);
        const hash_node_ptr node = h_n(malloc(MAX_INNER_NODE_SIZE()));
        //const hash_node_ptr node = h_n(allocate_inner_node());
        //const hash_node_ptr node = reinterpret_cast<hash_node_ptr>(allocate_inner_node());
        node->type = NodeType::HashNode;
        node->node_lock.init();
        node->slot_size = slot_size;
        node->id = id;
        node->init();
        if constexpr (traits::AllowConcurrency)
            node->copy = nullptr;
        return node;
    }

    inline dense_node_ptr allocate_dense_node(bool is_parent=false){
        const dense_node_ptr node = d_n(malloc(MAX_INNER_NODE_SIZE()));
        //const dense_node_ptr node = reinterpret_cast<dense_node_ptr>(allocate_inner_node());
        node->type = NodeType::DenseNode;
        node->node_lock.init();
        node->init();
        return node;
    }
private:
};

} // namespace name
;