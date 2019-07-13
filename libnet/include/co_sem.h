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

#ifndef __CO_SEM__H__
#define __CO_SEM__H__

#include <stdint.h>
#include <set>
#include "thread.h"
#include <atomic>

namespace znet
{

class CCoSem
{
public:
    CCoSem();
    ~CCoSem();

public:
    void Post();
    bool Wait(uint32_t dwTimeout = -1);
    bool TryWait();

private:
    typedef std::set<uint64_t> SemQueue;
    SemQueue m_oQueue;
    volatile uint32_t m_dwLock;
    volatile uint32_t m_dwCount{0};
};

}
#endif
