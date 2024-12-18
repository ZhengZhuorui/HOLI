#pragma once 
#include <thread>
typedef aex::aex_rw_spinlock<aex::aex_default_traits<ULL, ULL>, true> RWLock;

bool test_rw_lock(){
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
    
    RWLock lk;
    bool res;
    lk.lock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    lk.unlock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());

    lk.lock_shared();
    lk.lock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    lk.unlock_shared();
    res = lk.try_upgrade_lock();
    AEX_SUCCESS("res=" << res << ", lk.lockCount=" << lk.lockCount.load());
    lk.downgrade_lock();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    lk.unlock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    
    //
    lk.lock_shared();
    lk.lock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    res = lk.try_upgrade_lock_without_read();
    AEX_SUCCESS("res=" << res << ", lk.lockCount=" << lk.lockCount.load());
    lk.unlock();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());
    lk.unlock_shared();
    AEX_SUCCESS("lk.lockCount=" << lk.lockCount.load());

    return true;
}