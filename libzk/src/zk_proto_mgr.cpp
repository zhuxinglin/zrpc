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


#include "zk_proto_mgr.h"

using namespace zkapi;

ZkProtoMgr::ZkProtoMgr() : m_pWatcher(nullptr),
                           m_iCurrHostIndex(0)
{

}

ZkProtoMgr::~ZkProtoMgr()
{

}

void ZkProtoMgr::Release()
{
    delete this;
}

const char* ZkProtoMgr::GetErr()
{
    return m_sErr.c_str();
}

int ZkProtoMgr::Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId)
{
    if (!pszHost)
    {
        m_sErr = "check host param empty";
        return -1;
    }

    m_oCli.SetConnTimeoutMs(dwTimeout * 1000);
    if (pWatcher)
        m_pWatcher = pWatcher;

    if (pClientId)
        memcpy(&m_oClientId, pClientId, sizeof(m_oClientId));
    else
        memcpy(&m_oClientId, 0, sizeof(m_oClientId));

    if (SetConnectAddr(pszHost) < 0)
    {
        m_sErr = "check host error";
        return -1;
    }

    znet::CNet::GetObj()->Register(this, );

    return 0;
}

int ZkProtoMgr::SetConnectAddr(const char *pszHost)
{
    const char* p = pszHost;
    const char* s = p;
    const char* d = nullptr;
    while (*p)
    {
        char c = *p;
        ++ p;
        if (('0' > c && c > '9') && c != ',' && c != ':' && c != '.')
            return -1;

        if (d)
        {
            if (c == ',')
            {
                address_info addr;
                addr.ip.append(s, d - s - 1);
                s = p;
                addr.port = atoi(d);
                d = nullptr;
                m_vAddr.push_back(addr);
            }
            continue;
        }

        if (c == ':')
            d = p;
    }

    if (d)
    {
        address_info addr;
        addr.ip.append(s, d - s - 1);
        addr.port = atoi(d);
        m_vAddr.push_back(addr);
    }
    return 0;
}

void ZkProtoMgr::Error(const char* pszExitStr)
{
    m_sErr = pszExitStr;
}

void ZkProtoMgr::Run()
{

}