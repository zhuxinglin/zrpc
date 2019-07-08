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

#ifndef __JSVC__H__
#define __JSVC__H__

#include "config.h"
#include "monitor_so.h"
#include "so_plugin.h"

namespace zrpc
{

class CJSvc
{
public:
    CJSvc();
    ~CJSvc();

public:
    int Init(CConfig* pCfg);
    int Start();
    void Stop();

private:
    int InitGlobal(CConfig::config_info* pCfg);
    int Register(CConfig::config_info* pCfg);
    int Remove(const char* pszUnixPath);
    int LogInit(CConfig::config_info* pCfg);

private:
    CMonitorSo m_oMonitorSo;
    CSoPlugin m_oPlugin;
};

}

#endif
