#pragma once
#include "aex/aex_def.h"
namespace aex{

template<typename _Key, 
        typename _Val,
        typename traits>
class aex_node_allocator_con : public aex_node_allocator<_Key, _Val, traits>{
public:
    typedef aex_node_allocator<_Key, _Val, traits> base_allocator;

    typedef aex_tree<_Key, _Val, traits> base_tree;

    typedef typename base_tree::inner_node inner_node;

    typedef typename base_tree::data_node data_node;
    
    typedef typename base_tree::inner_node_ptr inner_node_ptr;

    typedef typename base_tree::data_node_ptr data_node_ptr;

    inline static size_type MUTEX_MEMORY_USED(size_type slot_size){
        return ceil(1.0 * slot_size / traits::NODE_MUTEX_SLOT_SIZE) * sizeof(std::shared_mutex);
    }

    inline static size_type INNER_NODE_MEMORY_USED(size_type slot_size){
        return BITMAP_MEMORY_USED(slot_size) + KEY_MEMORY_USED(slot_size) + PTR_MEMORY_USED(slot_size) + \
                +MUTEX_MEMORY_USED(slot_size) + align_8bytes(sizeof(inner_node_con));
    }


    inline inner_node_ptr allocate_inner_node(slot_type real_slot_size, bool ml_node_flag){
        this->foreach_node_free();
        #ifdef AEX_EXPERIMENT
        ++inner_node_nums;
        #endif

        slot_type slot_size = real_slot_size;

        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);

        slot_size += traits::ERROR_BOUND;

        size_type memory_used = INNER_NODE_MEMORY_USED(slot_size);
        this->_memory_used += memory_used;
        inner_node_ptr node = new inner_node(slot_size);
        //node->real_slot_size = real_slot_size;

        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        if (ml_node_flag){
            SET_FLAG(node, node_property::ML_NODE);
            node->clear_bitmap();
        }
        return node;
    };

    inline data_node_ptr allocate_data_node(){
        this->foreach_node_free();
        #ifdef AEX_EXPERIMENT
        ++data_node_nums;
        #endif
        this->_memory_used += STATIC_DATA_NODE_MEMORY_USED();
        data_node_ptr node = new data_node();
        SET_FLAG(node, node_property::LEAF);
        SET_FLAG(node, node_property::STATIC_NODE);
        #ifdef AEX_EXPERIMENT
        node_id[static_cast<node_ptr>(node)] = max_node_id;
        id_node.push_back(node);
        ++max_node_id;
        #endif

        return node;
    };

    inline void free_node(inner_node_ptr p){
        AEX_ASSERT(p != nullptr);
        _memory_used -= INNER_NODE_MEMORY_USED(p->slot_size) + MUTEX_MEMORY_USED(p->slot_size);
        #ifdef AEX_EXPERIMENT
        --data_node_nums;
        #endif
        inner_node_que.push(p);
    }

    inline void free_node(static_data_node_ptr p){
        AEX_ASSERT(p != nullptr);
        _memory_used -= STATIC_DATA_NODE_MEMORY_USED();
        #ifdef AEX_EXPERIMENT
        --data_node_nums;
        #endif
        data_node_que.push(p);
    }

    inline void foreach_node_free(){
        for (int i = data_node_que.size() - 1; i >= 0; --i){
            if (inner_node_que[i]->get_cnt() == 0){
                std::swap(inner_node_que[i], inner_node_que[inner_node_que.size() - 1]);
                delete inner_node_que[inner_node_que.size() - 1];
                inner_node_que.pop_back();
            }
        }
        for (int i = data_node_que.size() - 1; i >= 0; --i){
            if (data_node_que[i]->get_cnt() == 0){
                std::swap(data_node_que[i], data_node_que[data_node_que.size() - 1]);
                delete data_node_que[data_node_que.size() - 1];
                data_node_que.pop_back();
            }
        }
    }

    inline void free_node(node_ptr p){      
        if (p != nullptr){
            if (IS_LEAF_NODE(p)) {
                if constexpr (std::is_same<data_node, static_data_node>::value) 
                    free_node(static_cast<static_data_node_ptr>(p));
            else free_node(static_cast<inner_node_ptr>(p));
        }
        this->foreach_node_free();
    }

private:
    std::vector<inner_node_ptr> inner_node_que;
    std::vector<data_node_ptr> data_node_que;

}
};

}