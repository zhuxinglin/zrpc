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
            LOGE_BIZ(INIT) << m_oMysql[i].pSqlCli->GetErr();
            return -1;
        }
    }
    return 0;
}

int DaoConfig::Add(const ShmConfigTable* pConf)
{
    if (!pConf)
        return -1;

    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);
    std::string sSql = "insert into shm_config(shm_key,shm_value,status,del_flag,create_time,update_time,author,description)values(?,?,?,?,?,?,?,?)";
    oHelper.SetString(pConf->key);
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

int DaoConfig::Mod(ShmConfigTable* pConf)
{
    if (!pConf || pConf->key.empty())
        return -1;
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "update shm_config set shm_value=?,status=?,update_time=?,author=?";
    if (!pConf->description.empty())
        sSql.append(",description=?");
    sSql.append(" where shm_key=? and del_flag=0");

    oHelper.SetString(pConf->value);
    oHelper.SetValueI(pConf->status);
    oHelper.SetValueI(getTimeMs());
    oHelper.SetString(pConf->author);
    if (!pConf->description.empty())
        oHelper.SetString(pConf->description);
    oHelper.SetString(pConf->key);

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query();
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

int DaoConfig::Del(const std::string& sKey, const std::string& sAuthor)
{
    if (sKey.empty())
        return -1;

    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "update shm_config set del_flag=1,update_time=?,author=? where shm_key=?";

    oHelper.SetValueI(getTimeMs());
    oHelper.SetString(sAuthor);
    oHelper.SetString(sKey);

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
int DaoConfig::Query(const std::string& sKey, uint32_t dwPageNo, uint32_t dwPageSize, std::vector<ShmConfigTable>& vConf)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select id,shm_key,shm_value,status,del_flag,create_time,update_time,author,description from shm_config where del_flag=0 ";
    if (sKey.empty())
    {
        sSql.append("order by update_time desc limit ?, ?");
        oHelper.SetValueI(dwPageNo * dwPageSize);
        oHelper.SetValueI(dwPageSize);
    }
    else
    {
        sSql.append("and shm_key=?");
        oHelper.SetString(sKey);
    }

    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query(vConf);
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}

int DaoConfig::GetAllConfig(std::vector<ShmConfigTable>& vConf)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select id,shm_key,shm_value,status,del_flag,create_time,update_time,author,description from shm_config where del_flag=0 and status=2";
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

int DaoConfig::ConfigCount(uint32_t& dwSumCount)
{
    ManageMysqlPool* pCli = getMysqlCliObj();
    if (!pCli)
        return -1;

    mysqlcli::MySqlHelper oHelper(pCli->pSqlCli);

    std::string sSql = "select count(*) from shm_config where del_flag=0";
    LOGI_BIZ(DAO) << oHelper.GenerateSql(sSql);

    int64_t iRet = oHelper.Query(dwSumCount);
    if(iRet < 0)
        LOGE_BIZ(DAO) << oHelper.GetErr();
    pCli->wRef = 0;
    return static_cast<int>(iRet);
}
