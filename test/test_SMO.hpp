#pragma once


template<typename key_type,
        typename value_type,
        typename traits=aex::aex_default_traits<key_type, value_type> >
bool test_get_childs_con(key_type* key, size_t num_keys){
    typedef aex::aex_tree<key_type, value_type, traits> Index;
    Index index;
    typedef typename Index::hash_node_ptr hash_node_ptr;
    typedef typename Index::node_ptr node_ptr;
    hash_node_ptr node = index.allocate_hash_node();
    std::vector<key_type> key_buf;
    std::vector<node_ptr> child_buf;

}