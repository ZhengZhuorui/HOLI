#pragma once

template<typename key_type>
bool test_lower_bound_with_error_bound_avx(){
    AEX_HINT("test lower bound with error bound using AVX2 instruction");
    key_type data[10];
    for (int i = 0; i < 10; ++i)
        data[i] = 1.0 * rand();
    std::sort(data, data + 10);
    for (int i = 0; i <= 8; ++i){
        int res = aex::lower_bound_with_error_bound<key_type, 8>(data, data + 10, data[i]) - data;
        if (res != i){
            AEX_ERROR("real pos=" << i << ", get pos=" << res);
            return false;
        }
    }
    AEX_SUCCESS("test success!");
    return true;
}