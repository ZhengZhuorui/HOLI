#pragma once

template<typename _Key,
        typename _Val, 
        typename traits=aex::aex_default_traits<_Key, _Val> >
class mock_aex_tree : public aex::aex_tree<_Key, _Val, traits>{
public:
};