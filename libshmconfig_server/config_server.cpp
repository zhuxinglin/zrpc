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

#include "config_server.h"

using namespace zplugin;

#define URL_QUERY_LIST  "/shm/config/querylist"
#define URL_MOD_CONFIG  "/shm/config/mod"
#define URL_ADD_CONFIG  "/shm/config/add"
#define URL_DEL_CONFIG  "/shm/config/del"
#define URL_SYNC_ALL_CONFIG  "/shm/sync/all/config"

#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#define SHM_CONFIG_ROOT     "/shm_config/"

ConfigServer::ConfigServer():m_pZkApi(0)
{
}

ConfigServer::~ConfigServer()
{
}

int ConfigServer::Initialize(znet::CLog *pLog, znet::CNet *pN, CSharedData *pProc, CSharedData* pSo)
{
    znet::CLog::SetObj(pLog);
    znet::CNet::Set(pN);
    LOGI_BIZ(INIT) << "shm config server";
    std::string sZkConfig;
    std::string sMysqlConfig;

    if (getConnectConfig(sZkConfig, sMysqlConfig) < 0)
        return -1;

    m_pZkApi = zkapi::IZkApi::CreateObj();
    if (m_oDao.Init(sMysqlConfig.c_str()) < 0)
    {
        LOGE_BIZ(INIT) << "init mysql failed, connect : " << sMysqlConfig;
        delete m_pZkApi;
        return -1;
    }

    ZkConfig* pZkConfig = new ZkConfig(m_pZkApi);
    pZkConfig->Init(sZkConfig.c_str());

    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_QUERY_LIST, sizeof(URL_QUERY_LIST) - 1), &ConfigServer::Query));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_MOD_CONFIG, sizeof(URL_MOD_CONFIG) - 1), &ConfigServer::Mod));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_ADD_CONFIG, sizeof(URL_ADD_CONFIG) - 1), &ConfigServer::Add));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_DEL_CONFIG, sizeof(URL_DEL_CONFIG) - 1), &ConfigServer::Del));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_SYNC_ALL_CONFIG, sizeof(URL_SYNC_ALL_CONFIG) - 1), &ConfigServer::SyncAllShmConfig));
    return 0;
}

int ConfigServer::GetRouteTable(std::set<uint64_t>& setKey)
{
    auto it = m_mapFun.begin();
    for (; it != m_mapFun.end(); ++ it)
        setKey.insert(it->first);
    return 0;
}

int ConfigServer::Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pMessage)
{
    auto it = m_mapFun.find(dwKey);
    if (it == m_mapFun.end())
        return -1;

    return (this->*(it->second))(pController, pMessage);
    return 0;
}

int ConfigServer::Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp)
{
    return 0;
}

void ConfigServer::Release()
{
    LOGI_BIZ(CLOSE) << "libshmconfig_server exit .....";
    if(m_pZkApi)
        m_pZkApi->Close();
}

int ConfigServer::getConnectConfig(std::string& sZkConfig, std::string& sMysqlConfig)
{
    FILE* fp = fopen("./conf/shmconfig_server.cfg", "rb");
    if (!fp)
    {
        LOGE_BIZ(INIT) << "open config fail './conf/shmconfig_server.cfg' failed";
        return -1;
    }

    char* szBuf = new char[1024];
    while (!feof(fp))
    {
        memset(szBuf, 0, 1024);
        fgets(szBuf, 1024, fp);
        if (sMysqlConfig.empty())
        {
            if (getValue(strstr(szBuf, "mysql"), sizeof("mysql"), sMysqlConfig) == 0)
                continue;
        }

        if (sZkConfig.empty())
        {
            if (getValue(strstr(szBuf, "zookeeper"), sizeof("zookeeper"), sZkConfig) == 0)
                continue;
        }
    }
    delete szBuf;
    LOGI_BIZ(INIT) << "key : mysql, value : " << sMysqlConfig;
    LOGI_BIZ(INIT) << "key : zookeeper, value : " << sZkConfig;

    if (sMysqlConfig.empty() || sZkConfig.empty())
    {
        LOGE_BIZ(INIT) << "read config failed, file :'./conf/shmconfig_server.cfg'";
        return -1;
    }
    return 0;
}

int ConfigServer::getValue(const char* pszKey, int iKeyLen, std::string& sStr)
{
    if (!pszKey)
        return -1;

    pszKey += iKeyLen;
    while(*pszKey)
    {
        if (*pszKey == ' ' || *pszKey == '=' || *pszKey == '\t')
            ++ pszKey;
        break;
    }

    if (*pszKey == 0 || *pszKey == '\r' || *pszKey == '\n')
        return -1;

    const char* s = pszKey;
    while (*pszKey != 0 && *pszKey != '\r' && *pszKey != '\n')
        ++pszKey;

    sStr.append(s, pszKey - s);
    return 0;
}

zplugin::CPluginBase *SoPlugin()
{
    return new ConfigServer();
}

int ConfigServer::Add(CControllerBase* pController, std::string* pMessage)
{
    dao::ShmConfigTable* pConf = new dao::ShmConfigTable;
    LOGI_BIZ(ADD) << *pMessage;
    if(CheckAdd(pController, pMessage, pConf) < 0)
    {
        delete pConf;
        return -1;
    }

    if (pConf->status == 2)
    {
        if (AddZk(pConf) < 0)
        {
            delete pConf;
            WriteError(pController, -1, "add zk failed");
            return -1;
        }
    }

    int iRet = m_oDao.Add(pConf);
    delete pConf;
    if (iRet < 0)
    {
        WriteError(pController, -1, "add failed");
        return -1;
    }
    WriteError(pController, 0, "success");
    return 0;
}

int ConfigServer::Del(CControllerBase* pController, std::string* pMessage)
{
    LOGI_BIZ(DEL) << *pMessage;
    if (pMessage->empty())
    {
        WriteError(pController, -1, "not path");
        return -1;
    }
    std::string sKey;
    std::string sAuthor;
    getParam(pMessage->c_str(), "key", sKey);
    getParam(pMessage->c_str(), "author", sAuthor);
    if (sKey.empty() || sAuthor.empty())
    {
        WriteError(pController, -1, sKey.empty() ? "key empty" : "author empty");
        return -1;
    }

    int iRet = DeleteZk(sKey);
    if (iRet == -1)
    {
        WriteError(pController, -1, "system path");
        return -1;
    }

    if (m_oDao.Del(sKey, sAuthor) < 0)
    {
        WriteError(pController, -1, "delete db failed");
        return -1;
    }
    WriteError(pController, 0, "success");
    return 0;
}

int ConfigServer::Mod(CControllerBase* pController, std::string* pMessage)
{
    LOGI_BIZ(MOD) << *pMessage;
    dao::ShmConfigTable* pConf = new dao::ShmConfigTable;
    if(CheckAdd(pController, pMessage, pConf) < 0)
    {
        delete pConf;
        return -1;
    }

    if (pConf->status == 2)
    {
        int iRet = IsExist(pConf);
        if (iRet == -1)
        {
            delete pConf;
            WriteError(pController, -1, "system failed");
            return -1;
        }

        if (iRet == 0)
        {
            iRet = UpdateZk(pConf);
            if (iRet < 0)
            {
                delete pConf;
                WriteError(pController, -1, "update zk failed");
                return -1;
            }
        }
        else
        {
            if (AddZk(pConf) < 0)
            {
                delete pConf;
                WriteError(pController, -1, "add zk failed");
                return -1;
            }
        }
    }

    int iRet = m_oDao.Mod(pConf);
    delete pConf;
    if (iRet < 0)
    {
        WriteError(pController, -1, "update db failed");
        return -1;
    }

    WriteError(pController, 0, "success");
    return 0;
}

int ConfigServer::Query(CControllerBase* pController, std::string* pMessage)
{
    LOGI_BIZ(QUERY) << *pMessage;
    std::string sKey;
    dao::ConfigResp* pResp = new dao::ConfigResp;
    getParam(pMessage->c_str(), "key", sKey);
    if (sKey.empty())
    {
        std::string sPage;
        getParam(pMessage->c_str(), "page_no", sPage);
        if (!sPage.empty())
            pResp->page.page_no = strtol(sPage.c_str(), nullptr, 10);

        getParam(pMessage->c_str(), "page_size", sPage);
        if (!sPage.empty())
            pResp->page.page_size = strtol(sPage.c_str(), nullptr, 10);

        if (pResp->page.page_size == 0)
            pResp->page.page_size = 20;

        if (m_oDao.ConfigCount(pResp->page.total_num) < 0)
        {
            WriteError(pController, -1, "query db failed");
            delete pResp;
            return -1;
        }
    }

    if (!sKey.empty() || pResp->page.total_num != 0)
    {
        if (m_oDao.Query(sKey, pResp->page.page_no, pResp->page.page_size, pResp->data) < 0)
        {
            WriteError(pController, -1, "query db failed");
            delete pResp;
            return -1;
        }
    }

    pResp->page.total_page = pResp->page.total_num / pResp->page.page_size;
    if (pResp->page.total_num % pResp->page.page_size)
        pResp->page.total_page += 1;

    serialize::CJson oJson;
    oJson << *pResp;
    delete pResp;

    sKey = oJson.GetJson();
    CHttpController* pHttp = dynamic_cast<CHttpController*>(pController);
    pHttp->WriteResp(sKey.c_str(), sKey.length(), 200, 0, 3000, true);
    return 0;
}

int ConfigServer::SyncAllShmConfig(CControllerBase* pController, std::string* pMessage)
{
    std::vector<dao::ShmConfigTable> vConf;
    if (m_oDao.GetAllConfig(vConf) < 0)
    {
        WriteError(pController, -1, "query db failed");
        return -1;
    }

    for (auto it = vConf.begin(); it != vConf.end(); ++ it)
    {
        if (AddZk(&(*it)) < 0)
        {
            WriteError(pController, -1, "add zk failed");
            return -1;
        }
    }
    WriteError(pController, 0, "");
    return 0;
}

void ConfigServer::WriteError(CControllerBase* pController, int iCode, std::string sMsg)
{
    CHttpController* pHttp = dynamic_cast<CHttpController*>(pController);
    std::string sBody = "{\"code\":";
    sBody.append(std::to_string(iCode)).append(",\"msg\":\"").append(sMsg).append("\",\"data\":[]}");
    pHttp->WriteResp(sBody.c_str(), sBody.length(), 200, iCode, 3000, true);
}

int ConfigServer::CheckAdd(CControllerBase* pController, std::string* pMessage, dao::ShmConfigTable* pConf)
{
    serialize::CJsonToObjet oJson(pMessage->c_str());
    if (oJson)
    {
        LOGW_BIZ(ADD) << "json : " << *pMessage;
        WriteError(pController, -1, "json error");
        return -1;
    }
    oJson >> *pConf;

    if (pConf->key.empty() || pConf->key.length() >= 64)
    {
        LOGW_BIZ(ADD) << "key > 64 : " << pConf->key;
        WriteError(pController, -1, "check key > 64");
        return -1;
    }

    if (pConf->value.length() >= 4096)
    {
        LOGW_BIZ(ADD) << "value > 4096 : " << pConf->value;
        WriteError(pController, -1, "check value > 4096");
        return -1;
    }
    return 0;
}

int ConfigServer::AddZk(dao::ShmConfigTable* pConf)
{
    zkproto::zk_acl ac;
    ZK_ACL_OPEN(ac);
    std::vector<zkproto::zk_acl> acl;
    acl.push_back(ac);
    std::string sRes;
    std::string sKey = SHM_CONFIG_ROOT;
    sKey.append(pConf->key);
    int iRet = m_pZkApi->Create(sKey.c_str(), pConf->value, &acl, 0, sRes);
    if (iRet < 0)
    {
        LOGW_BIZ(ADD) << "add zk failed : " << pConf->key;
    }
    return iRet;
}

int ConfigServer::IsExist(dao::ShmConfigTable* pConf)
{
    zkproto::zk_stat stat;
    std::string sKey = SHM_CONFIG_ROOT;
    sKey.append(pConf->key);
    return m_pZkApi->Exists(sKey.c_str(), 0, &stat);
}

int ConfigServer::UpdateZk(dao::ShmConfigTable* pConf)
{
    zkproto::zk_stat stat;
    std::string sKey = SHM_CONFIG_ROOT;
    sKey.append(pConf->key);
    return m_pZkApi->SetData(sKey.c_str(), pConf->value, -1, &stat);
}

int ConfigServer::DeleteZk(std::string& sKey)
{
    std::string sPath = SHM_CONFIG_ROOT;
    sPath.append(sKey);
    return m_pZkApi->Delete(sPath.c_str(), -1);
}

const char* ConfigServer::getParam(const char* pszParam, const char* pszKey, std::string& sStr)
{
    if (!pszParam)
        return nullptr;

    const char* s = strstr(pszParam, pszKey);
    if (!s)
        return nullptr;

    s += strlen(pszKey);
    if (*s != '=')
        return nullptr;
    ++s;

    const char* e = s;
    while (*e && *e != '&') ++e;

    sStr.append(s, e - s);

    if (*e == '&') ++e;

    return e;
}
