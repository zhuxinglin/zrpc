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


#include "go.h"
#include <unistd.h>
#include "coroutine.h"
#include "task_queue.h"
#include <unistd.h>
#include "timer_fd.h"
#include <atomic>
#include <sys/types.h>
#include <sys/syscall.h>

using namespace znet;

std::atomic_uint g_dwExecTheadCount(0);
CSem g_oSem;

CGo::CGo()
{
}

CGo::~CGo()
{
}

int CGo::Initialize(void *pUserData)
{
    return 0;
}

int CGo::PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg)
{
    return 0;
}

void CGo::Run(uint32_t dwId)
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    CCoroutine *pCor = CCoroutine::GetObj();
    ucontext_t uCon;
    pCor->GetContext(&uCon);
    while (m_bExit)
    {
        g_oSem.Wait();

        ++g_dwExecTheadCount;

        CTaskNode* pTaskNode;
        while ((pTaskNode = pTaskQueue->GetFirstExecTask()))
        {
            ITaskBase *pTask = pTaskNode->pTask;
            if (pTask->m_wRunStatus == ITaskBase::RUN_NOW 
                || pTask->m_wRunStatus == ITaskBase::RUN_LOCK 
                || pTask->m_wRunStatus == ITaskBase::RUN_SLEEP)
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }
            else if ((pTask->m_wRunStatus == ITaskBase::RUN_EXIT) && pTask->m_wIsRuning)
            {
                // 这里只实用于注册epoll失败时有效，如正在执行的协程不能做本操作，否则栈中保存堆的地址将不能释放
                pTask->Error("wait execute timeout");
                delete pTask;
                continue;
            }

            if (!pTask->m_pContext && !pTask->m_pSp && pCor->Create(pTask) < 0)
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }

            pTask->m_wIsRuning = false;
            pTask->m_pMainCo = &uCon;
            pTask->m_wRunStatus = ITaskBase::RUN_EXEC;
            //printf("call start 2222222222222222222222222 %li  %lu    %p   %u\n", syscall(SYS_gettid), pTask->m_qwCid, pTask, pTask->m_wRunStatus);
            pCor->Swap(pTask);
            /*int iType = uCon.uc_mcontext.gregs[1];
            int iFd = uCon.uc_mcontext.gregs[2];
            int iMod = uCon.uc_mcontext.gregs[3];
            int iEvent = uCon.uc_mcontext.gregs[4];*/
            //printf("call end 00000000000000000000000000 %li  %lu    %p   %d\n", syscall(SYS_gettid), pTask->m_qwCid, pTask, pTask->m_wRunStatus);
            if (pTask->m_wRunStatus != ITaskBase::RUN_EXIT)
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }

            {
                //printf("__________________ %p\n", pTask->m_oPtr.get());
                std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
                pTask->m_oPtr = nullptr;
            }
        }

        --g_dwExecTheadCount;
    }
}

