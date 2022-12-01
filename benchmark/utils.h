#pragma once
#include <iostream>
#include <random>
#include <vector>
#include <utility>
#include <algorithm>
#include <map>
using std::vector;
using std::pair;
typedef unsigned long long size_type;
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
void read_bineary_file(FILE* file, vector<T> &data, size_t n){
    printf("read data...\n");
    size_t sz;
    fread(&sz, sizeof(size_t), 1, file);
    if (sz < n){
        printf("file key nums less than insert key");
        fflush(stdout);
        exit(0);
    }
    printf("data num_keys=%lu\n", sz);
    data.resize(n);
    fread(data.data(), sizeof(T), n, file);
}

template<typename T>
void write_bineary_file(FILE* file, vector<T> &data){
    size_t n = data.size();
    fwrite(&n, sizeof(size_t), 1, file);
    fwrite(data.data(), sizeof(T), n, file);
}
