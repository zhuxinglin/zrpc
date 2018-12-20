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

class CTaskQueue
{
private:
    CTaskQueue();
    ~CTaskQueue();

public:
    static CTaskQueue* GetObj();
    static void Release();
    CTaskNode *Create(ITaskBase* pTask);
    CTaskNode *AddExecTask(CTaskNode *pNode, bool bIsExist = true);
    void AddWaitExecTask(CTaskNode *pNode);
    CTaskNode *GetFirstExecTask();
    void DelTask(CTaskNode *pNode);

    CTaskNode *AddWaitTask(CTaskNode *pNode);
    void DelWaitTask(CTaskNode *pNode, bool bIsExist = true);

    int SwapWaitExec(uint64_t qwCid);

    int SwapTimerExec(uint64_t qwCurTime);

private:
    CTaskNode* AddTask(CTaskNode* pNode, int iRunStatus);
    int DelRbTask(CTaskWaitRb *pRb, CTaskNode *pNode);
    CTaskNode *UpdateTask(CTaskWaitRb *pRb, uint64_t qwCid);
    void SwapTimerExec(uint64_t qwCurTime, int iIndex, int& iSu);
    CTaskWaitRb* GetWaitRb(uint64_t qwCid);
    void UpdateTaskTime(CTaskWaitRb *pRb, CTaskNode *pTaskNode, CTaskKey& oKey);

private:
    CMemoryPool m_oPool;
    CTaskQueueNode m_oExec;

    CTaskWaitRb *m_pWait;
    uint32_t m_dwSumCpu;

    static CTaskQueue* m_pSelf;
};

#endif
