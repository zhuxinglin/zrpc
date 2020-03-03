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

#ifndef __CO_LOCK__H__
#define __CO_LOCK__H__

#include <queue>
#include <stdint.h>

namespace znet
{

class CCoLock
{
public:
    CCoLock();
    ~CCoLock();

public:
    void Lock();
    void Unlock();

private:
    uint64_t Pop();

private:
    typedef std::queue<uint64_t> LockQueue;
    LockQueue m_oLock;
    volatile uint16_t m_wLock;
    volatile uint16_t m_wSync;
};

class CCoLocalLock
{
public:
    CCoLocalLock(CCoLock& oLock) : m_oLock(oLock)
    {
        m_oLock.Lock();
    }

    ~CCoLocalLock()
    {
        m_oLock.Unlock();
    }

private:
    CCoLock& m_oLock;
};

}
#endif
