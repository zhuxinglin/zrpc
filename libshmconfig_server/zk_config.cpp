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

#include "zk_config.h"
#include "log.h"

using namespace zplugin;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"

ZkConfig::ZkConfig(zkapi::IZkApi* zk) : m_pZkApi(zk)
{
    m_sCoName = "zk_shmconfig_server";
}

ZkConfig::~ZkConfig()
{
}

int ZkConfig::Init(const char* pZkAddr)
{
    m_sZkAddr = pZkAddr;
    LOGI_BIZ(ZK_INIT) << "zk address : " << m_sZkAddr;
    znet::CNet::GetObj()->Register(this, nullptr, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);
    return 0;
}

void ZkConfig::Run()
{
    if (m_pZkApi->Init(m_sZkAddr.c_str(), nullptr, 30, 0, nullptr) < 0)
    {
        LOGE_BIZ(INIT) << m_pZkApi->GetErr();
        delete m_pZkApi;
        return ;
    }
    LOGI_BIZ(INTI) << "connect zookeeper success";
    CreateRoot();
}

void ZkConfig::CreateRoot()
{
    Sleep(500);
    while (1)
    {
        zkproto::zk_stat stat;
        int iRet = m_pZkApi->Exists("/shm_config", 0, &stat);
        if (iRet < 0)
        {
            if (iRet == -2)
                break;

            Sleep(500);
            continue;
        }
        return;
    }

    while (1)
    {
        std::string sRes;
        zkproto::zk_acl ac;
        ZK_ACL_OPEN(ac);
        std::vector<zkproto::zk_acl> acl;
        acl.push_back(ac);
        int iRet = m_pZkApi->Create("/shm_config", sRes, &acl, 0, sRes);
        if (iRet < 0)
        {
            if (iRet == -1)
                break;
            Sleep(100);
        }
        break;
    }
}

#pragma GCC diagnostic pop
