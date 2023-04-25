#pragma once
#include <bits/stdc++.h>
#include "aex/aex_map.h"
using namespace std;
// 1. all
template<typename K, typename V>
bool test_aex(std::pair<K, V>* data, size_t n);

// 2.function
template<typename T>
bool test_exponential_search_lower_bound(T* data, size_t n);

// 3. model
template<typename T>
bool test_linear_model(T* data, size_t n);

template<typename T>
bool test_exp_model(T* data, size_t n);

template<typename T>
bool test_log_model(T* data, size_t n);

template<typename T>
bool test_aex_model(T* data, size_t n);

// 4. operation
template<typename K, typename V>
bool test_aex_insert(std::pair<K, V>* value, size_t n){return false;}

template<typename K, typename V>
bool test_aex_bulk_load(std::pair<K, V>* value, size_t n){return false;}

template<typename K, typename V>
bool test_aex_find(std::pair<K, V>* value, size_t n){return false;}

template<typename K, typename V>
bool test_aex_erase(std::pair<K, V>* value, size_t n){return false;}

#include "test_function.hpp"
#include "test_index.hpp"
#include "test_model.hpp"