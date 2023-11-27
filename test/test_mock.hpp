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

    typedef typename traits::size_type size_type;

    typedef typename traits::slot_type slot_type;

    typedef typename traits::version_type version_type;

    typedef aex::aex_tree<_Key, _Val, traits> parent;

    // iterator:
    typedef typename aex::aex_iterator<_Key, _Val, traits> iterator;

    typedef typename aex::aex_const_iterator<_Key, _Val, traits> const_iterator;

    typedef typename aex::aex_reverse_iterator<_Key, _Val, traits> reverse_iterator;

    typedef typename aex::aex_const_reverse_iterator<_Key, _Val, traits> const_reverse_iterator;

    typedef typename aex::aex_node_base<_Key, _Val, traits> base_node;

    typedef base_node* node_ptr;

    // inner_node:    
    typedef typename parent::inner_node inner_node;

    typedef inner_node* inner_node_ptr;

    typedef typename inner_node::Model inner_node_model;

    // data_node:
    typedef typename parent::dynamic_data_node dynamic_data_node;
    typedef typename parent::static_data_node static_data_node;
    typedef typename parent::dynamic_data_node_ptr dynamic_data_node_ptr;
    typedef typename parent::static_data_node_ptr static_data_node_ptr;

    typedef typename parent::data_node data_node;

    typedef data_node* data_node_ptr;
    
    typedef typename data_node::Model data_node_model;

    // bitmap:
    typedef typename aex::aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::bitmap bitmap;

    void print_detail(){
        msg = detail_msg();
        msg.level_node_nums.resize(traits::MAX_DEPTH);
        dfs_detail(this->root);
        AEX_HINT("inner node number=" << msg.inner_node << ", data node number=" << msg.data_node << 
                ", ml inner node number=" << msg.ml_inner_node << ", ml data node number=" << msg.ml_data_node <<
                ", ml inner node ratio=" << 1.0 * msg.ml_inner_node / msg.inner_node);
        AEX_HINT("data size=" << msg.data_size);
        AEX_HINT("avg seg nums=" << 1.0 * msg.tot_seg_nums / msg.ml_inner_node);
    }
    
    void dfs_detail(node_ptr node){
        if (node == nullptr) return;
        if (IS_LEAF_NODE(node)){
            ++msg.level_node_nums[0];
            msg.data_node++;
            msg.ml_data_node += IS_ML_NODE(node);
            msg.data_size += node->size;
            return;
        }
        else{
            msg.inner_node++;
            msg.ml_inner_node += IS_ML_NODE(node);
            inner_node_ptr in = static_cast<inner_node_ptr>(node);
            ++msg.level_node_nums[in->level];
            node_ptr* node_child = in->child_ptr;
            if (IS_ML_NODE(node)){
                bitmap bm = in->bitmap_ptr;
                //msg.tot_seg_nums += in->model.args.seg_nums;
                for (slot_type i = 0; i <= in->slot_size; ++i){
                    if (bitmap_impl::at(bm, i)){
                        dfs_detail(node_child[i]);
                    }
                }
                dfs_detail(node_child[in->slot_size - 1]);
            }
            else{
                for (slot_type i = 0; i < in->size; ++i){
                    dfs_detail(node_child[i]);
                }
            }
        } 
    }
    
    struct detail_msg{
        detail_msg():level_node_nums(0), inner_node(0), data_node(0), ml_inner_node(0), ml_data_node(0), tot_seg_nums(0), data_size(0){}
        vector<size_type> level_node_nums;
        size_type inner_node, data_node;
        size_type ml_inner_node, ml_data_node;
        size_type tot_seg_nums, data_size;
    }msg;

    std::pair<key_type, bool> debug(node_ptr node){
        bool flag = true;
        key_type max_key = std::numeric_limits<key_type>::lowest();
        if (IS_LEAF_NODE(node)){
            ++msg.data_node;
            data_node_ptr dn = static_cast<data_node_ptr>(node);
            max_key = dn->key[dn->size - 1];
            for (slot_type i = 0; i < dn->size; ++i){
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
                for (slot_type i = 0; i < in->slot_size; ++i){
                    // check if the key is larger than prev position key
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_ERROR("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    if (bitmap_impl::at(bm, i)){
                        ++cnt;
                        auto res = debug(node_child[i]);
                        AEX_ASSERT(max_key < res.first);
                        //max_key = std::max(max_key, res.first);
                        max_key = res.first;
                        flag &= res.second;
                        if (node_key[i] < max_key){
                            AEX_ERROR("i=" << i << "node key=" << node_key[i] << ", max_key=" << max_key);
                        }
                        
                        // check if the key position is smaller than predict position
                        slot_type pos = in->predict(node_key[i]);
                        if (i < pos || i - pos >= traits::ERROR_BOUND){
                            AEX_ERROR("pos=" << i << " predict=" << pos);
                            flag = false;
                        }
                    }
                }
                auto res = debug(node_child[in->slot_size - 1]);
                AEX_ASSERT(max_key < res.first);
                max_key = res.first;
                flag &= res.second;
            }
            else{
                for (slot_type i = 0; i < in->size; ++i){
                    // check if the key is larger than prev position key 
                    if (i > 0 && node_key[i] < node_key[i - 1]){
                        AEX_ERROR("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                        flag = false;
                    }
                    
                    auto res = debug(node_child[i]);
                    if (max_key > res.first){
                        AEX_ERROR("max_key=" << max_key << ", res.first=" << res.first);
                    }
                    AEX_ASSERT(max_key < res.first);
                    if (node_key[i] < res.first){
                        flag = false;
                        AEX_ERROR("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                    }
                    max_key = res.first;
                    flag &= res.second; 
                    //AEX_ASSERT(i < node->predict(node_key[i]));
                }
            }
        }
        return std::make_pair(max_key, flag);
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