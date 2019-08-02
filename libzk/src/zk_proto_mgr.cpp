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
#include <string.h>

using namespace zkapi;

ZkProtoMgr::ZkProtoMgr() : m_iCurrHostIndex(0)
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

    m_iTimeout = dwTimeout * 1000;
    m_oCli.SetConnTimeoutMs(3 * 1000);

    if (pClientId)
        memcpy(&m_oClientId, pClientId, sizeof(m_oClientId));
    else
        memset(&m_oClientId, 0, sizeof(m_oClientId));

    if (setConnectAddr(pszHost) < 0)
    {
        m_sErr = "check host error";
        return -1;
    }

    m_oEvent.Init(pWatcher);

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
    bool bIsConnect = true;
    
    while (m_bIsExit)
    {
        if (bIsConnect)
        {
            m_oCli.Close();
            connectZkSvr();
            bIsConnect = false;
        }

        {
            std::shared_ptr<char> oMsg;
            int iSumLen = Read(oMsg);
            if (iSumLen < 0)
            {
                bIsConnect = true;
                continue;
            }

            if (dispatchMsg(oMsg, iSumLen) < 0)
                bIsConnect = true;
        }
    }
}

int ZkProtoMgr::Read(std::shared_ptr<char>& oMsg)
{
    int iLen = 0;
    int iRet;
    while (1)
    {
        iRet = m_oCli.Read(reinterpret_cast<char*>(&iLen), sizeof(iLen), m_iTimeout);
        if (iRet < 0)
        {
            if (iRet == -1)
                return -1;

            // ·¢ËÍping
            if (ping() < 0)
                return -1;
            continue;
        }

        if (iRet == sizeof(iLen))
            break;
    }

    iLen = ntohl(iLen);
    if (iLen < 0)
        return -1;

    int iSumLen = iLen;
    oMsg.reset(new char[iLen + 1]);
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
        return -1;

    return iSumLen;
}

int ZkProtoMgr::connectZkSvr()
{
    auto conn = [&]() -> int 
    {
        zk_connect_request *pReq = new zk_connect_request;
        pReq->protocol_version = 0;
        pReq->last_zxid_seen = this->m_iLastZxid;
        pReq->timeout = this->m_iTimeout;
        pReq->session_id = this->m_oClientId.client_id;
        pReq->passwd_len = sizeof(pReq->passwd);
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

        {
            std::shared_ptr<char> oMsg;
            iRet = Read(oMsg);
            if (iRet < 0)
            {
                m_oCli.Close();
                continue;
            }

            zk_connect_response* co = reinterpret_cast<zk_connect_response*>(oMsg.get());
            co->Ntoh();

            // if (m_oClientId.client_id != 0 && m_oClientId.client_id != co->session_id)
            // {
            //     m_oCli.Close();
            //     continue;
            // }

            m_oClientId.client_id = co->session_id;
            memcpy(m_oClientId.password, co->passwd, sizeof(m_oClientId.client_id));
            m_iTimeout = co->timeout;
        }
        break;
    }

    return 0;
}

int ZkProtoMgr::ping()
{
    zk_request_header hdr(PING_XID, ZOO_PING_OP);
    hdr.Hton();
    return m_oCli.Write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
}

int ZkProtoMgr::dispatchMsg(std::shared_ptr<char>& oMsg, int iSumLen)
{
    zk_reply_header* hdr = reinterpret_cast<zk_reply_header*>(oMsg.get());
    hdr->Ntoh();
    if (hdr->zxid > 0)
        m_iLastZxid = hdr->zxid;

    if (hdr->xid == WATCHER_EVENT_XID)
    {
        zk_watcher_event* evt = reinterpret_cast<zk_watcher_event*>(hdr->data);
        evt->Ntoh();
        iSumLen -= (sizeof(zk_reply_header) - sizeof(zk_watcher_event));
        std::shared_ptr<char> o(new char[iSumLen + 1]);
        memcpy(o.get(), evt->path, iSumLen);
        ZkEvent oEv(o, evt->type);
        m_oEvent.Push(oEv);
    }
    else if (hdr->xid == SET_WATCHES_XID)
    {}
    else if (hdr->xid == AUTH_XID)
    {
        if (hdr->err != 0)
            return -1;
    }
    else
    {
        
    }
    return 0;
}
