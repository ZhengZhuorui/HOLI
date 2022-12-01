#include <bits/stdc++.h>

#include "benchmark.h"
#include "utils.h"
#include "generate_dataset.h"

using namespace std;
/*
 * Required flags:
 * generate_dataset
 * input_files
 * key_type
 * num_keys
 * index
 * Optional flags:
 * batch_size
 *
 */
int main(int argc, char** argv){
    //auto flags = parse_flags(argc, argv);
    auto flags = parse_flags(argc, argv);
    auto key_type = flags["key_type"];
    auto input_files = flags["input_files"];
    FILE *file = fopen(input_files.c_str(), "rb");
    auto dataset = flags["dataset"];
    auto func = flags["func"];
    long long num_keys = stoll(flags["num_keys"]);
    auto index_name = flags["index"];
    long long batch_size = 10000000;
    if (flags.find("batch_size") != flags.end()) 
        batch_size = stoll(flags["batch_size"]);

    if (key_type == "int"){
        vector<pair<long long, long long> > data;
        // query
        if (func == "query"){
            read_bineary_file<pair<long long, long long>>(file, data, num_keys);
            vector<long long> query;
            vector<long long> answer;
            //generate_uniform_unique_dataset<long long, long long>(data, query, answer, num_keys, batch_size, LONG_MIN_VALUE, LONG_MAX_VALUE);

            generate_query(data, query, answer, batch_size, num_keys);
            if (index_name == "aex"){
                aex_query_bench(data, query, answer);
            }
            else if (index_name == "stl_map"){
                stlmap_query_bench(data, query, answer);
            }
            else if (index_name == "stx_btree"){
                stx_btree_query_bench(data, query, answer);
            }
            else if (index_name == "alex"){
                alex_query_bench(data, query, answer);
            }
            else if (index_name == "pgm"){
                pgm_query_bench(data, query, answer);
            }
        }
        else if (func == "insert"){
            read_bineary_file<pair<long long, long long>>(file, data, num_keys + batch_size);
            vector<pair<long long, long long> > insert_data;
            //read_bineary_file<pair<long long, long long>>(file, data, batch_size);
            insert_data.resize(batch_size);
            memcpy(insert_data.data(), data.data() + num_keys, batch_size * sizeof(pair<long long, long long>));
            if (index_name == "aex"){
                aex_insert_bench(data, insert_data);
            }
            else if (index_name == "stl_map"){
                stlmap_insert_bench(data, insert_data);
            }
            else if (index_name == "stx_btree"){
                stx_btree_insert_bench(data, insert_data);
            }
            else if (index_name == "alex"){
                alex_insert_bench(data, insert_data);
            }
            else if (index_name == "pgm"){
                pgm_insert_bench(data, insert_data);
            }
        }
    }
    else if (key_type == "float"){
        
    }

}