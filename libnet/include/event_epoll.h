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
        EPOLL_ADD = 0,      // 添加
        EPOLL_MOD = 1,      // 修改
        EPOLL_DEL = 2,      // 删除
    };

    enum _SetCtlEvent
    {
        EPOLL_ET_READ = 0,    // 边沿触发读
        EPOLL_ET_WRITE = 1,   // 边沿触发写
        EPOLL_READ = 2,       // 水平触发读
        EPOLL_WRITE = 3,      // 水平触发写
    };

public:
    int Create();
    int SetCtl(int iFd, int iOpt, int iEvent, void *pData);
    int Wait(epoll_event *pEv, int iEvSize, uint32_t dwTimeout);
    void Close(int iFd = -1);
};

}
#endif

