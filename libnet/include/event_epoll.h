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

namespace znet
{

class CEventEpoll : public CFileFd
{
public:
    CEventEpoll();
    ~CEventEpoll();

public:
    enum _SetCtlOpt
    {
        EPOLL_ADD = 0,
        EPOLL_MOD = 1,
        EPOLL_DEL = 2,
    };

    enum _SetCtlEvent
    {
        EPOLL_ET_IN = 0,
        EPOLL_ET_OUT = 1,
        EPOLL_IN = 2,
        EPOLL_OUT = 3,
    };

public:
    int Create();
    int SetCtl(int iFd, int iOpt, int iEvent, void *pData);
    int Wait(epoll_event *pEv, int iEvSize, uint32_t dwTimeout);
    void Close(int iFd = -1);
};

}
#endif

