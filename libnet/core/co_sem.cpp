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

#include "co_sem.h"
#include "coroutine.h"

CCoSem::CCoSem() : m_qwCurId(0)
{
}

CCoSem::~CCoSem()
{
}

void CCoSem::Post()
{
}

bool CCoSem::Wait(uint32_t dwTimeout)
{
    return true;
}
