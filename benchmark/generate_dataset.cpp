#include <bits/stdc++.h>
#include "utils.h"
#include "generate_dataset.h"


const long long LONG_MAX_VALUE = (1LL << 62) - 1, LONG_MIN_VALUE = - ((1LL << 62) - 1);
typedef unsigned long long ULL;

bool utils(std::map<std::string, std::string> flags){
    bool generate_prime = (flags.find("prime") != flags.end());
    if (generate_prime){
        long long key = stoll(flags["key"]);
        std::cout << "Max Prime < " << key << std::endl;
        for (long long i = key; i >= 0; --i)
            if (aex::is_prime(i)){
                std::cout << "prime=" << i << std::endl;
                break;
            }
        return true;
    }
    return false;
}

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
    
    if (utils(flags))
        return 0;

    auto output_file_name = flags["output_file"];
    auto key_type = flags["key_type"];
    auto distribution = flags["distribution"];
    long long num_keys = stoll(flags["num_keys"]);

    FILE* output_file = fopen(output_file_name.c_str(), "wb");
    FILE* input_file;
    std::string input_file_name;
    if (flags.find("input_file") != flags.end()){
        input_file_name = flags["input_file"];
        input_file = fopen(input_file_name.c_str(), "rb");
    }

    bool generate_real_dataset = (flags.find("real") != flags.end());
    
    //printf("%s\n", output_files.c_str());
    if (generate_real_dataset){
        bool is_head = !((input_file_name.find(".bin.data") != std::string::npos) || (input_file_name.find("generate_data") != std::string::npos));
        std::vector<ULL> bin_data;
        read_bineary_file(input_file, bin_data, num_keys, is_head);
        auto workload = flags["workload"];
        //ULL lookup_size = ;
        if (workload == "lookup"){
            
        }
        else if (workload == "insert"){

        }
        else if (workload == "erase"){

        }
        else if (workload == "mix"){

        }
        fclose(input_file);
    }
    
    if (key_type == "uint64"){
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
            write_bineary_file(output_file, data);
        }
        else if (distribution == "normal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<long long> data;
            generate_normal_unique_dataset<long long>(data, num_keys, mean, stddev);
            write_bineary_file(output_file, data);
        }
        else if (distribution == "lognormal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<long long> data;
            generate_lognormal_unique_dataset<long long>(data, num_keys, mean, stddev);
            write_bineary_file(output_file, data);
        }
        else if (distribution == "id_ascend"){
            std::vector<long long> data(num_keys);
            for (long long i = 0; i < num_keys; ++i)
                data[i] = i;
            write_bineary_file(output_file, data);
        }
        else if (distribution == "multikey"){
            std::vector<long long> data(num_keys);
            for (long long i = 0; i < num_keys; ){
                long long j = std::min(num_keys, i + rand()%65536);
                long long x = rand();
                while (i < j) data[i++] = x;
            }
            write_bineary_file(output_file, data);
        }
    }
    else if (key_type == "float64"){
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
            write_bineary_file(output_file, data);
        }
        else if (distribution == "normal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<double> data;
            generate_normal_unique_dataset<double>(data, num_keys, mean, stddev);
            for (long long i = 0; i < std::min(100LL, num_keys); ++i)
                std::cout << data[i] << " ";
            std::cout << std::endl;
            write_bineary_file(output_file, data);
        }
        else if (distribution == "lognormal"){
            double mean = stod(flags["mean"]);
            double stddev = stod(flags["stddev"]);
            vector<double> data;
            generate_lognormal_unique_dataset<double>(data, num_keys, mean, stddev);
            write_bineary_file(output_file, data);
        }
    }
    
    fclose(output_file);
}