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

#include "timer_fd.h"
#include <sys/timerfd.h>

CTimerFd::CTimerFd()
{
}

CTimerFd::~CTimerFd()
{
}

int CTimerFd::Create(uint32_t dwTimeoutUs, bool bIsTimer)
{
    m_iFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (m_iFd < 0)
    {
        SetErr("create timer fd fail", m_iFd);
        return -1;
    }

    return Write(dwTimeoutUs, bIsTimer);
}

int CTimerFd::Write(uint32_t dwTimeoutUs, bool bIsTimer)
{
    struct timespec now;
    struct itimerspec its;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        SetErr("Error clock_gettime timer", m_iFd);
        return -1;
    }

    its.it_value.tv_sec = now.tv_sec + dwTimeoutUs / 1000000;
    its.it_value.tv_nsec = now.tv_nsec + dwTimeoutUs % 1000000;
    if (bIsTimer)
    {
        its.it_interval.tv_sec = dwTimeoutUs / 1000000;
        its.it_interval.tv_nsec = dwTimeoutUs % 1000000;
    }

    if (timerfd_settime(m_iFd, TFD_TIMER_ABSTIME, &its, NULL) < 0)
    {
        SetErr("set timer fd time fail", m_iFd);
        return -1;
    }

    return 0;
}

int CTimerFd::Read(uint64_t *pTimeoutUs)
{
    return read(m_iFd, pTimeoutUs, sizeof(uint64_t));
}

uint64_t CTimerFd::GetUs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}
