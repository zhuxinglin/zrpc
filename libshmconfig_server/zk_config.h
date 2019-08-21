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

#ifndef __ZK_CONFIG__H__
#define __ZK_CONFIG__H__

#include <libnet.h>
#include <zk_api.h>

namespace zplugin
{

class ZkConfig : public znet::ITaskBase
{
public:
    ZkConfig(zkapi::IZkApi* zk);
    ~ZkConfig();

public:
    int Init(const char* pZkAddr);

private:
    virtual void Run();
    void CreateRoot();

private:
    zkapi::IZkApi* m_pZkApi;
    std::string m_sZkAddr;
};

}

#endif
