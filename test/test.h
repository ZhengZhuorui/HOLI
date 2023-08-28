#pragma once
#include <bits/stdc++.h>
#include "aex/aex_map.h"

#include "benchmark/generate_dataset.h"

#include "test/test_traits.h"
#include "test/test_mock.hpp"

enum OperationType{
    Lookup=0,
    Insert=1,
    Erase=2,
};

template<typename key_type,
        typename value_type,
        typename node_ptr,
        typename traits=aex::aex_default_traits<key_type, value_type>>
void construct_data_node_array(key_type* key, size_t num_keys, node_ptr* child_buf){
    mock_aex_tree<key_type, value_type> tree;
    typedef typename mock_aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::size_type size_type;

    for (size_type i = 0; i < num_keys; ++i){
        child_buf[i] = tree.node_allocator.allocate_data_node(traits::MIN_DATA_NODE_SLOT_SIZE, false);
        static_cast<data_node_ptr>(child_buf[i])->key[0] = key[i];
        child_buf[i]->size = 1;
    }
}

#include "test/test_function.hpp"
#include "test/test_model.hpp"
#include "test/test_node.hpp"
#include "test/test_SMO_split.hpp"

#include "test/test_index.hpp"
#include "test/test_index_mix.hpp"