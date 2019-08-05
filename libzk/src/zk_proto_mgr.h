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
#include "hashtable/hashtable_itr.h"
#include <co_chan.h>
#include <atomic>

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
    virtual int AddAuth(const char *pszScheme, const std::string sCert);
    virtual int Create(const char *pszPath, const std::string &sValue,
                       const std::vector<zkproto::zk_acl> *acl, int flags, std::string &sPathBuffer);
    virtual int Delete(const char *pszPath, int version);
    virtual int Exists(const char *pszPath, int watch, zkproto::zk_stat *stat);
    virtual int GetData(const char *pszPath, int watch, std::string &sBuff, zkproto::zk_stat *stat);
    virtual int SetData(const char *pszPath, const std::string &sBuffer, int version, zkproto::zk_stat *stat);
    virtual int GetChildern(const char *pszPath, int watch, std::vector<std::string> &str);
    virtual int GetChildern(const char *pszPath, int watch, std::vector<std::string> &str, zkproto::zk_stat *stat);
    virtual int GetAcl(const char *pszPath, std::vector<zkproto::zk_acl> &acl, zkproto::zk_stat *stat);
    virtual int SetAcl(const char *pszPath, int version, const std::vector<zkproto::zk_acl> &acl);
    virtual int Multi(int count, const std::vector<zoo_op_t> &ops, std::vector<zoo_op_result_t> *result);
    virtual int Sync(const char* pszPath);

private:
    virtual void Run();
    virtual void Error(const char* pszExitStr);

private:
    int setConnectAddr(const char *pszHost);
    int connectZkSvr();
    int connectResp();
    int dispatchMsg(std::shared_ptr<char>& oMsg, int iSumLen);
    int ping();
    int sendSetWatcher();
    int sendAuthInfo();
    template<class T>
    int sendAuthPackage(const T& it);
    std::string&& prependString(const char* path, int flags);
    int isValidPath(const char* path, int len, const int flags);
    std::string subString(const std::string& server_path);

    std::string&& getData();
    int sendData(std::string& data, int32_t xid, int type, uint32_t dwTimeout = 0xFFFFFFFF);
    int Read(std::shared_ptr<char>& oMsg);
    int getXid();
    struct return_result;
    int readResult(std::shared_ptr<return_result>& oRes);

private:
    clientid_t m_oClientId;
    std::string m_sErr;
    int m_iCurrHostIndex;
    znet::CNetClient m_oCli;
    bool m_bIsExit{true};
    int m_iTimeout;
    int64_t m_iLastZxid{0};
    WatcherEvent *m_pEvent;
    std::string m_sChroot;
    std::atomic_int m_iXid;
    bool m_bIsConnect = true;

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

    struct auth_info
    {
        std::string scheme;
        std::string auth;

        auth_info() = default;
        ~auth_info() = default;
        auth_info(const auth_info &v) = default;
    };

    std::vector<address_info> m_vAddr;
    hashtable *active_node_watchers;
    hashtable *active_exist_watchers;
    hashtable *active_child_watchers;
    std::vector<auth_info> m_vAuth;

    struct return_result
    {
        std::shared_ptr<char> msg;
        int err;
        int xid;
        int msg_len{0};
    };
    znet::CCoChan<std::shared_ptr<return_result> > m_oChan;
};


}

#endif