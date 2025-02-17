#pragma once

template<typename _Tp,
        typename traits>
bool test_bitmap(long long n, long long batch){
    aex_tree<_Tp, _Tp, traits> tree;
    typedef typename aex_tree<_Tp, _Tp, traits>::hash_node hash_node;
    typedef typename aex_tree<_Tp, _Tp, traits>::hash_node_ptr hash_node_ptr;
    typedef typename aex_tree<_Tp, _Tp, traits>::Allocator Allocator;
    Allocator allocator;
    srand(time(0));
    unsigned char* x = new unsigned char[n];
    for (int i = 0; i < n; ++i) 
        x[i] = (rand() % 32) == 0;
    hash_node_ptr node = allocator.allocate_hash_node(n);
    for (slot_type i = 0; i = n; ++i)
        if (x[i])
            bitmap_impl::set_one(node->bitmap_ptr, i);


    /**
    inline void array_lock(const slot_type l_pos, const slot_type r_pos){}
    inline void array_unlock(const slot_type l_pos, const slot_type r_pos){}
    inline void array_lock_shared(const slot_type l_pos, const slot_type r_pos){}
    inline void array_unlock_shared(const slot_type l_pos, const slot_type r_pos){}
    inline bool try_array_upgrade_lock(const slot_type l_pos, const slot_type r_pos, bool &restart){return true;}
    inline void array_downgrade_lock(const slot_type l_pos, const slot_type r_pos){}
    inline slot_type lock_shared_until_next_item(const slot_type pos){return next_item(pos);}
    inline slot_type lock_until_next_item(const slot_type prev_pos, const slot_type pos){return next_item(pos);}
    inline slot_type try_array_lock_shared_until_prev_item(const slot_type next_pos, const slot_type pos, bool &restart){return prev_item(pos);}
    */
}