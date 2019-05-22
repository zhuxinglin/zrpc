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
namespace znet
{

class CCoroutine
{
private:
    CCoroutine();
    ~CCoroutine();

public:
    static CCoroutine* GetObj();
    static void Release();
    inline void GetContext(ucontext_t *uCon) { getcontext(uCon); }
    void SetRspSize(uint32_t dwRspSize);
    int Create(ITaskBase *pBase);
    void Swap(ITaskBase* pDest, ITaskBase* pSrc);
    void Swap(ITaskBase *pDest, bool bIsMain = true);
    void Del(ITaskBase *pBase);
    const char* GetErr() {return m_pszErr;}
    ITaskBase* GetTaskBase();

private:
    static void Run(void* p);

private:
    CMemoryPool m_oRsp;
    CMemoryPool m_oContext;
    uint32_t m_dwRspSize;
    static CCoroutine* m_pSelf;
    const char* m_pszErr;
};

}

#endif
