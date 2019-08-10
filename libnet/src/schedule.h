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
public:
    CSchedule();
    ~CSchedule();

public:
    std::string GetErr();
    virtual int PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg);

private:
    virtual int Initialize(void* pUserData);
    virtual void Run(uint32_t dwId);

private:
    CEventEpoll m_oEvent;
};

}

#endif
