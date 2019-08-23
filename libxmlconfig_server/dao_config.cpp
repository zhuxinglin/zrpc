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

#include "dao_config.h"
#include "log.h"
#include <sys/time.h>

using namespace dao;

DaoConfig::DaoConfig()
{
}

DaoConfig::~DaoConfig()
{
    for (int i = 0; i < DB_CONNECT_COUNT; ++ i)
    {
        if (m_oMysql[i].pSqlCli)
        {
            m_oMysql[i].pSqlCli->Close();
            delete m_oMysql[i].pSqlCli;
            m_oMysql[i].pSqlCli = nullptr;
        }
    }
}

int DaoConfig::Init(const char* pszDbAddr)
{
    for (int i = 0; i < DB_CONNECT_COUNT; ++ i)
    {
        m_oMysql[i].pSqlCli = new mysqlcli::MysqlCli;
        if (m_oMysql[i].pSqlCli->Connect(pszDbAddr) < 0)
        {
            LOGE_BIZ() << m_oMysql[i].pSqlCli->GetErr();
            return -1;
        }
    }
    return 0;
}

int DaoConfig::Add(const XmlConfigTable* pConf)
{
    if (!pConf)
        return -1;

    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);
    std::string sSql = "insert into xml_config(xml_key1,xml_key2,xml_key3,xml_key4,xml_value,status,del_flag,create_time,update_time,author,description)"
                        "values(?,?,?,?,?,?,?,?,?,?,?)";
    oHelper.SetString(pConf->key1);
    oHelper.SetString(pConf->key2);
    oHelper.SetString(pConf->key3);
    oHelper.SetString(pConf->key4);
    oHelper.SetString(pConf->value);
    oHelper.SetValueI(pConf->status);
    oHelper.SetValueI(pConf->del_flag);
    oHelper.SetValueI(getTimeMs());
    oHelper.SetValueI(getTimeMs());
    oHelper.SetString(pConf->author);
    oHelper.SetString(pConf->description);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query();
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

int DaoConfig::Mod(XmlConfigTable* pConf)
{
    if (!pConf || pConf->key1.empty() || pConf->key2.empty())
        return -1;
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "update xml_config set xml_value=?,status=?,update_time=?,author=?";
    if (!pConf->description.empty())
        sSql.append(",description=?");
    sSql.append(" where xml_key1=? and xml_key2=? and xml_key3=? and xml_key4=? and del_flag=0");

    oHelper.SetString(pConf->value);
    oHelper.SetValueI(pConf->status);
    oHelper.SetValueI(getTimeMs());
    oHelper.SetString(pConf->author);
    if (!pConf->description.empty())
        oHelper.SetString(pConf->description);
    oHelper.SetString(pConf->key1);
    oHelper.SetString(pConf->key2);
    oHelper.SetString(pConf->key3);
    oHelper.SetString(pConf->key4);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query();
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

int DaoConfig::Del(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, const std::string& sAuthor)
{
    if (sKey1.empty() || sKey2.empty())
        return -1;

    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "update xml_config set del_flag=1,update_time=?,author=? where xml_key1=? and xml_key2=? and xml_key3=? and xml_key4=?";

    oHelper.SetValueI(getTimeMs());
    oHelper.SetString(sAuthor);
    oHelper.SetString(sKey1);
    oHelper.SetString(sKey2);
    oHelper.SetString(sKey3);
    oHelper.SetString(sKey4);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query();
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

// PageNo uint32 `json:"page_no"`
// PageSize uint32 `json:"page_size"`
// TotalNum uint32 `json:"total_num"`
// TotalPage uint32 `json:"total_page"`
int DaoConfig::Query(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, 
                    uint32_t dwPageNo, uint32_t dwPageSize, std::vector<XmlConfigTable>& vConf)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select id,xml_key1,xml_key2,xml_key3,xml_key4,xml_value,status,del_flag,create_time,update_time,author,description from xml_config where del_flag=0";
    if (sKey1.empty())
    {
        sSql.append(" limit ?, ?");
        oHelper.SetValueI(dwPageNo == 1 ? 0 : dwPageNo * dwPageSize);
        oHelper.SetValueI(dwPageSize);
    }
    else
    {
        setKey(sSql, sKey1, sKey2, sKey3, sKey4, oHelper);
    }

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query(vConf);
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

int DaoConfig::Query(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, std::vector<XmlConfigInitInfo>& vConf)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select xml_key1,xml_key2,xml_key3,xml_key4,xml_value from xml_config where status=2 and del_flag=0";
    setKey(sSql, sKey1, sKey2, sKey3, sKey4, oHelper);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);
    int64_t iRet = oHelper.Query(vConf);
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

DaoConfig::ManageMysqlPool* DaoConfig::getMysqlCliObj()
{
    for (int i = 0; i < DB_CONNECT_COUNT; ++ i)
    {
        char16_t wRef = 0;
        if (m_oMysql[i].wRef.compare_exchange_weak(wRef, static_cast<char16_t>(1)))
            return &m_oMysql[i];
    }
    return nullptr;
}

uint64_t DaoConfig::getTimeMs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int DaoConfig::ConfigCount(uint32_t& dwSumCount, const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select count(*) from xml_config where del_flag=0";
    setKey(sSql, sKey1, sKey2, sKey3, sKey4, oHelper);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query(dwSumCount);
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

void DaoConfig::setKey(std::string& sSql, const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, mysqlcli::MySqlHelper& oHelper)
{
    if (!sKey1.empty())
    {
        sSql.append(" and xml_key1=?");
        oHelper.SetString(sKey1);
    }
    if (!sKey2.empty())
    {
        sSql.append(" and xml_key2=?");
        oHelper.SetString(sKey2);
    }
    if (!sKey3.empty())
    {
        sSql.append(" and xml_key3=?");
        oHelper.SetString(sKey3);
    }
    if (!sKey4.empty())
    {
        sSql.append(" and xml_key4=?");
        oHelper.SetString(sKey4);
    }
}
