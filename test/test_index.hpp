#include "test/test.h"

template<typename K, typename V>
bool test_aex(std::pair<K, V>* data, size_t n){
    aex::aex_map<K , V> mp;
    random_shuffle(data, data + n);
    // insert
    for (size_t i = 0; i < n; ++i){
        #ifdef AEX_DEBUG
            if (false) mp.set_debug_level(1);
            else mp.set_debug_level(0);
        #endif 
        mp.insert(std::make_pair(data[i].first, data[i].second));
    }

    // find
    int M = std::min(n, (size_t)100);
    for (int i = 0; i < M; ++i){
        size_t x = rand() % n;
        auto y = mp.find(data[x].first);
        if (y.data() != data[x].second){
            printf("Error!");
        }
    }

    // erase
    random_shuffle(data, data + n);

    for (int i = 0; i < M; ++i){
        mp.erase(data[i].first);
    }

    //bulk load
    {
        std::sort(data, data + n);
        aex::aex_map<K, V> mp;
        mp.bulk_load(data, n);
        typename aex::aex_map<K, V>::stats st = mp.get_stats();
        printf("inner node=%lu, data node=%lu size=%lu height=%lu", st.inner_node, st.data_node, st.size, st.height);

        // find
        for (int i = 0; i < M; ++i){
            size_t x = rand() % n;
            auto y = mp.find(data[x].first);
            if (y.data() != data[x].second){
                printf("Error!");
            }
        }

        // erase
        random_shuffle(data, data + n);
        for (int i = 0; i < M; ++i){
            mp.erase(data[i].first);
        }
    }

    return true;
}
