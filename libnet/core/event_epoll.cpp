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

#include "event_epoll.h"
#include <sys/types.h>
#include "event_fd.h"
#include <string.h>

CEventEpoll::CEventEpoll()
{
}

CEventEpoll::~CEventEpoll()
{
    Close();
}

int CEventEpoll::Create()
{
    m_iFd = epoll_create(1024);
    if (m_iFd < 0)
        SetErr("create epoll fail! ", m_iFd);
    return m_iFd;
}

int CEventEpoll::SetCtl(int iFd, int iOpt, int iEvent, void *pData)
{
    epoll_event ev;
    if (iOpt == 0)
        iOpt = EPOLL_CTL_ADD;
    else if (iOpt == 1)
        iOpt = EPOLL_CTL_MOD;
    else
        iOpt = EPOLL_CTL_DEL;

    if (iEvent == 0)
        ev.events = EPOLLET | EPOLLIN;
    else if (iEvent == 1)
        ev.events = EPOLLET | EPOLLOUT;
    else if (iEvent == 2)
        ev.events = EPOLLIN;
    else
        ev.events = EPOLLOUT;

    ev.data.ptr = pData;
    int iRet = epoll_ctl(m_iFd, iOpt, iFd, &ev);
    if (iRet < 0)
    {
        char szBuf[128];
        snprintf(szBuf, 128, "set epoll ctrl fail, client fd:[%d] opt:[%d], events:[%d], ", iFd, iOpt, ev.events);
        SetErr(szBuf, m_iFd);
        return -1;
    }
    return 0;
}

int CEventEpoll::Wait(epoll_event *pEv, int iEvSize, uint32_t dwTimeout)
{
    int iRet = 0;
    while (1)
    {
        errno = 0;
        iRet = epoll_wait(m_iFd, pEv, iEvSize, dwTimeout);
        if (iRet <= 0)
        {
            // 被系统打断等待事件
            if (EINTR == errno)
            {
                continue;
            }

            // 等待超时
            if (ETIMEDOUT == errno || errno == 0)
                iRet = 0;
            else
                iRet = -1;
        }
        break;
    }
    return iRet;
}

void CEventEpoll::Close(int iFd)
{
    if (-1 != m_iFd)
    {
        CEventFd oEve;
        int iFdTr = oEve.Create();
        oEve.Write(1);

        struct epoll_event ev;
        // 将服务类型及fd设置到epoll中
        ev.data.u64 = iFdTr;
        ev.events = EPOLLIN | EPOLLIN;
        epoll_ctl(m_iFd, EPOLL_CTL_ADD, iFdTr, &ev);
        usleep(0);
    }
}
