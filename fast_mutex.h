
// from : https://vorbrodt.blog/2019/02/12/fast-mutex/ - February 12, 2019 by Martin Vorbrodt
#pragma once

#include <atomic>
#include "event.h"

class fast_mutex
{
public:
    fast_mutex() : m_state(0) {}

    void lock()
    {
        if (m_state.exchange(1, std::memory_order_acquire))
            while (m_state.exchange(2, std::memory_order_acquire))
                m_waitset.wait();
    }

    void unlock()
    {
        if (m_state.exchange(0, std::memory_order_release) == 2)
            m_waitset.signal();
    }

private:
    std::atomic<unsigned int> m_state;
    auto_event m_waitset;
};
