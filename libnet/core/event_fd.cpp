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

#include "event_fd.h"
#include <sys/types.h>
#include <sys/eventfd.h>

using namespace znet;

CEventFd::CEventFd()
{
}

CEventFd::~CEventFd()
{
}

int CEventFd::Create(uint32_t dwInitval, int iFlags)
{
    m_iFd = eventfd(dwInitval, iFlags | EFD_NONBLOCK);
    if (m_iFd < 0)
        SetErr("create event fd fail! ", m_iFd);
    return m_iFd;
}

int CEventFd::Read(uint64_t *pFlag)
{
    return eventfd_read(m_iFd, pFlag);
}

int CEventFd::Write(uint64_t qwFlag)
{
    return eventfd_write(m_iFd, qwFlag);
}

