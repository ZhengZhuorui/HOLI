#pragma once
#include <atomic>

namespace aex{
class aex_spinlock{
public:
    aex_spinlock():locked(ATOMIC_FLAG_INIT){}
    aex_spinlock(const aex_spinlock&) = delete;
    void lock(){
        //uint8_t unlocked = 0, locked = 1;
        while (!locked.test_and_set()){}
    }
    void unlock(){
        //this->locked = 0;
        locked.clear();
    }
    std::atomic_flag locked;
};

class aex_read_write_lock{
public:
aex_read_write_lock() = default;
aex_read_write_lock(const aex_read_write_lock&) = delete;

void lock_reader(){
    while(true){
        int c = counter.load(std::memory_order_relaxed);
        if (c == -1){
            //pause_cpu();
            continue;
        }
        while (c != -1){
            if (counter.compare_exchange_strong(c, c + 1, std::memory_order_acquire))
                return;
        }
    }

}
void unlock_reader(){
    counter.fetch_sub(1, std::memory_order_release);
}

void lock_writer(){
    while (true){
        while (counter.load(std::memory_order_relaxed) != 0){
            //pause_cpu();
        }
        int c = 0;
        if (counter.compare_exchange_strong(c, -1, std::memory_order_acquire))
            break;
    }
}

void unlock_writer(){
    counter.exchange(0, std::memory_order_release);
}
private:
    std::atomic_int counter{0};
};

}