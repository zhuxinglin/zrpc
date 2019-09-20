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

#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include "memory_pool.h"
#include <stdint.h>
#include "net_struct.h"
#include <sys/time.h>

namespace znet
{

class CTaskQueue
{
public:
    CTaskQueue(uint32_t dwWorkThreadCount);
    ~CTaskQueue();

public:
    CTaskNode *Create(ITaskBase* pTask);
    CTaskNode *AddExecTask(CTaskNode *pNode, bool bIsExist = true);
    void AddWaitExecTask(CTaskNode *pNode);
    CTaskNode *GetFirstExecTask();
    void DelTask(CTaskNode *pNode);

    CTaskNode *AddWaitTask(CTaskNode *pNode);
    void DelWaitTask(CTaskNode *pNode, bool bIsExist = true);

    int SwapWaitToExec(uint64_t qwCid, bool bIsLock = true);

    int SwapTimerToExec(uint64_t qwCurTime);

    void UpdateTimeout(uint64_t qwCid);

    uint32_t GetCurTaskCount() const{return m_dwCurTaskCount;};

    void AddAllToExec();

    void ExitTask(uint64_t qwCid);
    bool IsExitTask(uint64_t qwCid);

private:
    CTaskNode* AddTask(CTaskNode* pNode, int iRunStatus);
    int DelRbTask(CTaskWaitRb *pRb, CTaskNode *pNode);
    CTaskNode *UpdateTask(CTaskWaitRb *pRb, uint64_t qwCid, bool bIsLock);
    void SwapTimerToExec(uint64_t qwCurTime, int iIndex, int& iSu);
    CTaskWaitRb* GetWaitRb(uint64_t qwCid);
    void UpdateTaskTime(CTaskWaitRb *pRb, CTaskNode *pTaskNode);
    CTaskNode* DelMultiMap(CTaskWaitRb *&pRb, const CTaskKey& oKey, uint64_t qwCId);
    CTaskNode* GetTaskNode(uint64_t qwCid, CTaskWaitRb *pRb);

private:
    CMemoryPool m_oPool;
    CTaskQueueNode m_oExec;

    CTaskWaitRb *m_pWait;
    uint32_t m_dwSumCpu;
    volatile uint32_t m_dwCurTaskCount;
};

}

#endif
