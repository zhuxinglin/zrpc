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
#include "zk_protocol.h"

using namespace zkapi;

ZkProtoMgr::ZkProtoMgr() : m_pWatcher(nullptr),
                           m_iCurrHostIndex(0)
{

}

ZkProtoMgr::~ZkProtoMgr()
{

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

    m_iTimeout = dwTimeout;
    m_oCli.SetConnTimeoutMs(3 * 1000);
    if (pWatcher)
        m_pWatcher = pWatcher;

    if (pClientId)
        memcpy(&m_oClientId, pClientId, sizeof(m_oClientId));
    else
        memset(&m_oClientId, 0, sizeof(m_oClientId));

    if (setConnectAddr(pszHost) < 0)
    {
        m_sErr = "check host error";
        return -1;
    }

    znet::CNet::GetObj()->Register(this, 0, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);

    return 0;
}

void ZkProtoMgr::Close()
{
    m_bIsExit = false;
    m_oCli.Close();
}

int ZkProtoMgr::setConnectAddr(const char *pszHost)
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
    connectZkSvr();
    while (m_bIsExit)
    {
        int iLen = 0;
        int iRet = m_oCli.Read(reinterpret_cast<char*>(&iLen), sizeof(iLen));
        if (iRet < 0)
        {
            if (m_bIsExit)
                connectZkSvr();
            continue;
        }

        iLen = ntohl(iLen) - sizeof(iLen);
        int iSumLen = iLen;
        std::shared_ptr<char> oMsg(new char[iLen + 1]);
        char* p = oMsg.get();
        do
        {
            iRet = m_oCli.Read(p, iLen);
            if (iRet < 0)
                break;

            iLen -= iRet;
            p += iRet;
        }while (iLen > 0);

        if (iRet < 0)
        {
            if (m_bIsExit)
                connectZkSvr();
            continue;
        }

        dispatchMsg(oMsg, iSumLen);
    }
}

int ZkProtoMgr::connectZkSvr()
{
    auto conn = [&]() -> int 
    {
        zk_connect_request *pReq = new zk_connect_request;
        pReq->len = sizeof(zk_connect_request);
        pReq->protocol_version = 0;
        pReq->last_zxid_seen = this->m_iLastZxid;
        pReq->timeout = this->m_iTimeout;
        pReq->session_id = this->m_oClientId.client_id;
        memcpy(pReq->passwd, this->m_oClientId.password, sizeof(pReq->passwd));
        pReq->Hton();
        int iRet = m_oCli.Write(reinterpret_cast<char*>(pReq), sizeof(zk_connect_request), 3000);
        delete pReq;
        return iRet;
    };
    while (1)
    {
        if (m_iCurrHostIndex >= static_cast<int>(m_vAddr.size()))
            m_iCurrHostIndex = 0;

        const address_info &addr = m_vAddr[m_iCurrHostIndex];
        ++ m_iCurrHostIndex;
        int iRet = m_oCli.Connect(addr.ip.c_str(), addr.port);
        if (iRet < 0)
            continue;

        if (conn() < 0)
        {
            m_oCli.Close();
            continue;
        }
        break;
    }

    return 0;
}

void ZkProtoMgr::dispatchMsg(std::shared_ptr<char>& oMsg, int iSumLen)
{
    
}
