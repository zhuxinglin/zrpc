/*
*
*
*
*
*
*
*
*
*
*
*
*
* 
 */

#ifndef __CO_CHAN_H__
#define __CO_CHAN_H__

#include <queue>
#include <memory>
#include "co_sem.h"
#include <atomic>


namespace znet
{

template<typename T>
class CCoChan
{
public:
    CCoChan(uint32_t dwLimit = -1){m_dwQueueLimit = dwLimit;}
    ~CCoChan(){}

public:
    CCoChan& operator<<(T &o)
    {
        if (m_bIsClose)
            return *this;

        while (1)
        {
            lock();
            if (m_dwQueueLimit == m_oChan.size())
            {
                ++m_dwEnterLimit;
                unlock();

                if (!m_oInputSem.Wait(m_dwInputTimeout))
                {
                    --m_dwEnterLimit;
                    return *this;
                }

                --m_dwEnterLimit;
                continue;
            }
            break;
        }

        m_oChan.push(o);
        unlock();
        m_oOutputSem.Post();
        return *this;
    }

    T& operator >> (T& o)
    {
        if (m_bIsClose)
            return o;

        do
        {
            if (!m_oOutputSem.Wait(m_dwOutputTimeout))
                return o;
            lock();

            if (m_oChan.empty())
            {
                unlock();
                if (CNet::GetObj()->IsExitCo())
                    break;
                continue;
            }

            o = m_oChan.front();
            m_oChan.pop();
            unlock();
            break;
        }while(1);

        if (m_dwEnterLimit != 0)
            m_oInputSem.Post();

        return o;
    }

    void Close()
    {
        m_bIsClose = true;
    }

    void SetQueueLimit(uint32_t dwLimit)
    {
        m_dwEnterLimit = dwLimit;
    }

    void SetInputTimeout(uint32_t dwInputTimeout)
    {
        m_dwInputTimeout = dwInputTimeout;
    }

    void SetOutputTimeout(uint32_t dwOutputTimeout)
    {
        m_dwOutputTimeout = dwOutputTimeout;
    }

private:
    void lock()
    {
        while (__sync_lock_test_and_set(&m_dwSync, 1));
    }

    void unlock()
    {
        __sync_lock_release(&m_dwSync);
    }

private:
    typedef std::queue<T> ChanQueue;
    ChanQueue m_oChan;
    uint32_t m_dwInputTimeout = -1;
    uint32_t m_dwOutputTimeout = -1;
    CCoSem m_oInputSem;
    CCoSem m_oOutputSem;
    volatile uint32_t m_dwSync = 0;
    uint32_t m_dwQueueLimit = -1;
    bool m_bIsClose = false;
    std::atomic_uint m_dwEnterLimit{0};
};

}
#endif
