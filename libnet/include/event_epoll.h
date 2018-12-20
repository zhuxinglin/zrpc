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

#ifndef __EVENT_EPOLL_H__
#define __EVENT_EPOLL_H__

#include "file_fd.h"
#include <sys/epoll.h>

class CEventEpoll : public CFileFd
{
public:
    CEventEpoll();
    ~CEventEpoll();

public:
    int Create();
    int SetCtl(int iFd, int iOpt, int iEvent, void *pData);
    int Wait(epoll_event *pEv, int iEvSize, uint32_t dwTimeout);
    
private:
    virtual void Close(int iFd = -1);
};


#endif

