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

#ifndef __CO_CHAN_H__
#define __CO_CHAN_H__

#include <queue>
#include "task_base.h"

class CCoChan
{
public:
    CCoChan(ITaskBase* pTask);
    ~CCoChan();

public:
    bool Write(void* pMsg, uint32_t dwType);
    uint32_t operator () (uint32_t dwTimeout = -1);
    void* GetMsg();

private:
    struct CChan
    {
        uint32_t dwType;
        void* pMsg;
    };

    typedef std::queue<CChan> ChanQueue;
    ChanQueue m_oChan;
    volatile uint32_t m_dwSync;
    uint64_t m_qwCoId;
    ITaskBase* m_pTask;
};

#endif
