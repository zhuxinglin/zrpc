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


#ifndef __COROUTIONE__H__
#define __COROUTIONE__H__

#include <ucontext.h>
#include "task_base.h"
#include "memory_pool.h"
#include "thread.h"
namespace znet
{

class CCoroutine
{
public:
    CCoroutine();
    ~CCoroutine();

public:
    inline void GetContext(ucontext_t *uCon) { getcontext(uCon); }
    void SetRspSize(uint32_t dwRspSize);
    int Create(ITaskBase *pBase);
    void Swap(ITaskBase* pDest, ITaskBase* pSrc);
    void Swap(ITaskBase *pDest, bool bIsMain = true);
    void Del(ITaskBase *pBase);
    const char* GetErr() {return m_pszErr;}
    ITaskBase* GetTaskBase();

private:
    static void Run(void* p, void* t);

private:
    CMemoryPool m_oRsp;
    CMemoryPool m_oContext;
    uint32_t m_dwRspSize;
    const char* m_pszErr;
    pthread_key_t m_KeyContext;
};

}

#endif
