#pragma once

class ThreadPool{
public:
    ThreadPool();
    ThreadPool(int threads);
    ~ThreadPool();
    
    typedef std::function<void()> task;
    typedef std::queue<task> tasks;
    void start();
    void stop();
    void add_task(const task&);
    void synchronize();

private:
    ThreadPool(const ThreadPool&);
    const ThreadPool& operator=(const ThreadPool&);
    void thread_loop();
    

    std::vector<std::thread*> m_threads;
    std::mutex m_mutex;
    std::mutex work_mutex;
    std::condition_variable cv;

    volatile int un_work_num;
    tasks m_task;
    volatile bool is_started;
    int nprocs;
};


ThreadPool::ThreadPool():m_mutex(),
                        is_started(false){
    this->nprocs = std::thread::hardware_concurrency();
    this->un_work_num = this->nprocs;
}

ThreadPool::ThreadPool(int threads):m_mutex(),
                        is_started(false){
    this->nprocs = threads;
    this->un_work_num = this->nprocs;
}

ThreadPool::~ThreadPool(){
    if (is_started == true)
        this->stop();
}
void ThreadPool::start(){
    this->is_started = true;
    for (int i = 0; i < this->nprocs; ++i){
        this->m_threads.push_back(new std::thread(std::bind(&ThreadPool::thread_loop, this)));
    }
}
void ThreadPool::synchronize(){
    std::unique_lock<std::mutex> lck(this->work_mutex);
    while (this->un_work_num != this->nprocs || this->m_task.empty() == false)
        this->cv.wait(lck);
    //std::cout << this->un_work_num << " " << this->m_task.size() << std::endl;
}

void ThreadPool::stop(){
    //std::lock_guard<std::mutex> lck(this->m_mutex);
    this->is_started = false;
    for (auto it : this->m_threads){
        if (it->joinable() == true)
            it->join();
        delete it;
    }
    this->m_threads.clear();
}
void ThreadPool::add_task(const task& t){
    std::lock_guard<std::mutex> lck(this->m_mutex);
    this->m_task.push(t);
}

void ThreadPool::thread_loop(){
    while (this->is_started == true){
        task t = NULL;
        this->m_mutex.lock();
        if (this->m_task.empty() == false){
            --this->un_work_num;
            t = this->m_task.front();
            this->m_task.pop();
        }
        this->m_mutex.unlock();

        if (t != NULL){
            t();
            this->m_mutex.lock();
            ++this->un_work_num;
            this->m_mutex.unlock();
            //printf("[thread_loop]:%d %d\n", this->un_work_num, this->m_task.size());
        }
        
        if (this->un_work_num == this->nprocs && this->m_task.empty() == true)
            this->cv.notify_one();
        

    }
    return;
}
