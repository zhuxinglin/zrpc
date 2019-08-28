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


#ifndef __CONFIG_WATCHER_H__
#define __CONFIG_WATCHER_H__

#include <zk_api.h>
#include <map>
#include "config_shm.h"

#define SHM_CONFIG_ROOT     "/shm_config"

namespace zplugin
{

class ConfigWatcher : public zkapi::IWatcher
{
public:
    ConfigWatcher(zkapi::IZkApi* zk);
    ~ConfigWatcher();

    int Init();

private:
    virtual void OnWatcher(int type, znet::ITaskBase* pTask, const std::string& path);
    void SetAllData(bool bIsConnect);
    int GetAllDirData(std::map<std::string, std::string>& mapConfig, bool bIsConnect);
    int GetChildDir(std::vector<std::string>& str);
    int GetChildDirData(const std::string& sChildPath, std::string& sValue);
    void SetShmData(std::map<std::string, std::string>& mapConfig, bool bIsConnect);
    void OnNotification(const std::string& path);
    std::string GetKey(const std::string& path);
    void DeleteKey(const std::string& path);

private:
    zkapi::IZkApi* m_pZkApi;
    ConfigShm m_oShm;
};

}

#endif
