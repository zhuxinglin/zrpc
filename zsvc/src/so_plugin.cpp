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

#include "so_plugin.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"

#define SO_VERSION      ".so.V"

using namespace zrpc;
using namespace zplugin;

CSoPlugin::CSoPlugin()
{
    m_mapRoute = new map_so_info;
}

CSoPlugin::~CSoPlugin()
{
    if (m_mapRoute)
    {
        Del(m_mapRoute, false);
        delete m_mapRoute;
    }
    m_mapRoute = 0;
}

int CSoPlugin::LoadSo(const char *pszPluginPah)
{
    DIR *pDir;
    if ((pDir = opendir(pszPluginPah)) == NULL)
    {
        LOGE << "open dir '" << pszPluginPah  << "' fail";
        return -1;
    }

    struct dirent entry, *result = 0;
    while ((readdir_r(pDir, &entry, &result)) == 0)
    {
        if (result == 0)
            break;
        if (result->d_type != DT_REG)
            continue;

        if (strstr(result->d_name, SO_VERSION) == 0)
            continue;

        std::string sSoPath(pszPluginPah);
        sSoPath.append(result->d_name);

        if (Repeat(sSoPath) < 0)
            continue;

        LoadCallSo(sSoPath.c_str());
    }
    closedir(pDir);
    return 0;
}

int CSoPlugin::UpdateSo(const char *pszSoName, map_so_info *pmapRoute)
{
    set_key *pkey;
    CSoFunAddr *pAddr;
    pAddr = GetLoadSo(pszSoName, &pkey);
    if (!pAddr)
        return -1;

    pmapRoute->insert(map_so_info::value_type(pAddr, pkey));
    return 0;
}

int CSoPlugin::ExecSo(CControllerBase *pContrller, uint64_t qwKey, std::string *pMessage, int &iCode)
{
    map_so_info* pRoute = m_mapRoute;
    iCode = 404;
    int iRet = -1;
    for (map_so_info_it it = pRoute->begin(); it != pRoute->end(); ++it)
    {
        set_key_it iter = it->second->find(qwKey);
        if (iter == it->second->end())
            continue;

        __sync_fetch_and_add(&it->first->dwCount, 1);
        iRet = it->first->pPlugin->Process(pContrller, qwKey, pMessage);
        __sync_fetch_and_sub(&it->first->dwCount, 1);
        iCode = 200;
        break;
    }
    return iRet;
}

int CSoPlugin::InnerSo(CControllerBase *pContrller, uint64_t qwKey, std::string *pReq, std::string *pResp)
{
    map_so_info *pRoute = m_mapRoute;
    int iRet = -1;
    for (map_so_info_it it = pRoute->begin(); it != pRoute->end(); ++it)
    {
        set_key_it iter = it->second->find(qwKey);
        if (iter == it->second->end())
            continue;

        __sync_fetch_and_add(&it->first->dwCount, 1);
        iRet = it->first->pPlugin->Process(pContrller, qwKey, pReq, pResp);
        __sync_fetch_and_sub(&it->first->dwCount, 1);
        break;
    }
    return iRet;
}

int CSoPlugin::LoadCallSo(const char *pszSoName)
{
    set_key* pkey;
    CSoFunAddr *pAddr = GetLoadSo(pszSoName, &pkey);
    if (!pAddr)
    {
        return -1;
    }

    m_mapRoute->insert(map_so_info::value_type(pAddr, pkey));
    return 0;
}

CSoFunAddr *CSoPlugin::GetLoadSo(const char *pszSoName, set_key **psetKey)
{
    void *pHandle = dlopen(pszSoName, RTLD_NOW);
    if (!pHandle)
    {
        LOGE << "load so fail, " << dlerror();
        return 0;
    }
    dlerror();
    void *pFunAddr = dlsym(pHandle, "SoPlugin");
    if (!pFunAddr)
    {
        LOGE << "find 'SoPlugin' function fail, " << dlerror();
        dlclose(pHandle);
        return 0;
    }

    CSoFunAddr* pAddr = new CSoFunAddr();
    pAddr->pSoHandle = pHandle;
    pAddr->pFun = (SoExportFunAddr)pFunAddr;
    pAddr->pPlugin = pAddr->pFun();
    pAddr->iFlag = 0;
    pAddr->sSoName = pszSoName;
    pAddr->dwCount = 0;
    if (!pAddr->pPlugin)
    {
        LOGE << "create '" << pszSoName << "',fun 'SoPlugin' fail";
        delete pAddr;
        dlclose(pHandle);
        return 0;
    }

    if (pAddr->pPlugin->Initialize(znet::CLog::GetObj()) < 0)
    {
        LOGE << "create '" << pszSoName << "',fun 'Initialize' fail";
        delete pAddr;
        dlclose(pHandle);
        return 0;
    }

    *psetKey = new set_key;
    if (!*psetKey)
    {
        LOGE << "new key '" << pszSoName << "',fun 'SoPlugin' fail";
        delete pAddr;
        dlclose(pHandle);
        return 0;
    }

    pAddr->pPlugin->GetRouteTable(**psetKey);
    return pAddr;
}

int CSoPlugin::Swap(map_so_info **pmapRoute)
{
    map_so_info *pTemp = *pmapRoute;
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end(); ++it)
    {
        const char *s = strstr(it->first->sSoName.c_str(), SO_VERSION);
        s += sizeof(SO_VERSION);
        while(*s != 0 && *s != '_') ++s;
        std::string sTmp = it->first->sSoName;
        sTmp.resize(s - it->first->sSoName.c_str());
        for (map_so_info_it iter = pTemp->begin(); iter != pTemp->end(); ++iter)
        {
            if (iter->first->sSoName.find(sTmp.c_str()) != std::string::npos)
            {
                it->first->iFlag = 1;
                continue;
            }

            pTemp->insert(map_so_info::value_type(it->first, it->second));
        }
    }

    pTemp = m_mapRoute;
    m_mapRoute = *pmapRoute;
    *pmapRoute = pTemp;
    return 0;
}

int CSoPlugin::Del(map_so_info *pmapRoute, bool bIsAll)
{
    for (map_so_info_it it = pmapRoute->begin(); it != pmapRoute->end(); ++it)
    {
        if (it->first->iFlag == 0 && bIsAll)
            continue;
        
        // 正在执行中
        if (it->first->dwCount != 0)
            return -1;

        it->first->pPlugin->Release();
        dlclose(it->first->pSoHandle);
        if (bIsAll)
            remove(it->first->sSoName.c_str());
        delete it->first;
        delete it->second;
    }
    return 0;
}

int CSoPlugin::Repeat(std::string sSoName)
{
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end();)
    {
        std::string sTmp = sSoName;
        const char *s = sSoName.c_str();
        const char *c = strstr(s, SO_VERSION) + sizeof(SO_VERSION);
        while (*c != 0 && *c != '_') ++c;
        sTmp.resize(c - s);
        uint32_t dwPost;
        dwPost = it->first->sSoName.find(sTmp);
        if (dwPost == std::string::npos)
        {
            ++it;
            continue;
        }

        struct stat oSoFile1;
        struct stat oSoFile2;
        stat(sSoName.c_str(), &oSoFile1);
        stat(it->first->sSoName.c_str(), &oSoFile2);

        if (oSoFile2.st_mtime > oSoFile1.st_mtime)
        {
            remove(sSoName.c_str());
            return -1;
        }

        it->first->pPlugin->Release();
        dlclose(it->first->pSoHandle);
        remove(it->first->sSoName.c_str());
        delete it->first;
        delete it->second;
        m_mapRoute->erase(it ++);
        break;
    }
    return 0;
}

int CSoPlugin::Uninstall(map_so_info *pmapRoute, const char *pszSoName)
{
    int iRet = -1;
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end(); ++ it)
    {
        if (it->first->sSoName.compare(pszSoName) == 0)
        {
            it->first->iFlag = 1;
            iRet = 0;
            continue;
        }

        pmapRoute->insert(map_so_info::value_type(it->first, it->second));
    }
    return iRet;
}
