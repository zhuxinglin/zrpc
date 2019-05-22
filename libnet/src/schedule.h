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

#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "event_epoll.h"
#include "thread.h"
#include "event_fd.h"

namespace znet
{

class CSchedule : public CThread
{
private:
    CSchedule();
    ~CSchedule();

public:
    std::string GetErr();
    static CSchedule* GetObj();

private:
    virtual int Initialize(void* pUserData);
    virtual void Run(uint32_t dwId);
    virtual int PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg);
    virtual void Release();

private:
    CEventEpoll m_oEvent;
    static CSchedule* m_pSelf;
};

}

#endif
