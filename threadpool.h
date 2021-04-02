// bool done = false;
// mutex mutex_;
// std::condition_variable cv;
// void worker_thread(){

//     std::unique_lock< std::mutex > lk( mutex_ );
//     while (!done)
//     {
//         if( task_available )
//              run_task();
//         else
//             cv.wait( lk );

//     }
// }
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

class threadpool{
    std::vector<std::atomic<bool>> worked;
    std::vector<std::unique_ptr<std::thread>> threads;
    int csize = 0;
    bool running = true;
    std::function<void(int)> func;
    int m_num_threads = 1;
    int grain;
    std::atomic<int> counter;
    void work();
public:
    threadpool();
    threadpool(int numThreads);
    ~threadpool();
    // static void worker_thread(threadpool* tp, int id);
    friend void worker_thread(threadpool* tp, int id);

    void doWork(int size, std::function<void(int)> f, int grain);
};