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
    void Push();
    uint64_t Pop();

private:
    typedef std::queue<uint64_t> LockQueue;
    LockQueue m_oLock;
    volatile uint32_t m_dwSync;
    volatile uint32_t m_dwLock;
};

}
#endif
