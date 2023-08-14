#pragma once

using namespace aex;

template<typename _Key,
        typename _Val, 
        typename traits=aex::aex_default_traits<_Key, _Val, std::false_type> >
class mock_aex_tree : public aex::aex_tree<_Key, _Val, traits>{
public:

    // type traits
    typedef typename traits::key_type key_type;

    typedef typename traits::value_type value_type;

    typedef typename traits::used_as_set used_as_set;

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef typename traits::version_type version_type;

    // iterator:
    typedef typename aex::aex_iterator<_Key, _Val, traits> iterator;

    typedef typename aex::aex_const_iterator<_Key, _Val, traits> const_iterator;

    typedef typename aex::aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;

    typedef typename aex::aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    typedef typename aex::aex_node_base<_Key, _Val, traits> base_node;

    typedef base_node* node_ptr;

    // inner_node:    
    typedef typename aex::aex_inner_node<_Key, _Val, traits> inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model inner_node_model;

    // data_node:
    typedef typename aex::aex_data_node<_Key, _Val, traits> data_node;

    typedef data_node* data_node_ptr;
    
    typedef typename data_node::Model data_node_model;

    // bitmap:
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::bitmap bitmap;

    void print_detail(){
        msg.level_node_nums.resize(traits::MAX_DEPTH);
        dfs_detail(this->root);
    }
    
    void dfs_detail(node_ptr node){
        bool flag = true;
        key_type first_key;
        ++msg.level_node_nums[node->level];

        if (IS_LEAF_NODE(node)){
            return;
        }
        else{
            inner_node_ptr in = static_cast<inner_node_ptr>(node);
            key_type* node_key = in->key_ptr;
            node_ptr* node_child = in->child_ptr;
            if (IS_ML_NODE(node)){
                bitmap bm = in->bitmap_ptr;
                for (size_type i = 0; i <= in->slot_size; ++i){
                    if (bitmap_impl::at(bm, i)){
                        auto res = dfs_detail(node_child[i]);
                    }
                }
            }
            else{
                for (size_type i = 0; i < in->size; ++i){
                    auto res = dfs_detail(node_child[i]);
                }
            }
        } 
    }
    
    struct detail_msg{
        vector<size_type> level_node_nums;
        size_type inner_node, data_node;
    }msg;

    std::pair<key_type, bool> debug(node_ptr node){
        bool flag = true;
        key_type first_key;
        if (IS_LEAF_NODE(node)){
            ++msg.data_node;
            data_node_ptr dn = static_cast<data_node_ptr>(node);
            first_key = dn->key[0];
            for (size_type i = 0; i < dn->size; ++i){
                if (i > 0 && dn->key[i] < dn->key[i - 1]){
                    AEX_PRINT("Error! node[" << i-1 << "]=" << dn->key[i - 1] << " node[" << i << "]=" << dn->key[i]);
                    flag = false;
                }
            }
        }
        else{
            ++msg.inner_node;
            inner_node_ptr in = static_cast<inner_node_ptr>(node);
            key_type* node_key = in->key_ptr;
            node_ptr* node_child = in->child_ptr;
            if (IS_ML_NODE(node)){
                size_type cnt = 0;
                bitmap bm = in->bitmap_ptr;
                first_key = node_key[0];
                for (size_type i = 0; i < node->slot_size; ++i){
                    // check if the key is larger than prev position key
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    if (bitmap_impl::at(bm, i)){
                        ++cnt;
                        auto res = debug(node_child[i]);
                        flag &= res.second;
                        // check the child last key is equal to the node key
                        //if (node_key[i] != res.first){
                        //    AEX_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                        //    flag = false;
                        //}
                        // check if the key position is smaller than predict position
                        slot_type pos = in->predict(node_key[i]);
                        if (i < pos || i - pos >= traits::ERROR_BOUND){
                            AEX_PRINT("pos=" << i << " predict=" << pos);
                            flag = false;
                        }
                    }
                    
                }
            }
            else{
                first_key = node_key[0];
                for (size_type i = 0; i < in->size; ++i){
                    // check if the key is larger than prev position key 
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    
                    auto res = _debug(node_child[i]);
                    if (node_key[i] != res.first){
                        flag = false;
                        AEX_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                    }
                    flag &= res.second; 
                    //AEX_ASSERT(i < node->predict(node_key[i]));
                }
            }
        }
        return std::make_pair(first_key, flag);
    }

    bool debug_error(){
        std::pair<key_type, bool> res = (this->root == nullptr)? std::make_pair(0LL, true) : debug(this->root);
        size_type cnt = 0;
        bool flag = res.second;
        key_type prev_key;
        for (iterator it = this->begin(); it != this->end(); ++it){
            if (cnt > 0){
                /* check item is ordered */
                if (it.key() < prev_key){
                    flag = false;
                }
            }
            else prev_key = it.key();
            ++cnt;
        }
        /* check the data item is correct */
        if (cnt != this->m_stats.size){
            flag = false;
            AEX_PRINT("Error!");
        }
        return flag;
    }
};

//template<typename _Key,
//        typename _Val, 
//        typename traits=aex::aex_default_traits<_Key, _Val, std::false_type> >
//class mock_aex_inner_node_with_linear_model{
//
//}
//
//template<typename _Key,
//        typename _Val, 
//        typename traits=aex::aex_default_traits<_Key, _Val, std::false_type> >
//class mock_aex_tree_inner_node_with_linear_model : public mock_aex_tree<_Key, _Val, traits>{
//    typedef mock_aex_inner_node_with_linear_model<_Key, _Val, traits> ;
//    typedef inner_node* inner_node_ptr;
//}