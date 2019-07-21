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


#ifndef __SO_PLUGIN_H__
#define __SO_PLUGIN_H__

#include <map>
#include "so_struct.h"

namespace zrpc
{

class CSoPlugin
{
public:
    CSoPlugin();
    ~CSoPlugin();

public:
    int LoadSo(const char* pszPluginPah);
    int UpdateSo(const char *pszSoName, map_so_info *pmapRoute);
    int Swap(map_so_info **pmapRoute);
    int Del(map_so_info *pmapRoute, bool bIsAll);
    int Uninstall(map_so_info *pmapRoute, const char* pszSoName);

    int ExecSo(znet::SharedTask& oCo, zplugin::CControllerBase* pController, uint64_t qwKey, std::string *pMessage, int &iCode);
    int InnerSo(znet::SharedTask& oCo, zplugin::CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp);

private:
    int LoadCallSo(const char* pszSoName);
    CSoFunAddr *GetLoadSo(const char *pszSoName, set_key **psetKey, std::shared_ptr<zplugin::CSharedData>& pSo);
    int Repeat(std::string& sSoName);
    std::shared_ptr<zplugin::CSharedData> GetSoShareData(const char *pszSoName);
    std::string GetSoName(const char *pszSoName);

private:
    map_so_info* m_mapRoute;
    zplugin::CSharedData *m_pProc{nullptr};
};

}

#endif
