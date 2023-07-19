#pragma once
#include <bits/stdc++.h>
#include "aex/aex_map.h"

#include "benchmark/generate_dataset.h"

#include "test/test_mock.hpp"

enum OperationType{
    Lookup=0,
    Insert=1,
    Erase=2,
};

//template<typename key_type,
//        typename value_type>
//struct Operation{
//    unsigned char op_type;
//    key_type key;
//    value_type value;
//};


#include "test/test_function.hpp"
#include "test/test_index.hpp"
#include "test/test_model.hpp"
#include "test/test_node.hpp"
#include "test/test_SMO_split.hpp"