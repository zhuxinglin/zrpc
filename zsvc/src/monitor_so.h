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

class CMonitorSo : public ITaskBase
{
public:
    CMonitorSo();
    ~CMonitorSo();

public:
    int InitMonitorDir(const char* pszDir, CNet* pNet);
    int Start(CSoPlugin* pPlugin);

private:
    virtual void Run();

private:
    int m_iFd;
    int m_iIw;
    CNet* m_pNet;
    std::string m_sSoPath;
};

#endif
