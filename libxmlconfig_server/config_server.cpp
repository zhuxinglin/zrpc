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

#define URL_QUERY_LIST  "/xml/config/querylist"
#define URL_MOD_CONFIG  "/xml/config/mod"
#define URL_ADD_CONFIG  "/xml/config/add"
#define URL_DEL_CONFIG  "/xml/config/del"
#define URL_QUERY_KEY_LIST  "/xml/config/query"

#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"

ConfigServer::ConfigServer()
{
}

ConfigServer::~ConfigServer()
{
}

int ConfigServer::Initialize(znet::CLog *pLog, znet::CNet *pN, CSharedData *pProc, CSharedData* pSo)
{
    znet::CLog::SetObj(pLog);
    znet::CNet::Set(pN);
    LOGI_BIZ(INIT) << "xml config server";
    std::string sMysqlConfig;

    if (getConnectConfig(sMysqlConfig) < 0)
        return -1;

    if (m_oDao.Init(sMysqlConfig.c_str()) < 0)
    {
        LOGE_BIZ(INIT) << "init mysql failed, connect : " << sMysqlConfig;
        return -1;
    }

    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_QUERY_LIST, sizeof(URL_QUERY_LIST) - 1), &ConfigServer::Query));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_MOD_CONFIG, sizeof(URL_MOD_CONFIG) - 1), &ConfigServer::Mod));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_ADD_CONFIG, sizeof(URL_ADD_CONFIG) - 1), &ConfigServer::Add));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_DEL_CONFIG, sizeof(URL_DEL_CONFIG) - 1), &ConfigServer::Del));
    m_mapFun.insert(MAP_FUN::value_type(CUtilHash::UriHash(URL_QUERY_KEY_LIST, sizeof(URL_QUERY_KEY_LIST) - 1), &ConfigServer::InitQuery));
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
    LOGI_BIZ(CLOSE) << "libxmlconfig_server exit .....";
	delete this;
}

int ConfigServer::getConnectConfig(std::string& sMysqlConfig)
{
    FILE* fp = fopen("./conf/xmlconfig_server.cfg", "rb");
    if (!fp)
    {
        LOGE_BIZ(INIT) << "open config fail './conf/xmlconfig_server.cfg' failed";
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
    }
    fclose(fp);
    delete szBuf;
    LOGI_BIZ(INIT) << "key : mysql, value : " << sMysqlConfig;

    if (sMysqlConfig.empty())
    {
        LOGE_BIZ(INIT) << "read config failed, file :'./conf/xmlconfig_server.cfg'";
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
    dao::XmlConfigTable* pConf = new dao::XmlConfigTable;
    LOGI_BIZ(ADD) << *pMessage;
    if(CheckAdd(pController, pMessage, pConf) < 0)
    {
        delete pConf;
        return -1;
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
    std::string sKey1;
    std::string sKey2;
    std::string sKey3;
    std::string sKey4;
    std::string sAuthor;
    getParam(pMessage->c_str(), "key1", sKey1);
    getParam(pMessage->c_str(), "key2", sKey2);
    getParam(pMessage->c_str(), "key3", sKey3);
    getParam(pMessage->c_str(), "key4", sKey4);
    getParam(pMessage->c_str(), "author", sAuthor);
    if (sKey1.empty() || sKey2.empty() || sAuthor.empty())
    {
        WriteError(pController, -1, (sKey1.empty() || sKey2.empty()) ? "key empty" : "author empty");
        return -1;
    }

    if (m_oDao.Del(sKey1, sKey2, sKey3, sKey4, sAuthor) < 0)
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
    dao::XmlConfigTable* pConf = new dao::XmlConfigTable;
    if(CheckAdd(pController, pMessage, pConf) < 0)
    {
        delete pConf;
        return -1;
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
    std::string sKey1;
    std::string sKey2;
    std::string sKey3;
    std::string sKey4;
    dao::ConfigResp<dao::XmlConfigTable>* pResp = new dao::ConfigResp<dao::XmlConfigTable>;
    getParam(pMessage->c_str(), "key", sKey1);
    if (sKey1.empty())
    {
        std::string sPage;
        getParam(pMessage->c_str(), "page_no", sPage);
        if (!sPage.empty())
            pResp->page.page_no = strtol(sPage.c_str(), nullptr, 10);

        sPage = "";
        getParam(pMessage->c_str(), "page_size", sPage);
        if (!sPage.empty())
            pResp->page.page_size = strtol(sPage.c_str(), nullptr, 10);

        if (pResp->page.page_size == 0)
            pResp->page.page_size = 20;
    }
    else
        splitKey(sKey1, sKey2, sKey3, sKey4);

    if (m_oDao.ConfigCount(pResp->page.total_num, sKey1, sKey2, sKey3, sKey4) < 0)
    {
        WriteError(pController, -1, "query db failed");
        delete pResp;
        return -1;
    }

    if (!sKey1.empty() || pResp->page.total_num != 0)
    {
        if (m_oDao.Query(sKey1, sKey2, sKey3, sKey4, pResp->page.page_no, pResp->page.page_size, pResp->data) < 0)
        {
            WriteError(pController, -1, "query db failed");
            delete pResp;
            return -1;
        }
    }

    if (pResp->page.page_size == 0)
    {
        pResp->page.page_size = 1;
        pResp->page.total_num = 1;
    }

    pResp->page.total_page = pResp->page.total_num / pResp->page.page_size;
    if (pResp->page.total_num % pResp->page.page_size)
        pResp->page.total_page += 1;

    serialize::CJson oJson;
    oJson << *pResp;
    delete pResp;

    sKey1 = oJson.GetJson();
    CHttpController* pHttp = dynamic_cast<CHttpController*>(pController);
    pHttp->WriteResp(sKey1.c_str(), sKey1.length(), 200, 0, 3000, true);
    return 0;
}

int ConfigServer::InitQuery(CControllerBase* pController, std::string* pMessage)
{
    LOGI_BIZ(INIT_QUERY) << *pMessage;
    std::vector<std::string> vParam;
    getQueryParam(pController, pMessage, vParam);

    if (vParam.empty())
        return -1;

    std::vector<dao::XmlConfigInitInfo> vConf;
    for (auto it = vParam.begin(); it != vParam.end(); ++ it)
    {
        std::string sKey1 = *it;
        std::string sKey2;
        std::string sKey3;
        std::string sKey4;
        splitKey(sKey1, sKey2, sKey3, sKey4);

        m_oDao.Query(sKey1, sKey2, sKey3, sKey4, vConf);
    }

    dao::ConfigResp<dao::XmlConfigResp>* pResp = new dao::ConfigResp<dao::XmlConfigResp>;
    for (auto it = vConf.begin(); it != vConf.end(); ++ it)
    {
        dao::XmlConfigResp oResp;
        oResp.key.append(it->key1).append(".").append(it->key2);
        if (!it->key3.empty())
            oResp.key.append(".").append(it->key3);

        if (!it->key4.empty())
            oResp.key.append(".").append(it->key4);

        oResp.value = it->value;
        pResp->data.push_back(oResp);
    }

    serialize::CJson oJson;
    oJson << *pResp;
    delete pResp;

    std::string sKey = oJson.GetJson();
    LOGD_BIZ(INIT_QUERY) << "json : " << sKey;
    CHttpController* pHttp = dynamic_cast<CHttpController*>(pController);
    pHttp->WriteResp(sKey.c_str(), sKey.length(), 200, 0, 3000, true);
    return 0;
}

void ConfigServer::WriteError(CControllerBase* pController, int iCode, std::string sMsg)
{
    CHttpController* pHttp = dynamic_cast<CHttpController*>(pController);
    std::string sBody = "{\"code\":";
    sBody.append(std::to_string(iCode)).append(",\"msg\":\"").append(sMsg).append("\",\"data\":[]}");
    pHttp->WriteResp(sBody.c_str(), sBody.length(), 200, iCode, 3000, true);
}

int ConfigServer::CheckAdd(CControllerBase* pController, std::string* pMessage, dao::XmlConfigTable* pConf)
{
    serialize::CJsonToObjet oJson(pMessage->c_str());
    if (oJson)
    {
        LOGW_BIZ(ADD) << "json : " << *pMessage;
        WriteError(pController, -1, "json error");
        return -1;
    }
    oJson >> *pConf;

    if (pConf->key1.empty() || pConf->key1.length() >= 64 || pConf->key2.empty() || pConf->key2.length() >= 64)
    {
        LOGW_BIZ(ADD) << "key1 or key2, empty or length > 64 : " << pConf->key1 << " or " << pConf->key2;
        WriteError(pController, -1, "check key > 64");
        return -1;
    }
    return 0;
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

void ConfigServer::splitKey(std::string& sKey1, std::string& sKey2, std::string& sKey3, std::string& sKey4)
{
    std::string sKey;
    auto split = [](const char* s, std::string& sKey) -> const char*
    {
        if (!s)
            return nullptr;

        const char* e = s;
        while (*e && *e != '.') ++e;
        sKey.append(s, e - s);

        if (*e == '.')
            ++e;

        if (*e == 0)
            return nullptr;

        return e;
    };

    const char* s = sKey1.c_str();
    s = split(s, sKey);
    s = split(s, sKey2);
    s = split(s, sKey3);
    s = split(s, sKey4);
    sKey1 = sKey;
}

void ConfigServer::getQueryParam(CControllerBase* pController, std::string* pMessage, std::vector<std::string>& vParam)
{
    std::string sKey;
    getParam(pMessage->c_str(), "key", sKey);
    serialize::CJsonToObjet oJson(sKey.c_str());
    if (oJson)
    {
        LOGW_BIZ(QUERY) << "json : " << sKey;
        WriteError(pController, -1, "json error");
        return ;
    }
    oJson >> vParam;

    if (vParam.empty())
    {
        LOGW_BIZ(ADD) << "Parse param empty, json : " << *pMessage;
        WriteError(pController, -1, "json error");
        return ;
    }

    return;
}
