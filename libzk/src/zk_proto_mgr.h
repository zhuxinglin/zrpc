/*
* zk protocol manager
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

#ifndef __ZK_PROTO_MGR_H__
#define __ZK_PROTO_MGR_H__

#include "../include/zk_api.h"
#include <net_client.h>
#include "watcher_event.h"

namespace zkapi
{

class ZkProtoMgr : public IZkApi, public znet::ITaskBase
{
public:
    ZkProtoMgr();
    ~ZkProtoMgr();

private:
    virtual const char* GetErr();
    virtual int Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId);
    virtual void Close();

private:
    virtual void Run();
    virtual void Error(const char* pszExitStr);

private:
    int setConnectAddr(const char *pszHost);
    int connectZkSvr();
    int dispatchMsg(std::shared_ptr<char>& oMsg, int iSumLen);
    int ping();
    int Read(std::shared_ptr<char>& oMsg);

private:
    clientid_t m_oClientId;
    std::string m_sErr;
    int m_iCurrHostIndex;
    znet::CNetClient m_oCli;
    bool m_bIsExit{true};
    int m_iTimeout;
    int64_t m_iLastZxid{0};
    WatcherEvent m_oEvent;

    struct address_info
    {
        std::string ip;
        uint16_t port;

        address_info() = default;
        ~address_info() = default;
        address_info(const address_info& v) = default;

        address_info(address_info&& v)
        {
            ip = std::move(v.ip);
            port = port;
        }
    };
    std::vector<address_info> m_vAddr;
};


}

#endif