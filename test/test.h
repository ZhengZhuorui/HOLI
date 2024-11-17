#pragma once
#include <bits/stdc++.h>
#include "aex/aex.h"

#include "benchmark/generate_dataset.h"

enum OperationType{
    Lookup=0,
    Insert=1,
    Erase=2,
};

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type>>
void construct_data_node_array(key_type* key, size_t num_keys, typename aex_tree<key_type, value_type, traits>::data_node_ptr* child_buf){
    typedef typename aex_tree<key_type, value_type, traits>::data_node data_node;
    [[maybe_unused]]typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;

    for (size_t i = 0; i < num_keys; ++i){
        //child_buf[i] = static_cast<node_ptr>(tree.allocator.allocate_data_node());
        //child_buf[i] = reinterpret_cast<node_ptr>(new data_node(tree.version));
        child_buf[i] = new data_node(0);
        child_buf[i]->key[0] = key[i];
        child_buf[i]->size = 1;
    }
}

#include "test/test_avx.hpp"
#include "test/test_function.hpp"
#include "test/test_model.hpp"
#include "test/test_node.hpp"
#include "test/test_SMO_split.hpp"
#include "test/test_SMO_rescale.hpp"


#include "test/test_index.hpp"
#include "test/test_index_mix.hpp"

// muthi thread
#include "thread_pool.hpp"
#include "test/test_con_unit.hpp"
#include "test/test_index_con.hpp"