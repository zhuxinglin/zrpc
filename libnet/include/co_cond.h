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

#include <set>
#include <stdint.h>
#include "co_lock.h"

class CCoCond
{
public:
    CCoCond(/* args */);
    ~CCoCond();

public:
    void Signal();
    void Broadcast();
    void Wait(CCoLock* pLock);
    bool TimeWait(CCoLock* pLock, uint32_t dwTimeout);

private:
    typedef std::set<uint64_t> CondQueue;
    volatile uint32_t m_dwSync;
};


#endif
