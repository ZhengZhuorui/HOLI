#pragma once
#include <iostream>
#include <random>
#include <vector>
#include <utility>
#include <algorithm>
#include <map>
using std::vector;
using std::pair;

static const unsigned int seed = 0; 

std::map<std::string, std::string> parse_flags(int argc, char** argv) {
    std::map<std::string, std::string> flags;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        size_t equals = arg.find("=");
        size_t dash = arg.find("--");
        if (dash != 0) {
            std::cout << "Bad flag '" << argv[i] << "'. Expected --key=value"
                    << std::endl;
            continue;
        }
        std::string key = arg.substr(2, equals - 2);
        std::string val;
        if (equals == std::string::npos) {
            val = "";
            std::cout << "found flag " << key << std::endl;
        } else {
            val = arg.substr(equals + 1);
            std::cout << "found flag " << key << " = " << val << std::endl;
        }
        flags[key] = val;
    }
    return flags;
}

template<typename T>
size_t read_bineary_file(FILE* file, vector<T> &data, size_t n, bool is_head=true){
    std::cout << "read data...\n";
    data.resize(n);
    [[maybe_unused]] size_t _, __;
    if (is_head){
        __ = fread(&_, sizeof(size_t), 1, file);
        std::cout << "Dataset size=" << _ << std::endl;
    }
    size_t data_size = fread(data.data(), sizeof(T), n, file);
    for (int i = 0; i < std::min(100, (int)n); ++i)
        std::cout << data[i] << ", ";
    std::cout << "\nread data end\n";
    return data_size;
}

template<typename T>
size_t read_bineary_file(FILE* file, T* &data, size_t n, bool is_head=true){
    std::cout << "read data...\n" ;
    size_t _;
    if (is_head){
        fread(&_, sizeof(size_t), 1, file);
        std::cout << "Dataset size=" << _ << std::endl;
    }
    size_t data_size = fread(data, sizeof(T), n, file);
    for (int i = 0; i < std::min(100, (int)n); ++i)
        std::cout << data[i] << ", ";
    std::cout << "\nread data end\n";
    return data_size;
}

template<typename T>
void write_bineary_file(FILE* file, vector<T> &data){
    std::cout << "write data...\n";
    size_t n = data.size();
    std::cout << "example: " << std::endl;
    for (size_t i = 0; i < std::min(n, (size_t)100); ++i)
        std::cout << data[i] << ", ";
    fwrite(data.data(), sizeof(T), n, file);
    std::cout << "write data end\n";
}

template<typename T>
void write_bineary_file(FILE* file, vector<T> &data, size_t n, bool is_head=true){
    std::cout << "write data...\n";

    std::cout << "example: " << std::endl;
    for (size_t i = 0; i < std::min(n, (size_t)100); ++i)
        std::cout << data[i] << ", ";
    if (is_head){
        std::cout << "num_keys=" << n << std::endl;
        fwrite(&n, sizeof(size_t), 1, file);
    }
    fwrite(data.data(), sizeof(T), n, file);
    std::cout << "write data end\n";
}

template<typename T>
void change_small2big_endian(vector<T> &data, size_t n){
    int sz = sizeof(T);
    char buf[sizeof(T)], buf1[sizeof(T)];
    for (size_t i = 0; i < n; ++i){
        memcpy(buf1, data.data() + i, sizeof(T));
        for (int j = 0; j < sz; ++j)
            buf[j] = buf1[sz - j - 1];
        data[i] = *(reinterpret_cast<T*>(buf));
    }
}
