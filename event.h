
// from : https://vorbrodt.blog/2019/02/08/event-objects/ - February 8, 2019 by Martin Vorbrodt
#pragma once

#include <mutex>
#include <condition_variable>

class manual_event
{
public:
    explicit manual_event(bool signaled = false) noexcept
    : m_signaled(signaled) {}

    void signal() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signaled = true;
        m_cv.notify_all();
    }

    void wait() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&](){ return m_signaled != false; });
    }

    void reset() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signaled = false;
    }

private:
    bool m_signaled = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class auto_event
{
public:
    explicit auto_event(bool signaled = false) noexcept
    : m_signaled(signaled) {}

    void signal() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signaled = true;
        m_cv.notify_one();
    }

    void wait() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&](){ return m_signaled != false; });
        m_signaled = false;
    }

private:
    bool m_signaled = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};
