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

#include "task_queue.h"
#include <string.h>
#include "uconfig.h"
#include "thread.h"
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include "timer_fd.h"
#include "task_base.h"
#include <assert.h>

using namespace znet;
#define CO_EXIT     (ITaskBase::RUN_EXIT)

CTaskQueue::CTaskQueue(uint32_t dwWorkThreadCount)
{
    m_oPool.SetSize(sizeof(CTaskNode));
    m_oPool.SetMaxNodeCount(10000000);
    m_dwSumCpu = dwWorkThreadCount << 1;
    m_pWait = new CTaskWaitRb[m_dwSumCpu];
    m_dwCurTaskCount = 0;
}

CTaskQueue::~CTaskQueue()
{
    if (m_pWait)
        delete []m_pWait;
}

CTaskNode *CTaskQueue::Create(ITaskBase *pTask)
{
    CTaskNode* pNode = (CTaskNode *)m_oPool.Malloc();
    if (__builtin_expect(pNode == 0, 0))
        return 0;
    pNode->pNext = 0;
    pNode->pPrev = 0;
    pNode->pTask = pTask;
    pNode->dwRef = 0;
    return pNode;
}

CTaskNode *CTaskQueue::AddExecTask(CTaskNode *pNode, bool bIsExist)
{
    if (bIsExist)
    {
        if (!AddWaitTask(pNode))
            return 0;
        return AddTask(pNode, ITaskBase::RUN_EXEC);
    }

    return AddTask(pNode, ITaskBase::RUN_NOW);
}

void CTaskQueue::AddWaitExecTask(CTaskNode *pNode)
{
    AddTask(pNode, ITaskBase::RUN_WAIT);
}

CTaskNode *CTaskQueue::GetFirstExecTask()
{
    CTaskNode *pNode;
    {
        CSpinLock<> oLock(m_oExec.dwSync);
        if (!m_oExec.pHead)
            return 0;

        pNode = m_oExec.pHead;
        m_oExec.pHead = pNode->pNext;

        if (m_oExec.pHead)
            m_oExec.pHead->pPrev = 0;
        else
            m_oExec.pTail = 0;
    }

    pNode->pPrev = 0;
    pNode->pNext = 0;

    return pNode;
}

void CTaskQueue::DelTask(CTaskNode *pNode)
{
    m_oPool.Free(pNode);
}

CTaskNode *CTaskQueue::AddWaitTask(CTaskNode *pNode)
{
    {
        CTaskWaitRb *pRb = GetWaitRb(pNode->pTask->m_qwCid);
        CTaskKey oKey(CTimerFd::GetNs(), pNode->pTask->m_dwTimeout);
        pNode->pTask->m_qwBeginTime = oKey.qwTimeNs;

        CSpinLock<> oLock(&pRb->dwSync);
        if (!pRb->oFdRb.insert(pNode->pTask->m_qwCid, oKey))
            return 0;

        pRb->oTaskRb.insert(CTaskWaitRb::TaskRb::value_type(oKey, pNode));
    }
    __sync_fetch_and_add(&m_dwCurTaskCount, 1);
    return pNode;
}

CTaskWaitRb *CTaskQueue::GetWaitRb(uint64_t qwCid)
{
    uint32_t dwIndex = ITaskBase::GetSubCId(qwCid) % m_dwSumCpu;
    return &m_pWait[dwIndex];
}

void CTaskQueue::DelWaitTask(CTaskNode *pNode, bool bIsExist)
{
    if (bIsExist)
    {
        CTaskWaitRb *pRb = GetWaitRb(pNode->pTask->m_qwCid);
        DelRbTask(pRb, pNode);
        __sync_fetch_and_sub(&m_dwCurTaskCount, 1);
    }

    DelTask(pNode);
}

int CTaskQueue::SwapWaitToExec(uint64_t qwCid, bool bIsLock)
{
    CTaskWaitRb *pRb = GetWaitRb(qwCid);
    CTaskNode *pNode = UpdateTask(pRb, qwCid, bIsLock);
    if (!pNode)
        return -1;

    AddTask(pNode, ITaskBase::RUN_EXEC);
    return 0;
}

void CTaskQueue::UpdateTimeout(uint64_t qwCid)
{
    CTaskWaitRb *pRb = GetWaitRb(qwCid);
    UpdateTask(pRb, qwCid, false);
}

void CTaskQueue::ExitTask(uint64_t qwCid)
{
    CTaskWaitRb *pRb = GetWaitRb(qwCid);
    CSpinLock<> oLock(&pRb->dwSync);

    CTaskNode *pNode = GetTaskNode(qwCid, pRb);
    if (!pNode)
        return;

    while (__sync_lock_test_and_set(&pNode->pTask->m_wRunStatusLock, 1));
    pNode->pTask->m_wRunStatus |= ITaskBase::RUN_EXIT;
    __sync_lock_release(&pNode->pTask->m_wRunStatusLock);
    AddTask(pNode, ITaskBase::RUN_EXEC);
}

bool CTaskQueue::IsExitTask(uint64_t qwCid)
{
    CTaskWaitRb *pRb = GetWaitRb(qwCid);
    CSpinLock<> oLock(&pRb->dwSync);
    CTaskNode *pNode = GetTaskNode(qwCid, pRb);
    if (!pNode)
        return true;
    return pNode->pTask->IsExitCo();
}

CTaskNode *CTaskQueue::AddTask(CTaskNode *pNode, int iRunStatus)
{
    CSpinLock<> oLock(m_oExec.dwSync);
    if (iRunStatus == ITaskBase::RUN_WAIT)
    {
        //__sync_fetch_and_sub(&pNode->dwRef, 1);
        -- pNode->dwRef;
        if (pNode->dwRef == 0)
            return pNode;
    }
    else if (iRunStatus == ITaskBase::RUN_EXEC)
    {
        if (pNode->dwRef != 0)
        {
            //__sync_fetch_and_add(&pNode->dwRef, 1);
            pNode->dwRef = 2;
            return pNode;
        }
        // __sync_fetch_and_add(&pNode->dwRef, 1);
        ++pNode->dwRef;
    }

    if (!m_oExec.pHead)
    {
        m_oExec.pTail = pNode;
        m_oExec.pHead = pNode;
        return pNode;
    }
    m_oExec.pTail->pNext = pNode;
    pNode->pPrev = m_oExec.pTail;
    m_oExec.pTail = pNode;
    return pNode;
}

CTaskNode* CTaskQueue::UpdateTask(CTaskWaitRb *pRb, uint64_t qwCid, bool bIsLock)
{
    CSpinLock<> oLock(&pRb->dwSync);
    CTaskWaitRb::FdIt *it = pRb->oFdRb.find(qwCid);
    if (!it)
        return 0;

    CTaskNode* pNode = DelMultiMap(pRb, it->second, qwCid);
    if (!pNode)
    {
        pRb->oFdRb.erase(it);
        return 0;
    }
    UpdateTaskTime(pRb, pNode);

    if (bIsLock && pNode->pTask->m_wRunStatus & ITaskBase::RUN_LOCK)
    {
        while (__sync_lock_test_and_set(&pNode->pTask->m_wRunStatusLock, 1));
        pNode->pTask->m_wRunStatus = (pNode->pTask->m_wRunStatus & CO_EXIT) | ITaskBase::RUN_WAIT;
        __sync_lock_release(&pNode->pTask->m_wRunStatusLock);
    }

    return pNode;
}

int CTaskQueue::DelRbTask(CTaskWaitRb *pRb, CTaskNode *pNode)
{
    CSpinLock<> oLock(&pRb->dwSync);
    CTaskKey oKey(pNode->pTask->m_qwBeginTime, pNode->pTask->m_dwTimeout);
    pRb->oFdRb.erase(pNode->pTask->m_qwCid);
    DelMultiMap(pRb, oKey, pNode->pTask->m_qwCid);
    return 0;
}

CTaskNode* CTaskQueue::DelMultiMap(CTaskWaitRb *&pRb, const CTaskKey& oKey, uint64_t qwCId)
{
    CTaskWaitRb::TaskIt it = pRb->oTaskRb.find(oKey);
    if (it == pRb->oTaskRb.end())
        return nullptr;

    for (; it != pRb->oTaskRb.end(); ++ it)
    {
        if (qwCId == it->second->pTask->m_qwCid)
        {
            CTaskNode* pNode = it->second;
            pRb->oTaskRb.erase(it);
            return pNode;
        }
    }

    return nullptr;
}

int CTaskQueue::SwapTimerToExec(uint64_t qwCurTime)
{
    int iSu = -1;
    for (int i = 0; i < (int)m_dwSumCpu; ++i)
        SwapTimerToExec(qwCurTime, i, iSu);
    return iSu;
}

void CTaskQueue::SwapTimerToExec(uint64_t qwCurTime, int iIndex, int& iSu)
{
    CTaskWaitRb *pRb = &m_pWait[iIndex];
    CSpinLock<> oLock(pRb->dwSync);
    CTaskWaitRb::TaskIt it = pRb->oTaskRb.begin();
    for (; it != pRb->oTaskRb.end();)
    {
        const CTaskKey& oKey = it->first;
        CTaskNode *pTaskNode = it->second;
        ITaskBase *pBase = pTaskNode->pTask;

        if (oKey.dwTimeout == (uint32_t)-1)
            break;

        uint64_t qwEndTime = qwCurTime - oKey.qwTimeNs;
        qwEndTime /= 1e3L;

        if ((uint32_t)qwEndTime < oKey.dwTimeout)
            break;

#define TIMEOUT_CHECK (ITaskBase::RUN_INIT | ITaskBase::RUN_WAIT | ITaskBase::RUN_LOCK | ITaskBase::RUN_SLEEP)
        if (pBase->m_wStatus != ITaskBase::STATUS_TIMEOUT && pBase->m_wRunStatus & TIMEOUT_CHECK)
        {
            if (pBase->m_wRunStatus & ITaskBase::RUN_INIT)
            {
                if (qwEndTime > 100 * 1e3L)
                {
                    while (__sync_lock_test_and_set(&pBase->m_wRunStatusLock, 1));
                    pBase->m_wRunStatus = (CO_EXIT & pBase->m_wRunStatus) | ITaskBase::RUN_READY;
                    __sync_lock_release(&pBase->m_wRunStatusLock);
                }
                ++ it;
                continue;
            }

            pBase->m_wStatus = ITaskBase::STATUS_TIMEOUT;
            while (__sync_lock_test_and_set(&pBase->m_wRunStatusLock, 1));
            pBase->m_wRunStatus = (CO_EXIT & pBase->m_wRunStatus) | ITaskBase::RUN_READY;
            __sync_lock_release(&pBase->m_wRunStatusLock);

            it = pRb->oTaskRb.erase(it);
            UpdateTaskTime(pRb, pTaskNode);

            AddTask(pTaskNode, ITaskBase::RUN_EXEC);
            iSu = 0;
            continue;
        }
#undef TIMEOUT_CHECK
        ++ it;
    }
}

void CTaskQueue::UpdateTaskTime(CTaskWaitRb *pRb, CTaskNode *pTaskNode)
{
    ITaskBase *pBase = pTaskNode->pTask;
    pBase->m_qwBeginTime = CTimerFd::GetNs();

    CTaskWaitRb::FdIt *it = pRb->oFdRb.find(pBase->m_qwCid);
    it->second.qwTimeNs = pBase->m_qwBeginTime;
    it->second.dwTimeout = pBase->m_dwTimeout;

    CTaskKey oNewKey(pBase->m_qwBeginTime, pBase->m_dwTimeout);
    pRb->oTaskRb.insert(CTaskWaitRb::TaskRb::value_type(oNewKey, pTaskNode));
}

void CTaskQueue::AddAllToExec()
{
    for (int i = 0; i < (int)m_dwSumCpu; ++i)
    {
        CTaskWaitRb *pRb = &m_pWait[i];
        CSpinLock<> oLock(pRb->dwSync);
        CTaskWaitRb::TaskIt it = pRb->oTaskRb.begin();
        for (; it != pRb->oTaskRb.end(); ++it)
        {
            CTaskNode *pTaskNode = it->second;
            AddTask(pTaskNode, ITaskBase::RUN_EXEC);
        }
    }
}

CTaskNode* CTaskQueue::GetTaskNode(uint64_t qwCid, CTaskWaitRb *pRb)
{
    CTaskWaitRb::FdIt *it = pRb->oFdRb.find(qwCid);
    if (!it)
        return nullptr;

    CTaskWaitRb::TaskIt itNode = pRb->oTaskRb.find(it->second);
    if (itNode == pRb->oTaskRb.end())
    {
        pRb->oFdRb.erase(it);
        return nullptr;
    }

    for (; itNode != pRb->oTaskRb.end(); ++ itNode)
    {
        if (qwCid == itNode->second->pTask->m_qwCid)
            return itNode->second;
    }

    pRb->oFdRb.erase(it);
    return nullptr;
}

#undef CO_EXIT
