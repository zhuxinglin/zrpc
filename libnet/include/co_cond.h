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


#ifndef __CO_COND__H__
#define __CO_COND__H__

#include <queue>
#include <stdint.h>
#include "co_lock.h"
#include "thread.h"

namespace znet
{

class CCoCond
{
public:
    CCoCond(/* args */);
    ~CCoCond();

public:
    void Signal();
    void Broadcast();
    bool Wait(CCoLock *pLock, uint32_t dwTimeout = -1);

private:
    typedef std::queue<uint64_t> CondQueue;
    CondQueue m_oCond;
    volatile uint32_t m_dwLock{0};
};

}
#endif
