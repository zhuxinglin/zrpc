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

#include "co_cond.h"
#include "coroutine.h"

CCoCond::CCoCond() : m_dwSync(0)
{
}

CCoCond::~CCoCond()
{
}

void CCoCond::Signal()
{

}

void CCoCond::Broadcast()
{

}

void CCoCond::Wait(CCoLock *pLock)
{

}

bool CCoCond::TimeWait(CCoLock *pLock, uint32_t dwTimeout)
{
    return true;
}

