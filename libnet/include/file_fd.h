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

#ifndef __FILE_FD__H__
#define __FILE_FD__H__

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

namespace znet
{

class CFileFd
{
public:
    CFileFd() : m_iFd(-1)
    {}
    CFileFd(int iFd) : m_iFd(iFd)
    {}
    virtual ~CFileFd()
    {
        Close();
    }

public:
    int GetFd() { return m_iFd; }
    void SetFd(int iFd) { m_iFd = iFd; }
    inline std::string GetErr() { return m_sErr; }
    inline void SetErr(std::string sErr) { m_sErr = sErr; }

    virtual void Close(int iFd = -1)
    {
        if (iFd == -1)
        {
            iFd = m_iFd;
            m_iFd = -1;
        }

        if (iFd != -1)
            close(iFd);
    }

protected:
    void SetErr(const char* pszErr, int iFd)
    {
        m_sErr = "";
        char szBuf[64];
        snprintf(szBuf, 64, "fd: %d, ", iFd);
        m_sErr.append(pszErr).append(szBuf);
        snprintf(szBuf, 64, "errno: %d, errnostr: ", errno);
        m_sErr.append(szBuf).append(strerror(errno));
    }

protected:
    int m_iFd;
    std::string m_sErr;
};

}

#endif

