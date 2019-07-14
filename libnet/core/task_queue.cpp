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

using namespace znet;

extern uint32_t g_dwWorkThreadCount;
CTaskQueue *CTaskQueue::m_pSelf = 0;

CTaskQueue::CTaskQueue()
{
    m_oPool.SetSize(sizeof(CTaskNode));
    m_oPool.SetMaxNodeCount(100000);
    m_dwSumCpu = g_dwWorkThreadCount << 1;
    m_pWait = new CTaskWaitRb[m_dwSumCpu];
    m_dwCurTaskCount = 0;
}

CTaskQueue::~CTaskQueue()
{
    if (m_pWait)
        delete []m_pWait;
}

CTaskQueue *CTaskQueue::GetObj()
{
    if (!m_pSelf)
        m_pSelf = new CTaskQueue();
    return m_pSelf;
}

void CTaskQueue::Release()
{
    if (m_pSelf)
        delete m_pSelf;
    m_pSelf = 0;
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
        CSpinLock oLock(m_oExec.dwSync);
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
        CTaskKey oKey(CTimerFd::GetUs(), pNode->pTask->m_dwTimeout);
        pNode->pTask->m_qwBeginTime = oKey.qwTimeNs;

        CSpinLock oLock(&pRb->dwSync);
        if (!pRb->oTaskRb.insert(oKey, pNode))
            return 0;

        if (!pRb->oFdRb.insert(pNode->pTask->m_qwCid, oKey))
        {
            pRb->oTaskRb.erase(oKey);
            return 0;
        }
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

CTaskNode *CTaskQueue::AddTask(CTaskNode *pNode, int iRunStatus)
{
    CSpinLock oLock(m_oExec.dwSync);
    if (iRunStatus == ITaskBase::RUN_WAIT)
    {
        pNode->dwRef --;
        if (pNode->dwRef == 0)
            return pNode;
    }
    else if (pNode->dwRef != 0)
    {
        pNode->dwRef ++;
        return pNode;
    }
    pNode->dwRef++;

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
    CSpinLock oLock(&pRb->dwSync);
    CTaskWaitRb::FdIt *it = pRb->oFdRb.find(qwCid);
    if (!it)
        return 0;

    CTaskWaitRb::TaskIt *itNode = pRb->oTaskRb.find(it->second);
    if (!itNode)
    {
        pRb->oFdRb.erase(qwCid);
        return 0;
    }

    CTaskNode* pNode = itNode->second;
    UpdateTaskTime(pRb, pNode, it->second);
    
    if (bIsLock && pNode->pTask->m_wRunStatus == ITaskBase::RUN_LOCK)
        pNode->pTask->m_wRunStatus = ITaskBase::RUN_WAIT;

    return pNode;
}

int CTaskQueue::DelRbTask(CTaskWaitRb *pRb, CTaskNode *pNode)
{
    CTaskKey oKey(pNode->pTask->m_qwBeginTime, pNode->pTask->m_dwTimeout);

    CSpinLock oLock(&pRb->dwSync);
    pRb->oFdRb.erase(pNode->pTask->m_qwCid);
    return pRb->oTaskRb.erase(oKey);
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
    CTaskWaitRb::TaskIt *it = 0;
    CSpinLock oLock(pRb->dwSync);
    while (pRb->oTaskRb.begin(&it))
    {
        CTaskKey* pKey = &it->first;

        CTaskNode *pTaskNode = it->second;
        ITaskBase *pBase = pTaskNode->pTask;

        if (pKey->dwTimeout != (uint32_t)-1 
            && pBase->m_wStatus != ITaskBase::STATUS_TIMEOUT && pBase->m_wRunStatus != ITaskBase::RUN_EXEC)
        {
            uint64_t qwEndTime = qwCurTime - pKey->qwTimeNs;
            qwEndTime /= 1e3;

            if (pBase->m_wRunStatus == ITaskBase::RUN_INIT)
            {
                if (qwEndTime > 100 * 1e3)
                    pBase->m_wRunStatus = ITaskBase::RUN_WAIT;
                continue;
            }

            if ((uint32_t)qwEndTime >= pKey->dwTimeout)
            {
                pBase->m_wStatus = ITaskBase::STATUS_TIMEOUT;
                pBase->m_wRunStatus = ITaskBase::RUN_WAIT;

                UpdateTaskTime(pRb, pTaskNode, *pKey);

                AddTask(pTaskNode, ITaskBase::RUN_EXEC);
                it = 0;
                iSu = 0;
                continue;
            }
            break;
        }
    }
}

void CTaskQueue::UpdateTaskTime(CTaskWaitRb *pRb, CTaskNode *pTaskNode, CTaskKey &oKey)
{
    ITaskBase *pBase = pTaskNode->pTask;
    pBase->m_qwBeginTime = CTimerFd::GetUs();

    pRb->oTaskRb.erase(oKey);
    pRb->oFdRb.erase(pBase->m_qwCid);

    CTaskKey oNewKey(pBase->m_qwBeginTime, pBase->m_dwTimeout);
    pRb->oTaskRb.insert(oNewKey, pTaskNode);
    pRb->oFdRb.insert(pBase->m_qwCid, oNewKey);
}
