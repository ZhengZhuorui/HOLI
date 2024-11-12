#pragma once

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_data_node_merge_with_perf(key_type* key, size_t num_keys){
    AEX_HINT("[test SMO--data split ]");
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;   
    construct_data_node_array(key);
    tree.merge_nodes();
    
}

template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_SMO_inner_node_merge_with_perf(key_type* key, value_type* data, size_t num_keys){
    AEX_HINT("[test SMO--data split ]");
    typedef typename aex_tree<key_type, value_type, traits>::node_ptr node_ptr;
    [[maybe_unused]] typedef typename aex_tree<key_type, value_type, traits>::inner_node_ptr inner_node_ptr;
    typedef typename aex_tree<key_type, value_type, traits>::data_node_ptr data_node_ptr;
    typedef typename traits::slot_type slot_type;   
    construct_data_node_array();
    tree.merge_nodes();
}