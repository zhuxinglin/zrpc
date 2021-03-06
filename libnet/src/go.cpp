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
#include "context.h"

using namespace znet;

extern CContext* g_pContext;
#define CO_EXIT     (ITaskBase::RUN_EXIT)

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
    CContext* pCx = g_pContext;
    CTaskQueue *pTaskQueue = pCx->m_pTaskQueue;
    CCoroutine *pCor = pCx->m_pCo;
    ucontext_t uCon;
    pCor->GetContext(&uCon);
    while (m_bExit)
    {
        pCx->m_oSem.Wait();

        ++pCx->m_dwExecTheadCount;

        CTaskNode* pTaskNode;
        while ((pTaskNode = pTaskQueue->GetFirstExecTask()))
        {
            ITaskBase *pTask = pTaskNode->pTask;

            if (pTask->m_wRunStatus & ITaskBase::RUN_NOW)
            {
                pTaskQueue->AddExecTask(pTaskNode, false);
                continue;
            }
            else if (pTask->m_wRunStatus & ITaskBase::RUN_LOCK || pTask->m_wRunStatus & ITaskBase::RUN_SLEEP)
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }
            else if ((pTask->m_wRunStatus & ITaskBase::RUN_EXIT) && pTask->m_wIsRuning)
            {
                // 这里只实用于注册epoll失败时有效，如正在执行的协程不能做本操作，否则栈中保存堆的地址将不能释放
                pTask->Error("wait execute timeout");
                std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
                pTask->m_oPtr = nullptr;
                continue;
            }

            if (!pTask->m_pContext && !pTask->m_pSp && pCor->Create(pTask) < 0)
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }

            if ((pTask->m_wExitMode == ITaskBase::MANUAL_EXIT_SO) && (pTask->m_wRunStatus & CO_EXIT))
            {
                //printf("delete end 4444444444444444444444 %li  %lu    %p   %d  %u  %s\n", syscall(SYS_gettid), pTask->m_qwCid, pTask, pTask->m_wRunStatus, pTask->m_wExitMode, pTask->m_sCoName.c_str());
                std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
                pTask->m_oPtr = nullptr;
                continue;
            }

            pTask->m_wIsRuning = false;
            pTask->m_pMainCo = &uCon;
            pTask->m_wRunStatus = (CO_EXIT & pTask->m_wRunStatus) | ITaskBase::RUN_EXEC;
            //printf("call start ssssssssssssssssssssssss %li  %lu    %p   %u  %u %s\n", syscall(SYS_gettid), pTask->m_qwCid, pTask, pTask->m_wRunStatus, pTask->m_wExitMode, pTask->m_sCoName.c_str());
            pCor->Swap(pTask);
            /*int iType = uCon.uc_mcontext.gregs[1];
            int iFd = uCon.uc_mcontext.gregs[2];
            int iMod = uCon.uc_mcontext.gregs[3];
            int iEvent = uCon.uc_mcontext.gregs[4];*/
            //printf("call end eeeeeeeeeeeeeeeeeeeeeeeee %li  %lu    %p   %d  %u  %s\n", syscall(SYS_gettid), pTask->m_qwCid, pTask, pTask->m_wRunStatus, pTask->m_wExitMode, pTask->m_sCoName.c_str());
            if (!(pTask->m_wRunStatus & ITaskBase::RUN_EXIT))
            {
                pTaskQueue->AddWaitExecTask(pTaskNode);
                continue;
            }

            if (pTask->m_wExitMode == ITaskBase::MANUAL_EXIT_SO)
                continue;

            {
                std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
                pTask->m_oPtr = nullptr;
            }
        }

        --pCx->m_dwExecTheadCount;
    }
    ExitAllCoroutine();
}

void CGo::ExitAllCoroutine()
{
    g_pContext->m_pTaskQueue->AddAllToExec();
    CTaskNode *pTaskNode;
    while ((pTaskNode = g_pContext->m_pTaskQueue->GetFirstExecTask()))
    {
        ITaskBase *pTask = pTaskNode->pTask;
        {
            std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
            pTask->m_oPtr = nullptr;
        }
    }
}

#undef CO_EXIT
