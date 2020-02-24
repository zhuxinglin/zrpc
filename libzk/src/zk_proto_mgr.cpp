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
using namespace zkproto;

#define COMPLETION_WATCH -1
#define COMPLETION_VOID 0
#define COMPLETION_STAT 1
#define COMPLETION_DATA 2
#define COMPLETION_STRINGLIST 3
#define COMPLETION_STRINGLIST_STAT 4
#define COMPLETION_ACLLIST 5
#define COMPLETION_STRING 6
#define COMPLETION_MULTI 7

ZkProtoMgr::ZkProtoMgr() : m_iCurrHostIndex(0)
{
    m_iXid = time(0);
    m_sCoName = "zk_mgr";
}

ZkProtoMgr::~ZkProtoMgr()
{
}

const char* ZkProtoMgr::GetErr()
{
    return m_sErr.c_str();
}

int ZkProtoMgr::Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, int flags, const clientid_t *pClientId)
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

    const char* chroot = strchr(pszHost, '/');
    std::string sHost;
    if (chroot)
    {
        m_sChroot = chroot;
        sHost.append(pszHost, chroot - pszHost);
        if (!isValidPath(m_sChroot.c_str(), m_sChroot.length(), 0))
        {
            m_sErr = "chech path failed, path ->";
            m_sErr.append(m_sChroot);
            return -1;
        }
    }
    else
        sHost.append(pszHost);

    if (setConnectAddr(sHost.c_str()) < 0)
    {
        m_sErr = "check host error";
        return -1;
    }

    m_iFlags = flags;
    m_pEvent = new WatcherEvent;
    m_pEvent->Init(pWatcher);

    znet::CNet::GetObj()->Register(this, 0, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);

    return 0;
}

void ZkProtoMgr::Close()
{
    SetManualModeExit();
    m_bIsExit = false;
    if (!m_bIsConnect)
    {
        zk_request_header hdr(getXid(), ZOO_CLOSE_OP);
        hdr.Hton();
        m_oCli.Write(reinterpret_cast<char*>(&hdr), sizeof(hdr), 3000);
    }
    m_oCli.Close();
    std::shared_ptr<return_result> oRes;
    // 等待退出
    m_oChan >> oRes;
    if (m_pEvent)
    {
        m_pEvent->Exit();
        m_pEvent->CloseCo();
    }
    CloseCo();
    // 等待退出
    conet::ITaskBase::Sleep(10);
}

int ZkProtoMgr::setConnectAddr(const char *pszHost)
{
    const char* e = pszHost + strlen(pszHost);
    const char* d = nullptr;
    -- e;
    const char* s = pszHost;
    while (e != s)
    {
        char c = *e;
        -- e;
        if (('0' > c && c > '9') && c != ',' && c != ':' && c != '.')
            return -1;

        if (d)
        {
            if (c == ',')
            {
                address_info addr;
                addr.ip.append(e + 2, d - e - 3);
                addr.port = atoi(d);
                d = nullptr;
                m_vAddr.push_back(addr);
            }
            continue;
        }

        if (c == ':')
            d = e + 2;
    }

    if (d)
    {
        address_info addr;
        addr.ip.append(e, d - e - 1);
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
    while (m_bIsExit)
    {
        if (m_bIsConnect)
        {
            m_oCli.Close();
            connectZkSvr();
            m_bIsConnect = false;
        }

        if (!m_bIsExit)
            break;

        {
            std::shared_ptr<char> oMsg;
            int iSumLen = Read(oMsg);
            if (iSumLen < 0 || !m_bIsExit)
            {
                m_bIsConnect = true;
                continue;
            }

            if (dispatchMsg(oMsg, iSumLen) < 0)
                m_bIsConnect = true;
        }
    }
    exitCo();
}

void ZkProtoMgr::exitCo()
{
    std::shared_ptr<return_result> oRes;
    // 等待退出
    m_oChan << oRes;
}

int ZkProtoMgr::Read(std::shared_ptr<char>& oMsg)
{
    int iLen = 0;
    int iRet;
    while (m_bIsExit)
    {
        iRet = m_oCli.Read(reinterpret_cast<char*>(&iLen), sizeof(iLen), m_iTimeout);
        if (iRet < 0)
        {
            if (iRet == -1)
                return -1;

            if (!m_bIsExit)
                return -1;

            // ping
            if (ping() < 0)
                return -1;
            continue;
        }

        if (iRet == sizeof(iLen))
            break;
    }

    iLen = ntohl(iLen);
    if (iLen < 0 || !m_bIsExit)
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
    }while (iLen > 0 && m_bIsExit);

    if (iRet < 0 || !m_bIsExit)
        return -1;

    return iSumLen;
}

int ZkProtoMgr::connectZkSvr()
{
    while (m_bIsExit)
    {
        if (m_iCurrHostIndex >= static_cast<int>(m_vAddr.size()))
            m_iCurrHostIndex = 0;

        const address_info &addr = m_vAddr[m_iCurrHostIndex];
        ++ m_iCurrHostIndex;
        uint16_t wVer = 4; 
        if (strchr(addr.ip.c_str(), ':'))
            wVer = 6;

        m_oCli.Close();
        int iRet = m_oCli.Connect(addr.ip.c_str(), addr.port, ITaskBase::PROTOCOL_TCP, wVer);
        if (iRet < 0)
            continue;

        if (connectResp() < 0)
            continue;

        if (sendAuthInfo() < 0)
            continue;

        std::string sRoot = "/";
        ZkEvent oEv(-1, sRoot);
        m_pEvent->Push(oEv);
        break;
    }

    return 0;
}

int ZkProtoMgr::connectResp()
{
    auto conn = [&]() -> int 
    {
        zk_connect_request *pReq = new zk_connect_request;
        pReq->protocol_version = 0;
        pReq->last_zxid_seen = this->m_iLastZxid;
        pReq->timeout = this->m_iTimeout + 3000;
        pReq->session_id = this->m_oClientId.client_id;
        pReq->passwd_len = sizeof(pReq->passwd);
        memcpy(pReq->passwd, this->m_oClientId.password, sizeof(pReq->passwd));
        pReq->read_only = m_iFlags & 1;
        pReq->Hton();

        int iRet = m_oCli.Write(reinterpret_cast<char*>(pReq), sizeof(zk_connect_request), 3000);
        delete pReq;
        return iRet;
    };

    if (conn() < 0)
        return -1;

    std::shared_ptr<char> oMsg;
    if (Read(oMsg) < 0)
        return -1;

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
    return 0;
}

int ZkProtoMgr::ping()
{
    zk_request_header hdr(PING_XID, ZOO_PING_OP);
    hdr.Hton();
    return m_oCli.Write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
}

int ZkProtoMgr::sendAuthInfo()
{
    if (m_vAuth.empty())
        return 0;

    for (auto it = m_vAuth.begin(); it != m_vAuth.end(); ++ it)
    {
        if (sendAuthPackage(*it) < 0)
            return -1;
    }
    return 0;
}

std::string ZkProtoMgr::getData()
{
    std::string data;
    data.resize(sizeof(zk_request_header));
    return std::move(data);
}

int ZkProtoMgr::sendData(std::string& data, int32_t xid, int type, uint32_t dwTimeout)
{
    zk_request_header hdr(xid, type);
    hdr.len = data.size() - sizeof(hdr.len);
    hdr.Hton();
    data.replace(0, sizeof(hdr), reinterpret_cast<char *>(&hdr), sizeof(hdr));
    int iRet = m_oCli.Write(data.c_str(), data.size(), dwTimeout);
    if (iRet < 0)
    {
        m_sErr = "send data failed, ret: ";
        m_sErr.append(std::to_string(iRet));
    }
    return iRet;
}

int ZkProtoMgr::getXid()
{
    return m_iXid++;
}

template <class T>
int ZkProtoMgr::sendAuthPackage(const T &it)
{
    zk_auth_packet *au = new zk_auth_packet;
    au->type = 0;
    au->scheme = it.scheme;
    au->auth = it.auth;
    std::string data = getData();
    au->Hton(data);
    delete au;
    return sendData(data, AUTH_XID, ZOO_SETAUTH_OP);
}

int ZkProtoMgr::dispatchMsg(std::shared_ptr<char> &oMsg, int iSumLen)
{
    zk_reply_header* hdr = reinterpret_cast<zk_reply_header*>(oMsg.get());
    hdr->Ntoh();
    if (hdr->zxid > 0)
        m_iLastZxid = hdr->zxid;

    iSumLen -= sizeof(zk_reply_header);
    if (hdr->xid == WATCHER_EVENT_XID)
    {
        zk_watcher_event evt;
        evt.Ntoh(hdr->data, iSumLen);
        evt.path = subString(evt.path);
        ZkEvent oEv(evt.type, evt.path);
        m_pEvent->Push(oEv);
    }
    else if (hdr->xid == SET_WATCHES_XID)
    {}
    else if (hdr->xid == AUTH_XID)
    {
        if (m_vAuth.empty())
        {
            std::shared_ptr<return_result> oRes(new return_result);
            oRes->err = hdr->err;
            oRes->xid = hdr->xid;
            m_oChan << oRes;
        }
        else
        {
            if (hdr->err != 0)
                return -1;
        }
    }
    else
    {
        if (hdr->xid == PING_XID)
            return 0;

        std::shared_ptr<return_result> oRes(new return_result);
        oRes->err = hdr->err;
        oRes->xid = hdr->xid;
        if (hdr->err == 0 && iSumLen > 0)
        {
            std::shared_ptr<char> o(new char[iSumLen + 1]);
            memcpy(o.get(), hdr->data, iSumLen);
            oRes->msg = o;
            oRes->msg_len = iSumLen;
        }
        m_oChan << oRes;
    }
    return 0;
}

int ZkProtoMgr::readResult(std::shared_ptr<return_result>& oRes)
{
    m_oChan.SetOutputTimeout(30000);
    m_oChan >> oRes;
    if (!oRes)
    {
        m_sErr = "read failed, timeout";
        return -1;
    }
    return 0;
}

int ZkProtoMgr::AddAuth(const char *pszScheme, const std::string sCert)
{
    auth_info oAu;
    oAu.scheme = pszScheme;
    oAu.auth.append(sCert);

    if (sendAuthPackage(oAu) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
    {
        m_sErr = "auth failed";
        return -1;
    }

    m_vAuth.push_back(oAu);
    return 0;
}

std::string ZkProtoMgr::prependString(const char *path, int flags)
{
    if (m_sChroot.empty())
        return std::move(std::string(path));

    if (strlen(path) == 1)
        return std::move(std::string(m_sChroot));

    std::string sStr = m_sChroot;
    sStr.append(path);
    if (!isValidPath(sStr.c_str(), sStr.length(), flags))
    {
        m_sErr = "check path failed -> ";
        m_sErr.append(path);
        return std::move(std::string());
    }
    return std::move(sStr);
}

std::string ZkProtoMgr::subString(const std::string& server_path)
{
    if (m_sChroot.empty())
        return server_path;

    if (server_path.find(m_sChroot) == std::string::npos)
    {
        m_sErr = "server path ";
        m_sErr.append(server_path).append(" does not include chroot path ").append(m_sChroot);
        return server_path;
    }

    if (server_path.compare(m_sChroot) == 0) {
        return "/";
    }

    return std::string(server_path.c_str() + m_sChroot.length());
}

int ZkProtoMgr::Create(const char *pszPath, const std::string &sValue,
                       const std::vector<zkproto::zk_acl>* acl, int flags, std::string &sPathBuffer)
{
    zk_create_request *cr = new zk_create_request;
    cr->path = prependString(pszPath, flags);
    if (cr->path.empty())
    {
        delete cr;
        return -1;
    }
    cr->flags = flags;
    cr->data = sValue;

    if (acl)
        cr->acl = *acl;

    std::string data = getData();
    cr->Hton(data);
    delete cr;
    int xid = getXid();
    if (sendData(data, xid, ZOO_CREATE_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    zk_create_response resp;
    if (!resp.Ntoh(oRes->msg.get(), oRes->msg_len))
        return -1;

    sPathBuffer.append(subString(resp.path));
    return 0;
}

int ZkProtoMgr::Delete(const char *pszPath, int version)
{
    zk_delete_request *de = new zk_delete_request;
    de->path = prependString(pszPath, 0);
    if (de->path.empty())
    {
        delete de;
        return -1;
    }
    de->version = version;
    std::string data = getData();
    de->Hton(data);
    delete de;
    int xid = getXid();
    if (sendData(data, xid, ZOO_DELETE_OP) < 0)
        return 0;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    return 0;
}

int ZkProtoMgr::Exists(const char *pszPath, int watch, zkproto::zk_stat *stat)
{
    zk_exists_request* ex = new zk_exists_request;
    ex->path = prependString(pszPath, 0);
    if (ex->path.empty())
    {
        delete ex;
        return -1;
    }
    ex->watch = watch != 0;
    std::string data = getData();
    ex->Hton(data);
    delete ex;
    int xid = getXid();
    if (sendData(data, xid, ZOO_EXISTS_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    zk_exists_response resp;
    resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    *stat = resp.stat;
    return 0;
}

int ZkProtoMgr::GetData(const char *pszPath, int watch, std::string &sBuff, zkproto::zk_stat *stat)
{
    zk_get_data_request *gd = new zk_get_data_request;
    gd->path = prependString(pszPath, 0);
    if (gd->path.empty())
    {
        delete gd;
        return -1;
    }
    gd->watch = watch != 0;
    std::string data = getData();
    gd->Hton(data);
    delete gd;
    int xid = getXid();
    if (sendData(data, xid, ZOO_GETDATA_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    if (stat)
    {
        zk_get_data_response resp(sBuff, *stat);
        resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    }

    return 0;
}

int ZkProtoMgr::SetData(const char *pszPath, const std::string &sBuffer, int version, zkproto::zk_stat *stat)
{
    zk_set_data_request *sd = new zk_set_data_request;
    sd->path = prependString(pszPath, 0);
    if (sd->path.empty())
    {
        delete sd;
        return -1;
    }
    sd->data = sBuffer;
    sd->version = version;
    std::string data = getData();
    sd->Hton(data);
    delete sd;
    int xid = getXid();
    if (sendData(data, xid, ZOO_SETDATA_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    if (stat)
    {
        zk_set_data_response resp(*stat);
        resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    }

    return 0;
}

int ZkProtoMgr::GetChildern(const char *pszPath, int watch, std::vector<std::string> &str)
{
    zk_get_children_request* gc = new zk_get_children_request;
    gc->path = prependString(pszPath, 0);
    if (gc->path.empty())
    {
        delete gc;
        return -1;
    }
    gc->watch = watch;
    std::string data = getData();
    gc->Hton(data);
    delete gc;
    int xid = getXid();
    if (sendData(data, xid, ZOO_GETCHILDREN_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    zk_get_children_response resp(str);
    resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    return 0;
}

int ZkProtoMgr::GetChildern(const char *pszPath, int watch, std::vector<std::string> &str, zkproto::zk_stat *stat)
{
    zk_get_children_request* gc = new zk_get_children_request;
    gc->path = prependString(pszPath, 0);
    if (gc->path.empty())
    {
        delete gc;
        return -1;
    }
    gc->watch = watch;
    std::string data = getData();
    gc->Hton(data);
    delete gc;
    int xid = getXid();
    if (sendData(data, xid, ZOO_GETCHILDREN2_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    if (stat)
    {
        zk_get_children2_response resp(str, *stat);
        resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    }
    return 0;
}

int ZkProtoMgr::GetAcl(const char *pszPath, std::vector<zkproto::zk_acl> &acl, zkproto::zk_stat *stat)
{
    zk_get_acl_request *ga = new zk_get_acl_request;
    ga->path = prependString(pszPath, 0);
    if (ga->path.empty())
    {
        delete ga;
        return -1;
    }
    std::string data = getData();
    ga->Hton(data);
    delete ga;
    int xid = getXid();
    if (sendData(data, xid, ZOO_GETACL_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    if (stat)
    {
        zk_get_acl_response resp(acl, *stat);
        resp.Ntoh(oRes->msg.get(), oRes->msg_len);
    }
    return 0;
}

int ZkProtoMgr::SetAcl(const char *pszPath, int version, const std::vector<zkproto::zk_acl> &acl)
{
    zk_set_acl_request* sa = new zk_set_acl_request;
    sa->path = prependString(pszPath, 0);
    if (sa->path.empty())
    {
        delete sa;
        return -1;
    }
    sa->acl = acl;
    sa->version = version;

    std::string data = getData();
    sa->Hton(data);
    delete sa;
    int xid = getXid();
    if (sendData(data, xid, ZOO_SETACL_OP) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    return 0;
}

int ZkProtoMgr::Multi(const std::vector<zoo_op_t> &ops, std::vector<zoo_op_result_t> *result)
{
    if (sendMultiPackage(ops) < 0)
        return -1;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (!result)
        return 0;

    if (oRes->err != 0)
        return -2;

    getMulti(ops, *result, oRes->msg.get(), oRes->msg_len);

    return 0;
}

int ZkProtoMgr::sendMultiPackage(const std::vector<zoo_op_t> &ops)
{
    std::string data = getData();
    zk_multi_header* mh = new zk_multi_header;
    for (auto it = ops.begin(); it != ops.end(); ++ it)
    {
        mh->type = it->type;
        mh->done = 0;
        mh->err = -1;
        mh->Hton();
        data.append(reinterpret_cast<char*>(mh), sizeof(zk_multi_header));
        switch(it->type)
        {
        case ZOO_CREATE_OP:
        {
            zk_create_request* cr = new zk_create_request;
            cr->path = prependString(it->path.c_str(), it->flags);
            if (cr->path.empty())
            {
                delete cr;
                goto Exit0;
            }
            cr->flags = it->flags;
            cr->data = it->data;
            if (!it->acl.empty())
                cr->acl = it->acl;
            cr->Hton(data);
            delete cr;
        }
        break;

        case ZOO_DELETE_OP:
        {
            zk_delete_request* de = new zk_delete_request;
            de->path = prependString(it->path.c_str(), 0);
            if (de->path.empty())
            {
                delete de;
                goto Exit0;
            }
            de->version = it->version;
            de->Hton(data);
            delete de;
        }
        break;

        case ZOO_SETDATA_OP:
        {
            zk_set_data_request *sd = new zk_set_data_request;
            sd->path = prependString(it->path.c_str(), 0);
            if (sd->path.empty())
            {
                delete sd;
                goto Exit0;
            }
            sd->data = it->data;
            sd->version = it->version;
            sd->Hton(data);
            delete sd;
        }
        break;

        case ZOO_CHECK_OP:
        {
            zk_check_version_request* cv = new zk_check_version_request;
            cv->path = prependString(it->path.c_str(), 0);
            if (cv->path.empty())
            {
                delete cv;
                goto Exit0;
            }
            cv->version = it->version;
            cv->Hton(data);
            delete cv;
        }
        break;

        default:
        {
            m_sErr = "error type : ";
            m_sErr.append(std::to_string(it->type));
            goto Exit0;
        }
        break;
        }
    }
    mh->type = -1;
    mh->done = 1;
    mh->err = -1;
    data.append(reinterpret_cast<char*>(mh), sizeof(zk_multi_header));
    delete mh;

    {
        int xid = getXid();
        if (sendData(data, xid, ZOO_MULTI_OP) < 0)
            return -1;
    }

    return 0;
Exit0:
    delete mh;
    return -1;
}

void ZkProtoMgr::getMulti(const std::vector<zoo_op_t> &ops, std::vector<zoo_op_result_t>& result, char* data, int& len)
{
    bool bIsDone;
    do
    {
        bIsDone = false;
        for (auto it = ops.begin(); it != ops.end(); ++ it)
        {
            zk_multi_header* muhdr = reinterpret_cast<zk_multi_header*>(data);
            muhdr->Ntoh();
            if (muhdr->done)
                break;

            len -= sizeof(zk_multi_header);
            if (muhdr->type == -1)
            {
                zk_error_response* er = reinterpret_cast<zk_error_response*>(muhdr);
                er->Ntoh();

                len -= sizeof(zk_error_response);
                data = getMultiPackage(it->type, er->err, result, er->data, len, bIsDone);
                continue;
            }

            data = getMultiPackage(it->type, 0, result, muhdr->data, len, bIsDone);
            if (bIsDone)
                break;
        }
    } while (bIsDone);
}

char* ZkProtoMgr::getMultiPackage(int type, int err, std::vector<zoo_op_result_t>& result, char* data, int& len, bool& bIsDone)
{
    zoo_op_result_t oRes;
    oRes.err = err;
    if (err)
    {
        result.push_back(oRes);
        return data;
    }

    switch(type)
    {
    case ZOO_GETDATA_OP:
    {
        std::string sData;
        zk_get_data_response resp(sData, oRes.stat);
        oRes.value.push_back(sData);
        data = resp.Ntoh(data, len);
    }
    break;

    case ZOO_SETDATA_OP:
    {
        zk_set_data_response resp(oRes.stat);
        data = resp.Ntoh(data, len);
    }
    break;

    case ZOO_GETCHILDREN_OP:
    {
        zk_get_children_response resp(oRes.value);
        data = resp.Ntoh(data, len);
    }
    break;

    case ZOO_GETCHILDREN2_OP:
    {
        zk_get_children2_response resp(oRes.value, oRes.stat);
        data = resp.Ntoh(data, len);
    }
    break;

    case ZOO_CREATE_OP:
    {
        zk_create_response resp;
        data = resp.Ntoh(data, len);
        oRes.value.push_back(resp.path);
    }
    break;

    case ZOO_GETACL_OP:
    {
        zk_get_acl_response resp(oRes.acl, oRes.stat);
        data = resp.Ntoh(data, len);
    }
    break;

    case ZOO_CHECK_OP:
    case ZOO_DELETE_OP:
    break;

    case ZOO_MULTI_OP:
        bIsDone = true;
    return data;
    }
    result.push_back(oRes);
    return data;
}

int ZkProtoMgr::Sync(const char* pszPath)
{
    zk_sync_request sy;
    sy.path = prependString(pszPath, 0);
    if (sy.path.empty())
        return -1;

    std::string data = getData();
    sy.Hton(data);
    int xid = getXid();
    if (sendData(data, xid, ZOO_SYNC_OP) < 0)
        return 0;

    std::shared_ptr<return_result> oRes;
    if (readResult(oRes) < 0)
        return -1;

    if (oRes->err != 0)
        return -2;

    return 0;
}

int ZkProtoMgr::isValidPath(const char* path, int len, const int flags)
{
    char lastc = '/';
    char c;
    int i = 0;

  if (path == 0)
    return 0;
  if (len == 0)
    return 0;
  if (path[0] != '/')
    return 0;
  if (len == 1) // done checking - it's the root
    return 1;
  if (path[len - 1] == '/' && !(flags & 2))
    return 0;

  i = 1;
  for (; i < len; lastc = path[i], i++) {
    c = path[i];

    if (c == 0) {
      return 0;
    } else if (c == '/' && lastc == '/') {
      return 0;
    } else if (c == '.' && lastc == '.') {
      if (path[i-2] == '/' && (((i + 1 == len) && !(flags & 2))
                               || path[i+1] == '/')) {
        return 0;
      }
    } else if (c == '.') {
      if ((path[i-1] == '/') && (((i + 1 == len) && !(flags & 2))
                                 || path[i+1] == '/')) {
        return 0;
      }
    } else if (c > 0x00 && c < 0x1f) {
      return 0;
    }
  }

  return 1;
}