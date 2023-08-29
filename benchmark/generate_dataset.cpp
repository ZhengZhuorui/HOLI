#include <bits/stdc++.h>

#include "utils.h"
#include "generate_dataset.h"

const long long LONG_MAX_VALUE = (1LL << 62) - 1, LONG_MIN_VALUE = - ((1LL << 62) - 1);

/*
 * Required flags:
 * key_type
 * num_keys
 * distribution
 * output_files
 * 
 * Optional flags:
 * lower, upper(uniform)
 * mean, stddev(normal, lognormal)
 * 
 */

// Suggestion: output_files name: gen_ + _distribution(such as uniform) + _args(such as 1_1) + _key_type(such as int) + numkeys_(such as 1M).bin. 
// For example, gen_normal_int64_1M.bin

int main(int argc, char** argv){
    auto flags = parse_flags(argc, argv);
    auto output_files = flags["output_file"];
    auto key_type = flags["key_type"];
    auto distribution = flags["distribution"];
    long long num_keys = stoll(flags["num_keys"]);
    FILE* file = fopen(output_files.c_str(), "wb");
    //printf("%s\n", output_files.c_str());
    
    if (key_type == "int"){
        if (distribution == "uniform"){
            vector<long long> data;
            data.resize(num_keys);
            long long lower = LONG_MIN_VALUE, upper = LONG_MAX_VALUE;
            if (flags.find("lower") != flags.end()){
                lower = stoll(flags["lower"]);
            }
            if (flags.find("lower") != flags.end()){
                upper = stoll(flags["upper"]);
            }
            generate_unique_dataset<long long, std::uniform_int_distribution<long long>, long long>(data, num_keys, lower, upper);
            write_bineary_file(file, data);
        }
        else if (distribution == "normal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<long long> data;
            generate_normal_unique_dataset<long long>(data, num_keys, mean, stddev);
            write_bineary_file(file, data);
        }
        else if (distribution == "lognormal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<long long> data;
            generate_lognormal_unique_dataset<long long>(data, num_keys, mean, stddev);
            write_bineary_file(file, data);
        }
        else if (distribution == "id_ascend"){
            std::vector<long long> data(num_keys);
            for (long long i = 0; i < num_keys; ++i)
                data[i] = i;
            write_bineary_file(file, data);
        }
    }
    else if (key_type == "float"){
        if (distribution == "uniform"){
            vector<double> data;
            data.resize(num_keys);
            long long lower = LONG_MIN_VALUE, upper = LONG_MAX_VALUE;
            if (flags.find("lower") != flags.end()){
                lower = stoll(flags["lower"]);
            }
            if (flags.find("lower") != flags.end()){
                upper = stoll(flags["upper"]);
            }
            generate_unique_dataset<double, std::uniform_real_distribution<double>, double>(data, num_keys, lower, upper);
            int cnt = 0;
            for (int i = 0; i < 128; ++i)
                cnt += (data[i] > 0);
            std::cout << "data < 0 size=" << 128 - cnt << ", data > 0 size=" << cnt;
            write_bineary_file(file, data);
        }
        else if (distribution == "normal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<double> data;
            generate_normal_unique_dataset<double>(data, num_keys, mean, stddev);
            for (long long i = 0; i < std::min(100LL, num_keys); ++i)
                std::cout << data[i] << " ";
            std::cout << std::endl;
            write_bineary_file(file, data);
        }
        else if (distribution == "lognormal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<double> data;
            generate_lognormal_unique_dataset<double>(data, num_keys, mean, stddev);
            write_bineary_file(file, data);
        }
    }
    fclose(file);
}