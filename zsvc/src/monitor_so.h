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

#ifndef __MONITOR_SO__H__
#define __MONITOR_SO__H__

#include <task_base.h>
#include <libnet.h>
#include "so_plugin.h"

namespace zrpc
{

class CMonitorSo : public znet::ITaskBase
{
public:
    CMonitorSo();
    ~CMonitorSo();

public:
    int InitMonitorDir(const char* pszDir);
    int Start(CSoPlugin* pPlugin);

private:
    virtual void Run();

private:
    int m_iFd;
    int m_iIw;
    std::string m_sSoPath;
};

}

#endif
