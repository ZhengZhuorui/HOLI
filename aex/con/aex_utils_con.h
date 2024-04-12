namespace aex{

enum update_operator{
    UPDATE_DATA_NODE = 1,
    UPDATE_INNER_NODE_META = 2,
    UPDATE_INNER_NODE_SEGMENT = 3,
    UPDATE_INNER_NODE = 4
};
template<typename _Key, 
        typename _Val, 
        typename traits=aex_default_traits<_Key, _Val>>
struct aex_update_log_unit{
    update_operator op;
    node_ptr origin_node;

    struct META_MSG{
        slot_type size;
        Model &M;
    };

    union update_message
    {
        data_node_ptr new_data_node;
        META_MSG msg;
        inner_node_ptr new_inner_node;
    };

    void update(){
        switch (op)
        {
        case UPDATE_DATA_NODE:
            *static_cast<data_node_ptr>(origin_node) = node;
            break;
        case UPDATE_INNER_NODE_META:
            origin_node->size = size;
            break;
        case UPDATE_INNER_NODE_SEGMENT:
            for (size_type i = start; i < )
            break;
        default:
            break;
        }
    }
    void clear(){

    }
};

template<typename _Key, 
        typename _Val, 
        typename traits=aex_default_traits<_Key, _Val>>
class aex_update_log{
public:
    aex_update_log(){}
    ~aex_update_log(){
        for (int i = 0; i < nums; ++i)
            log_unit.clear();
    }
    inline void work(){
        for (int i = 0; i < nums; ++i){
            log_unit[i].work();
        }
    }
    
    
private:
    aex_update_log_unit log_unit[traits::MAX_DEPTH];
    int nums;
};

}