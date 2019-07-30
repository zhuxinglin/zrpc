/*
* zk api
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

#ifndef __ZK_API__H__
#define __ZK_API__H__

#include <stdint.h>
#include <string>
#include <net_client.h>
#include <libnet.h>

struct IWatcher
{
    virtual void OnWatcher(int type, int state, const char* path) = 0;
};

struct clientid_t
{
    int64_t client_id;
    char password[16];
};

class ZkApi
{
public:
    ZkApi();
    ~ZkApi();

public:
    int Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId);

private:
    clientid_t m_oClientId;
    std::string m_sErr;
    IWatcher* m_pWatcher;
    int m_iCurrHostIndex;
    int m_iHostCount;
    znet::CNetClient m_oCli;

    struct address_info
    {
        std::string ip;
        uint16_t port;
    };
    std::vector<address_info> m_vAddr;
};


#endif
