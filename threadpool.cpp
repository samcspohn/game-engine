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
#include "threadpool.h"

void threadpool::work()
{
    while (counter < csize)
    {
        int start = counter.fetch_add(grain);
        int end = start + grain;
        if (end > csize)
            end = csize;
        for (int i = start; i < end; i++)
        {
            func(i);
        }
    }
}

void worker_thread(threadpool *tp, int id)
{
    // using namespace std;
    // this_thread::sleep_for(100ms);
    while (tp->running)
    {
        if (!tp->worked[id])
        {
            tp->work();
            tp->worked[id] = true;
        }
        else
        {
            using namespace std;
            this_thread::sleep_for(1ns);
        }
    }
}
threadpool::threadpool()
{
    m_num_threads = std::thread::hardware_concurrency();
    worked = std::vector<std::atomic<bool>>(m_num_threads - 1);
    for (auto &b : worked)
        b = true;
    for (int i = 0; i < m_num_threads - 1; i++)
    {
        threads.push_back(std::make_unique<std::thread>(worker_thread, this, i));
    }
}
threadpool::threadpool(int numThreads)
{
    m_num_threads = numThreads;
    worked = std::vector<std::atomic<bool>>(m_num_threads - 1);
    for (auto &b : worked)
        b = true;
    for (int i = 0; i < m_num_threads - 1; i++)
    {
        threads.push_back(std::make_unique<std::thread>(worker_thread, this, i));
    }
}
threadpool::~threadpool()
{
    running = false;
    for (auto &t : threads)
    {
        t->join();
    }
}
void threadpool::doWork(int size, std::function<void(int)> f, int _grain = -1)
{
    if (_grain == -1)
    {
        grain = csize / m_num_threads;
    }
    else
        grain = _grain;
    counter = 0;
    func = f;
    csize = size;
    for (auto &b : worked)
        b = false;
    // work in main thread

    work();
    // wait for other threads

    for (auto &b : worked)
    {
        while (!b)
        {
            using namespace std;
            // this_thread::__sleep_for(0s,1ns);
            this_thread::sleep_for(0ns);
        }
    }
}
