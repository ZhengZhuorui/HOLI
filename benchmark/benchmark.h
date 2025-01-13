#pragma once
#include <vector>
#include <utility>

#include "aex.h"
#include "pgm_index.hpp"
#include "pgm_index_dynamic.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "lipp.h"
#pragma GCC diagnostic pop


#include "stx/btree_map.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#include "alex_map.h"
#pragma GCC diagnostic pop

#include "benchmark_insert.hpp"
#include "benchmark_erase.hpp"
#include "benchmark_query.hpp"
#include "benchmark_delta_query.hpp"
#include "benchmark_build.hpp"
#include "benchmark_range_query.hpp"
#include "benchmark_other.hpp"
#include "benchmark_mix.hpp"
/*
template<typename key_type, typename value_type>
void aex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data);

template<typename key_type, typename value_type>
void stlmap_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data);

template<typename key_type, typename value_type>
void stx_btree_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data);

template<typename key_type, typename value_type>
void alex_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data);

template<typename key_type, typename value_type>
void pgm_insert_bench(vector<pair<key_type, value_type> > &data, vector<pair<key_type, value_type> > &insert_data);


template<typename key_type, typename value_type>
void aex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer);

template<typename key_type, typename value_type>
void stlmap_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer);

template<typename key_type, typename value_type>
void stx_btree_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer);

template<typename key_type, typename value_type>
void alex_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer);

template<typename key_type, typename value_type>
void pgm_query_bench(vector<pair<key_type, value_type> > &data, vector<key_type> &query, vector<value_type> &answer);
*/