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


#include "coroutine.h"
#include <string.h>
#include <stdio.h>
#include "thread.h"
#include "task_queue.h"

CCoroutine *CCoroutine::m_pSelf = 0;
pthread_key_t g_KeyContext;

CCoroutine::CCoroutine() : m_oRsp(1024 * 1024, 1000),
                           m_oContext(sizeof(ucontext_t), 1000),
                           m_dwRspSize(1024 * 1024),
                           m_pszErr("")
{
    pthread_key_create(&g_KeyContext, NULL);
}

CCoroutine::~CCoroutine()
{
    pthread_key_delete(g_KeyContext);
}

CCoroutine *CCoroutine::GetObj()
{
    if (!m_pSelf)
        m_pSelf = new CCoroutine();
    return m_pSelf;
}

void CCoroutine::Release()
{
    if (m_pSelf)
        delete m_pSelf;
    m_pSelf = 0;
}

void CCoroutine::SetRspSize(uint32_t dwRspSize)
{
    m_dwRspSize = dwRspSize;
    m_oRsp.SetSize(dwRspSize + sizeof(long));
}

int CCoroutine::Create(ITaskBase *pBase)
{
    m_pszErr = "";
    if (!pBase)
    {
        m_pszErr = "task base obj null";
        return -1;
    }
    void* pSp = m_oRsp.Malloc();
    if (__builtin_expect(pSp == 0, 0))
    {
        m_pszErr = "new sp fail";
        return -1;
    }

    ucontext_t *pContext = (ucontext_t *)m_oContext.Malloc();
    if (__builtin_expect(pContext == 0, 0))
    {
        m_pszErr = "new context fail";
        m_oRsp.Free(pSp);
        return -1;
    }

    getcontext(pContext);
    pBase->m_pSp = pSp;
    pBase->m_pContext = pContext;
    pContext->uc_link = 0;
    pContext->uc_stack.ss_sp = pSp;
    pContext->uc_stack.ss_size = m_dwRspSize;
    makecontext(pContext, (void(*)())CCoroutine::Run, 1, pBase);
    return 0;
}

void CCoroutine::Swap(ITaskBase *pDest, ITaskBase *pSrc)
{
    pthread_setspecific(g_KeyContext, pDest);
    pDest->m_pMainCo = pSrc->m_pMainCo;
    swapcontext((ucontext_t *)pSrc->m_pContext, (ucontext_t *)pDest->m_pContext);
}

void CCoroutine::Swap(ITaskBase *pDest, bool bIsMain)
{
    if (bIsMain)
    {
        pthread_setspecific(g_KeyContext, pDest);
        swapcontext((ucontext_t *)pDest->m_pMainCo, (ucontext_t *)pDest->m_pContext);
    }
    else
    {
        pthread_setspecific(g_KeyContext, 0);
        /*typeof(*(((ucontext_t *)(pDest->m_pMainCo))->uc_mcontext).gregs)* pGregs;
        pGregs = ((ucontext_t *)(pDest->m_pMainCo))->uc_mcontext.gregs;
        pGregs[1] = iType;
        pGregs[2] = iFd;
        pGregs[3] = iMod;
        pGregs[4] = iEvent;*/
        swapcontext((ucontext_t *)pDest->m_pContext, (ucontext_t *)pDest->m_pMainCo);
    }
}

ITaskBase *CCoroutine::GetTaskBase()
{
    return (ITaskBase *)pthread_getspecific(g_KeyContext);
}

void CCoroutine::Del(ITaskBase *pBase)
{
    m_oRsp.Free(pBase->m_pSp);
    m_oContext.Free(pBase->m_pContext);
}

void CCoroutine::Run(void *p)
{
    ITaskBase *pBase = (ITaskBase *)p;
    pBase->Run();
    pBase->m_wRunStatus = ITaskBase::RUN_EXIT;
    m_pSelf->Swap(pBase, false);
}
