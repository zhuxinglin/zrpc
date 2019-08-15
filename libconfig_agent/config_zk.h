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

#ifndef __CONFIG_ZK_H__
#define __CONFIG_ZK_H__

#include <zk_api.h>
#include <libnet.h>
#include <map>
#include "config_watcher.h"

namespace zplugin
{

class ConfigZk : public znet::ITaskBase
{
public:
    ConfigZk(zkapi::IZkApi* zk);
    ~ConfigZk();

    int Init(const char* pZkAddrFileName);

private:
    virtual void Run();

private:
    zkapi::IZkApi* m_pZkApi;
    std::string m_sZkAddr;
    ConfigWatcher* m_pConfWatch;
};

}

#endif
