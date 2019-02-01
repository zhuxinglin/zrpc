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

#ifndef __NET_STRUCT_H__
#define __NET_STRUCT_H__

#include "task_base.h"
#include "rbtree.h"
#include "file_fd.h"

namespace znet
{

typedef struct _NetEvent
{
    NEWOBJ(ITaskBase, pNewObj);
    void* pData;
    CFileFd* pFd;
    char szServerName[32];
    uint32_t dwTimeoutUs;
    volatile uint32_t dwSync;
    uint8_t wProtocol;
    uint8_t wVer;
}CNetEvent;

typedef struct _TaskQueue
{
    _TaskQueue *pPrev;
    _TaskQueue *pNext;
    ITaskBase *pTask;
    uint32_t dwRef;
} CTaskNode;

typedef struct _TaskQueueNode
{
    CTaskNode* pHead;
    CTaskNode *pTail;
    volatile uint32_t dwSync;

    _TaskQueueNode() : pHead(0),
                       pTail(0),
                       dwSync(0)
    {}
}CTaskQueueNode;

struct CTaskKey
{
    uint64_t qwTimeNs;
    uint32_t dwTimeout;

    CTaskKey(uint64_t qwNs, uint32_t dwTo) : qwTimeNs(qwNs), dwTimeout(dwTo)
    {}

    bool operator>(CTaskKey& rhs)
    {
        if (qwTimeNs > rhs.qwTimeNs && (dwTimeout >= rhs.dwTimeout || ((qwTimeNs + dwTimeout) >= (rhs.qwTimeNs + rhs.dwTimeout))))
            return true;
        return false;
    }

    bool operator<(CTaskKey &rhs)
    {
        if (qwTimeNs < rhs.qwTimeNs && ((dwTimeout <= rhs.dwTimeout) || ((qwTimeNs + dwTimeout) <= (rhs.qwTimeNs + rhs.dwTimeout))))
            return true;
        return false;
    }

}__attribute__((aligned(4)));

typedef struct _TaskWaitRb
{
    typedef rbtree<CTaskKey, CTaskNode *> TaskRb;
    typedef TaskRb::iterator TaskIt;
    TaskRb oTaskRb;
    typedef rbtree<uint64_t, CTaskKey> FdRb;
    typedef FdRb::iterator FdIt;
    FdRb oFdRb;
    volatile uint32_t dwSync;
    _TaskWaitRb() : dwSync(0)
    {}
}CTaskWaitRb;

}

#endif
