#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
void test_lookup_con_unit(aex_tree<key_type, value_type, traits> &index, std::pair<key_type, value_type> &kv, long long id){
    value_type res;
    bool flag = index.find(kv.first, res);
    if (flag == false && res != std::numeric_limits<value_type>::max()){
        AEX_ERROR("id=" << id << ", query error. query no exists");
        AEX_ERROR("lookup error!");
        index.print_stats();
        return;
    }

    if (res != kv.second){
        AEX_ERROR("id=" << id << ", query error, answer=" << kv.second << ", get res=" << res);
        return;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
void test_insert_con_unit(aex_tree<key_type, value_type, traits> &index, std::pair<key_type, value_type> &kv, long long id){
    bool _ = index.insert(kv.first, kv.second);
    if (_ == false){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first);
        return;
    }
    value_type res;
    bool tmp = index.find(kv.first, res);
    if (tmp == false){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first << " not found");
        return;
    }
    if (res != kv.second){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first << ", value=" << kv.second << " not right, get=" << res);
        return;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type, false, void, true>>
void test_erase_con_unit(aex_tree<key_type, value_type, traits> &index, key_type &x, long long id){
    bool _ = index.erase(x);
    if (_ == false){
        AEX_ERROR("i=" << id << "erase error!");
        return;
    }
    value_type res;
    _ = index.find(x, res);
    if (_ == true){
        AEX_ERROR("Error! index can find data after erasing!");
        return;
    }
    return;
}