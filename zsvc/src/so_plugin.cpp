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

using namespace zrpc;
using namespace zplugin;

CSoPlugin::CSoPlugin()
{
    m_mapRoute = new map_so_info;
    m_pProc = new zplugin::CSharedData;
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
    // 是否为新增so
    //TODO:
    bool bIsUpdate = false;
    std::shared_ptr<zplugin::CSharedData> pSo = GetSoShareData(pszSoName, bIsUpdate);
    if (bIsUpdate)
        return -1;

    set_key *pkey;
    CSoFunAddr *pAddr;
    pAddr = GetLoadSo(pszSoName, &pkey, pSo);
    if (!pAddr)
        return -1;

    pmapRoute->insert(map_so_info::value_type(pAddr, pkey));
    return 0;
}

int CSoPlugin::ExecSo(znet::SharedTask& oCo, CControllerBase* pController, uint64_t qwKey, std::string *pMessage, int &iCode)
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
        iRet = it->first->pPlugin->Process(oCo, pController, qwKey, pMessage);
        __sync_fetch_and_sub(&it->first->dwCount, 1);

        iCode = 200;
        break;
    }
    return iRet;
}

int CSoPlugin::InnerSo(znet::SharedTask& oCo, CControllerBase* pController, uint64_t qwKey, std::string *pReq, std::string *pResp)
{
    map_so_info *pRoute = m_mapRoute;
    int iRet = -1;
    for (map_so_info_it it = pRoute->begin(); it != pRoute->end(); ++it)
    {
        set_key_it iter = it->second->find(qwKey);
        if (iter == it->second->end())
            continue;

        __sync_fetch_and_add(&it->first->dwCount, 1);
        iRet = it->first->pPlugin->Process(oCo, pController, qwKey, pReq, pResp);
        __sync_fetch_and_sub(&it->first->dwCount, 1);
        break;
    }
    return iRet;
}

int CSoPlugin::LoadCallSo(const char *pszSoName)
{
    set_key* pkey;
    std::shared_ptr<zplugin::CSharedData> pSo;
    CSoFunAddr *pAddr = GetLoadSo(pszSoName, &pkey, pSo);
    if (!pAddr)
    {
        return -1;
    }

    m_mapRoute->insert(map_so_info::value_type(pAddr, pkey));
    return 0;
}

CSoFunAddr *CSoPlugin::GetLoadSo(const char *pszSoName, set_key **psetKey, std::shared_ptr<zplugin::CSharedData>& pSo)
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
    pAddr->iDelFlag = 0;
    pAddr->sSoName = pszSoName;
    pAddr->dwCount = 0;
    if (!pSo)
    {
        pAddr->pSo.reset(new zplugin::CSharedData);
        memset(pAddr->pSo.get(), 0, sizeof(zplugin::CSharedData));
    }
    else
    {
        pAddr->pSo = pSo;
    }

    do
    {
        if (!pAddr->pPlugin)
        {
            LOGE << "create '" << pszSoName << "',fun 'SoPlugin' fail";
            break;
        }

        if (pAddr->pPlugin->Initialize(znet::CLog::GetObj(), znet::CNet::GetObj(), m_pProc, pSo.get()) < 0)
        {
            LOGE << "create '" << pszSoName << "',fun 'Initialize' fail";
            break;
        }

        *psetKey = new set_key;
        if (!*psetKey)
        {
            LOGE << "new key '" << pszSoName << "',fun 'SoPlugin' fail";
            break;
        }

        pAddr->pPlugin->GetRouteTable(**psetKey);
        return pAddr;
    }while (0);

    delete pAddr;
    dlclose(pHandle);
    return 0;
}

int CSoPlugin::Swap(map_so_info **pmapRoute)
{
    map_so_info *pTemp = *pmapRoute;
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end(); ++it)
    {
        std::string sTmp = GetSoName(it->first->sSoName.c_str());
        for (map_so_info_it iter = pTemp->begin(); iter != pTemp->end(); ++iter)
        {
            if (iter->first->sSoName.find(sTmp.c_str()) != std::string::npos)
            {
                it->first->iDelFlag = 1;
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

map_so_info* CSoPlugin::DelAll()
{
    m_bIsExit = true;
    map_so_info* pSoInfo = new map_so_info;
    map_so_info* pTemp = m_mapRoute;
    m_mapRoute = pSoInfo;
    if (pTemp)
    {
        for (map_so_info_it it = pTemp->begin(); it != pTemp->end(); ++ it)
            it->first->iDelFlag = 1;
    }

    return pTemp;
}

int CSoPlugin::Del(map_so_info *pmapRoute, bool bIsAll)
{
    LOGI << "delete so .... ";
    for (map_so_info_it it = pmapRoute->begin(); it != pmapRoute->end();)
    {
        if (it->first->iDelFlag == 0 && bIsAll)
        {
            ++it;
            continue;
        }

        // 正在执行中
        if (it->first->dwCount != 0)
            return -1;

        it->first->pPlugin->Release();
        dlclose(it->first->pSoHandle);
        if (bIsAll && !m_bIsExit)
            remove(it->first->sSoName.c_str());

        LOGI << "Uninstall : " << it->first->sSoName;
        delete it->first;
        delete it->second;
        it = pmapRoute->erase(it);
    }
    m_bIsExit = false;
    return 0;
}

int CSoPlugin::Repeat(std::string& sSoName)
{
    LOGI << "load => " << sSoName;
    std::string sTmp = GetSoName(sSoName.c_str());
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end();)
    {
        std::size_t dwPost;
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
            it->first->iDelFlag = 1;
            iRet = 0;
            continue;
        }

        pmapRoute->insert(map_so_info::value_type(it->first, it->second));
    }
    return iRet;
}

std::shared_ptr<zplugin::CSharedData> CSoPlugin::GetSoShareData(const char *pszSoName, bool &bIsUpdate)
{
    std::string sTmp = GetSoName(pszSoName);
    LOGI << "load => " << pszSoName;
    std::shared_ptr<zplugin::CSharedData> pSo(nullptr);
    for (map_so_info_it it = m_mapRoute->begin(); it != m_mapRoute->end(); ++it)
    {
        if (it->first->sSoName.find(sTmp) == std::string::npos)
            continue;

        if (it->first->sSoName.compare(pszSoName) == 0)
        {
            LOGW << "old so name : " << it->first->sSoName << " == " << "new so name : " << pszSoName;
            bIsUpdate = true;
            break;
        }
        pSo = it->first->pSo;
        break;
    }
    return pSo;
}

std::string CSoPlugin::GetSoName(const char* pszSoName)
{
    std::string sTmp = pszSoName;
    const char *s = pszSoName;
    const char *c = strstr(s, SO_VERSION) + sizeof(SO_VERSION) - 1;
    while (*c != 0 && *c != '.')
        ++c;
    sTmp.resize(c - s);
    return sTmp;
}
