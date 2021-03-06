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
*/

#include "config_watcher.h"

using namespace zplugin;


ConfigWatcher::ConfigWatcher(zkapi::IZkApi* zk)
{
    m_pZkApi = zk;
}

ConfigWatcher::~ConfigWatcher()
{
}

int ConfigWatcher::Init()
{
    return m_oShm.Init();
}

void ConfigWatcher::OnWatcher(int type, znet::ITaskBase* pTask, const std::string& path)
{
    LOGI_BIZ(WATCHER) << "type : " << type << ", path: " << path;
    if (type == static_cast<int>(zkapi::WatcherType::TYPE_ADD_DIR) || type == static_cast<int>(zkapi::WatcherType::TYPE_CONNECT_SUCCESS))
    {
        SetAllData(type == -1);
        return ;
    }
    else if (type == static_cast<int>(zkapi::WatcherType::TYPE_DELETE_DIR))
    {
        DeleteKey(path);
        return;
    }
    OnNotification(path);
}

void ConfigWatcher::SetAllData(bool bIsConnect)
{
    std::map<std::string, std::string> mapConfig;
    if (GetAllDirData(mapConfig, bIsConnect) < 0)
        return;

    SetShmData(mapConfig, bIsConnect);
}

int ConfigWatcher::GetAllDirData(std::map<std::string, std::string>& mapConfig, bool bIsConnect)
{
    std::vector<std::string> str;
    if (GetChildDir(str) < 0)
        return -1;

    if (!bIsConnect)
        m_oShm.AddShmKey(str);

    if (str.empty())
        return -1;      // 没有更改

    for (auto it : str)
    {
        std::string sChildPath(SHM_CONFIG_ROOT);
        sChildPath.append("/").append(it);
        std::string sValue;
        if (GetChildDirData(sChildPath, sValue) < 0)
            continue;
        mapConfig.insert(std::map<std::string, std::string>::value_type(it, sValue));
    }
    return 0;
}

int ConfigWatcher::GetChildDir(std::vector<std::string>& str)
{
    int iRet = m_pZkApi->GetChildern(SHM_CONFIG_ROOT, 1, str);
    if (iRet == 0)
    {
        LOGI_BIZ(WATCHER) << "get root config : " << SHM_CONFIG_ROOT << " success";
        return 0;
    }
    LOGE_BIZ(WATCHER) << SHM_CONFIG_ROOT << " error info : " << m_pZkApi->GetErr() << ", ret: " << iRet;
    return -1;
}

int ConfigWatcher::GetChildDirData(const std::string& sChildPath, std::string& sValue)
{
    zkproto::zk_stat oStat;
    int iRet = m_pZkApi->GetData(sChildPath.c_str(), 1, sValue, &oStat);
    if (iRet != 0)
    {
        LOGE_BIZ(WATCHER) << sChildPath << " error info : " << m_pZkApi->GetErr() << ", ret: " << (iRet == -2 ? "delete" : "-1");
        return -1;
    }
    LOGI_BIZ(WATCHER) << "path: " << sChildPath << ", value: " << sValue;
    return 0;
}

void ConfigWatcher::SetShmData(std::map<std::string, std::string>& mapConfig, bool bIsConnect)
{
    if (bIsConnect)
    {
        int iRet = m_oShm.CompareShmKey(mapConfig);
        if (iRet == 0 && mapConfig.empty())
            return;
    }
    else
        m_oShm.UpdateMap(mapConfig);

    // 更新共享内存
    m_oShm.WriteShmMemory();
}

void ConfigWatcher::OnNotification(const std::string& path)
{
    std::string sValue;
    if (GetChildDirData(path, sValue) < 0)
        return;

    std::string sKey = GetKey(path);
    m_oShm.WriteShmData(sKey, sValue);
}

std::string ConfigWatcher::GetKey(const std::string& path)
{
    const char* s = path.c_str();
    const char* e = path.c_str() + path.length();
    while(*e != '/' && s != e)
        -- e;
    ++ e;
    return std::move(std::string(e));
}

void ConfigWatcher::DeleteKey(const std::string& path)
{
    std::string sKey = GetKey(path);
    LOGI_BIZ(WATCHER) << "delete key: " << sKey;
    m_oShm.DeleteKey(sKey);
    m_oShm.WriteShmMemory();
}
