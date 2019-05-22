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

#ifndef __EVENT_FD__H__
#define __EVENT_FD__H__

#include "file_fd.h"

namespace znet
{

class CEventFd : public CFileFd
{
public:
    CEventFd();
    ~CEventFd();

public:
    int Create(uint32_t dwInitval = 0, int iFlags = 0);
    int Read(uint64_t* pFlag);
    int Write(uint64_t qwFlag);
};

}

#endif
