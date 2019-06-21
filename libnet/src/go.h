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

#ifndef __GO__H__
#define __GO__H__

#include "thread.h"
#include "event_fd.h"
#include "event_epoll.h"

namespace znet
{

class CGo : public CThread
{
public:
    CGo();
    ~CGo();

public:
    virtual int PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg);

private:
    virtual int Initialize(void* pUserData);
    virtual void Run(uint32_t dwId);

private:
    
};

}

#endif
