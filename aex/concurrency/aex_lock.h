#pragma once

#include <atomic>
#include <array>
#include <immintrin.h>
#include <sched.h>

namespace aex {
template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_spinlock{
    aex_spinlock(){}
    ~aex_spinlock(){}
    void init(){}
    inline void lock(){}
    inline void unlock(){}
    inline bool is_lock(){return true;}
    inline bool try_lock(){return true;}
    //inline bool is_lock(){return false;}
};

template<typename traits>
struct aex_spinlock<traits, true>{
    typedef aex_spinlock<traits, true> self;
    aex_spinlock() : writeLock(false) {}
    void init(){writeLock=false;}
    ~aex_spinlock(){}
    inline void lock() {
        bool expected = false;
        while (!writeLock.compare_exchange_weak(expected, true)) {
            _mm_pause();
            expected = false;
        }
    }
    inline void unlock() {
        writeLock.store(false);
    }

    inline bool try_lock(){
      bool expected = false;
      return writeLock.compare_exchange_strong(expected, true);
    }
    inline bool is_lock(){return writeLock.load();}
    //inline bool is_lock(){return false;}

    std::atomic<bool> writeLock;
};

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_rw_spinlock{
    aex_rw_spinlock(){}
    void init(){}
    inline void lock() const {}
    inline void unlock() const {}
    inline void lock_shared() const {}
    inline void unlock_shared() const {}
    inline void upgrade_lock() const {}
    inline void downgrade_lock() const {}
    inline bool try_lock() const {return true;}
    inline bool try_lock_shared() const {return true;}
    inline bool try_upgrade_lock() const {return true;}
    inline bool try_upgrade_lock_without_read() const {return true;}
    inline bool is_lock() const {return false;}
    inline bool is_lock_shared() const {return false;}
};

template<typename traits>
struct aex_rw_spinlock<traits, true>{
    typedef aex_rw_spinlock<traits, true> self;
    aex_rw_spinlock() : lockCount(0) {}
    void init(){lockCount = 0;}
    aex_rw_spinlock(const self &x) = delete;
    aex_rw_spinlock(const self &&x) = delete;
    ~aex_rw_spinlock(){}
    self& operator = (const self &x) = delete;
    self& operator = (const self &&x) = delete;
    void lock_shared() {
        unsigned int expected = lockCount.load() & (~1);
        unsigned int result   = expected + 0b10;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            _mm_pause();
            expected = lockCount.load() & (~1);
            result   = expected + 0b10;
        }
    }

    void unlock_shared() {
        AEX_ASSERT(is_lock_shared());
        lockCount.fetch_sub(0b10);
    }
    
    bool try_lock_shared(){
        unsigned int expected = lockCount.load() & (~1);
        unsigned int result   = expected + 0b10;
        if (!lockCount.compare_exchange_strong(expected, result)) {
            _mm_pause();
            return false;
        }
        return true;
    }

    void lock() {
        unsigned int expected = lockCount.load() & (~1);
        unsigned int result   = expected | 1;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            _mm_pause();
            expected = lockCount.load() & (~1);
            result   = expected | 1;
        }
        while (lockCount.load() >= 0b10);
    }

    void unlock() {
        AEX_ASSERT(is_lock());
        lockCount.fetch_sub(1);
    }

    bool try_lock(){
        unsigned int expected = lockCount.load() & (~1);
        unsigned int result   = expected | 1;
        if (!lockCount.compare_exchange_strong(expected, result)) {
            _mm_pause();
            return false;
        }
        while (lockCount.load() >= 0b10);
        return true;
    }

    bool try_upgrade_lock_without_read(){
        //unsigned int expected = lockCount.load() & (~1);
        //unsigned int result   = expected - 1;
        unsigned int expected = 0b10;
        unsigned int result   = 0b1;
        if (!lockCount.compare_exchange_strong(expected, result)) 
            return false;
        return true;
    }
    
    bool try_upgrade_lock(){
        unsigned int expected = lockCount.load() & (~1);
        unsigned int result   = expected - 1;
        if (!lockCount.compare_exchange_strong(expected, result)) {
            _mm_pause();
            return false;
        }
        while (lockCount.load() >= 0b10);
        return true;
    }

    // unused. may deadlock
    [[deprecated]] void upgrade_lock(){
        unsigned int expected = 0b10;
        unsigned int result   = 0b1;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            expected = lockCount.load() & (~1);
            result   = expected - 1;
        }
    }

    void downgrade_lock(){
        AEX_ASSERT(is_lock());
        lockCount.fetch_add(1);
    }

    inline bool is_lock() const {return (lockCount.load() & 1) == 1;}
    inline bool is_lock_shared() const {return lockCount.load() >= 0b10;}
    std::atomic<unsigned int> lockCount;
};

// optimistic lock implementation is based on https://github.com/wangziqi2016/index-microbench/blob/master/BTreeOLC/BTreeOLC_child_layout.h
template<typename traits, bool _ = traits::AllowConcurrency>
struct OptLock {

    OptLock() = default;
    OptLock(const OptLock& other) {}

    inline void init(){}
    inline uint64_t get_version_number(){return 0;}
    inline bool isLocked(uint64_t version) {return false;}
    inline bool isLocked() { return false; }
    inline void writeLock(){}
    inline void writeLockOrRestart(bool &needRestart) {}
    inline void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {}
    inline void writeUnlock() {}
    inline uint64_t downgradeLock(){ return 0; }
    inline void checkOrRestart(uint64_t startRead, bool &needRestart) const { }
    inline bool checkOrRestart(uint64_t startRead) const { return false; }
    inline uint64_t readLockOrRestart(bool &needRestart) { return 0; }
    inline void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const { }
    inline bool readUnlockOrRestart(uint64_t startRead) const { return false; }
    inline void writeUnlockObsolete() { }
    inline void labelObsolete() {  }
    inline bool isObsolete(uint64_t version) { return false; }
    inline bool isObsolete() { return false; }

};


template<typename traits>
struct OptLock<traits, true> {
    std::atomic<uint64_t> typeVersionLockObsolete{0b100};

    OptLock() = default;
    OptLock(const OptLock& other) {
      typeVersionLockObsolete = 0b100;
    }
    inline void init(){typeVersionLockObsolete = 0b100;}

    inline uint64_t get_version_number()
    {
      return typeVersionLockObsolete.load();
    }

    inline bool isLocked(uint64_t version) {
      return ((version & 0b10) == 0b10);
    }

    inline bool isLocked() {
      return ((typeVersionLockObsolete.load() & 0b10) == 0b10);
    }

    inline void writeLock(){
      uint64_t version = this->typeVersionLockObsolete.load() & (~0b10);
      while (!typeVersionLockObsolete.compare_exchange_strong(version, version + 0b10)){
        version = this->typeVersionLockObsolete.load() & (~0b10);
        _mm_pause();
      }
    }

    inline void writeLockOrRestart(bool &needRestart) {
      uint64_t version;
      version = readLockOrRestart(needRestart);
      if (needRestart) return;

      upgradeToWriteLockOrRestart(version, needRestart);
    }

    inline void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
      if (typeVersionLockObsolete.compare_exchange_strong(version, version + 0b10)) {
        version = version + 0b10;
      } else {
        _mm_pause();
        needRestart = true;
      }
    }

    inline void writeUnlock() {
      typeVersionLockObsolete.fetch_add(0b10);
    }

    inline uint64_t downgradeLock(){
      uint64_t version = typeVersionLockObsolete.load() + 0b10;
      writeUnlock();
      return version;
    }

    inline void checkOrRestart(uint64_t startRead, bool &needRestart) const {
      readUnlockOrRestart(startRead, needRestart);
    }
    inline bool checkOrRestart(uint64_t startRead) const{
      return readUnlockOrRestart(startRead);
    }

    inline uint64_t readLockOrRestart(bool &needRestart) {
      uint64_t version;
      version = typeVersionLockObsolete.load();
      //if (isLocked(version) || isObsolete(version)) {
      if (isLocked(version)) {
        _mm_pause();
        needRestart = true;
      }
      return version;
    }

    inline void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
      needRestart = (startRead != typeVersionLockObsolete.load());
    }

    inline bool readUnlockOrRestart(uint64_t startRead) const {
      return startRead != typeVersionLockObsolete.load();
    }

    inline void writeUnlockObsolete() {
      typeVersionLockObsolete.fetch_add(0b11);
    }

    inline void labelObsolete() {
      typeVersionLockObsolete.store((typeVersionLockObsolete.load() | 1));
    }

    inline bool isObsolete(uint64_t version) {
      return (version & 1) == 1;
    }

    inline bool isObsolete() {
      return (typeVersionLockObsolete.load() & 1) == 1;
    }

};
  // typedef tbb::spin_rw_mutex Alex_rw_mutex;
  // typedef OptLock Alex_mutex;
  // typedef Alex_rw_mutex::scoped_lock Alex_rw_lock;
  // typedef tbb::spin_mutex Alex_strict_mutex;
  // typedef Alex_strict_mutex::scoped_lock Alex_strict_lock;

}
