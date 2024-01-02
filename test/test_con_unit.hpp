#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_lookup_con_unit(aex::aex_tree_con<key_type, value_type, traits> &index, std::pair<key_type, value_type> &kv, int id){
    if (index.size() < 100)
        continue;
    value_type res;
    bool flag = index.find(kv.first, res);
    if (flag == false && v != std::numeric_limit<value_type>::max()){
        AEX_ERROR("id=" << id << ", query error. query no exists");
        AEX_ERROR("lookup error!");
        index.print_stats();
        index.print_detail();
        return false;
    }

    if (res.first != index_data[pos].first || x.data() != index_data[pos].second){
        AEX_ERROR("i=" << i << ", query error, query key=" << index_data[pos].first << ", data=" << index_data[pos].second << ", get key=" << x.key() << ", data=" << x.data());
        return false;
    } 

}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_insert_con_unit(aex::aex_tree_con<key_type, value_type, traits> &index, std::pair<key_type, value_type> &kv, int id){
    bool _ = index.insert(kv.first, kv.second);
    if (_ == false){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first);
        return false;
    }
    value_type res
    bool tmp = index.find(kv.first, res);
    if (tmp == false){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first << " not found");
        return false;
    }
    if (res != kv.second){
        AEX_ERROR("id=" << id << ", insert error, insert_key=" << kv.first << ", value=" << kv.second << " not right, get=" << res);
        return false;
    }
}

template<typename key_type,
        typename value_type,
        typename traits=aex_default_traits<key_type, value_type>>
bool test_erase_con_unit(aex::aex_tree_con<key_type, value_type, traits> &index, key_type &x, int id){
    bool _ = index.erase(x);
    if (_ == 0){
        AEX_ERROR("i=" << i << "erase error!");
        return false;
    }
    return true;
}