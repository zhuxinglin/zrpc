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

#ifndef __TIMER_FD_H__
#define __TIMER_FD_H__

#include "file_fd.h"

namespace znet
{

class CTimerFd : public CFileFd
{
public:
    CTimerFd();
    ~CTimerFd();

public:
    int Create(uint32_t dwTimeoutUs, bool bIsTimer = true);
    int Write(uint32_t dwTimeoutUs, bool bIsTimer = true);
    int Read(uint64_t* pTimeoutUs);
    static uint64_t GetNs();
};

}

#endif
