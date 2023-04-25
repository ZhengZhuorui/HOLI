#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>
#include <utility>

#include "aex/aex_map.h"
#include "test.h"
#include "utils.h"
#include "generate_dataset.h"

using namespace std;
typedef long long LL;
const int N = 10000000, M = 10000;

/*
 * Required flags:
 * unit ("index", "function", "model")
 * key_type: int or float
 * input_file
 * optional flags:
 * func: (if unit==function)
 * 
 */

int main(int argc, char** argv){
    srand(0);
    auto flags = parse_flags(argc, argv);
    auto type = flags["unit"];
    auto dataset = flags["dataset"];
    auto key_type = flags["key_type"];
    string file_name = flags["input_file"];
    FILE* file = fopen(file_name.c_str(), "rb");
    long long num_keys = stoll(flags["num_keys"]);
    if (key_type == "int"){
        vector<long long> bin_data;
        read_bineary_file<long long>(file, bin_data, num_keys);
        if (type == "index"){
            vector<std::pair<long long, long long>> data;
            pack_KV_dataset<long long, long long>(bin_data, data);
            test_aex(data.data(), data.size());
        }
        else if (type == "function"){
            auto func = flags["function"];
            if (func == "exp_find")
                test_exponential_search_lower_bound(bin_data.data(), num_keys);
        }
        else if (type == "model"){
            auto model_type = flags["model_type"];
            if (model_type == "linear")
                test_linear_model(bin_data.data(), num_keys);
            else if (model_type == "exp")
                test_exp_model(bin_data.data(), num_keys);
            else if (model_type == "log")
                test_log_model(bin_data.data(), num_keys);
            else if (model_type == "all")
                test_aex_model(bin_data.data(), num_keys);
        }
    }
    else if (key_type == "float"){
        vector<double> bin_data;
        read_bineary_file<double>(file, bin_data, num_keys);
        if (type == "index"){
            vector<std::pair<double, double>> data;
            pack_KV_dataset<double, double>(bin_data, data);
            test_aex(data.data(), data.size());
        }
        else if (type == "function"){
            auto func = flags["function"];
            if (func == "exp_find")
                test_exponential_search_lower_bound(bin_data.data(), num_keys);
        }
        else if (type == "model"){
            auto model_type = flags["model_type"];
            if (model_type == "linear")
                test_linear_model(bin_data.data(), num_keys);
            else if (model_type == "exp")
                test_exp_model(bin_data.data(), num_keys);
            else if (model_type == "log")
                test_log_model(bin_data.data(), num_keys);
            else if (model_type == "all")
                test_aex_model(bin_data.data(), num_keys);
        }
    }

    vector<pair<LL, LL> > data;

    for (int i = 0; i < N; ++i) data.emplace_back(i, rand());

    cout << "test finish" << endl;
    
}