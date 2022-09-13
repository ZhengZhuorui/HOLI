#include <bits/stdc++.h>

#include "utils.h"
#include "generate_dataset.h"

using namespace std;
const long long LONG_MAX_VALUE = (1LL << 62) - 1, LONG_MIN_VALUE = - ((1LL << 62) - 1);


int main(int argc, char** argv){
    auto flags = parse_flags(argc, argv);
    auto output_files = flags["output_files"];
    auto dataset = flags["dataset"];
    size_type num_keys = stoll(flags["num_keys"]);
    FILE* file = fopen(output_files.c_str(), "wb");
    if (dataset == "uniform_int"){
        vector<pair<long long, long long>> data;
        generate_uniform_unique_dataset<long long, long long>(data, num_keys, LONG_MIN_VALUE, LONG_MAX_VALUE);
        write_bineary_file(file, data);
    }
    else if (dataset == "normal_int"){

    }
    else if (dataset == "uniform_float"){

    }
    else if (dataset == "zipf_int"){

    }
    fclose(file);
}