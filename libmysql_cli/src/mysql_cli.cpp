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

#include "../include/mysql_cli.h"
#include "../include/mysql_helper.h"
#include <string.h>

using namespace mysqlcli;


MysqlCli::MysqlCli():m_pMySql(nullptr)
{
}

MysqlCli::~MysqlCli()
{
}

// pszConnectInfo = "用户名:密码@tcp(IP:端口)/数据库?charset=utf8"
int MysqlCli::Connect(const char* pszConnectInfo)
{
    if (m_pMySql || !pszConnectInfo)
    {
        m_sErr = "check param error";
        return -1;
    }
    std::string sUser;
    std::string sPasswrod;
    std::string sProto;
    std::string sAddr;
    std::string sDb;
    std::string sPort;
    std::string sCharset;

    const char* s = getConnectInfo(pszConnectInfo, ':', sUser);
    s = getConnectInfo(s, '@', sPasswrod, true);
    s = getConnectInfo(s, '(', sProto);
    if (sProto.compare("tcp") == 0)
    {
        s = getConnectInfo(s, ':', sAddr);
        if (sAddr.empty())
            sAddr = "127.0.0.1";
        s = getConnectInfo(s, ')', sPort);
        if (sPort.empty())
            sPort = "3306";
    }
    else
    {
        s = getConnectInfo(s, ')', sAddr);
        if (sAddr.empty())
            sAddr = "/tmp/mysql.sock";
    }
    if (*s == '/') ++ s;
    s = getConnectInfo(s, '?', sDb);
    s = getConnectInfo(s, 0, sCharset);
    m_pMySql = mysql_init(NULL);

    if (!mysql_real_connect(m_pMySql, sProto.compare("tcp") == 0 ? sAddr.c_str() : nullptr, sUser.c_str(), 
        sPasswrod.c_str(), sDb.c_str(), sPort.empty() ? 0 : atoi(sPort.c_str()), 
        sProto.compare("tcp") == 0 ? nullptr : sAddr.c_str(), 0))
    {
        m_sErr = mysql_error(m_pMySql);
        return -1;
    }

    std::size_t iPos = sCharset.find('=');
    if (iPos == std::string::npos)
        sCharset = "";

    sCharset = sCharset.substr(++ iPos);

    if (!sCharset.empty())
        mysql_set_character_set(m_pMySql, sCharset.c_str());

    return 0;
}

const char* MysqlCli::getConnectInfo(const char* s, char sh, std::string& sInfo, bool bIsCheck)
{
    const char* e = s;
    do
    {
        while (*e != sh && *e)
            ++ e;

        if (bIsCheck)
        {
            if (strchr(e + 1, sh))
            {
                ++ e;
                continue;
            }
            break;
        }
    }while (bIsCheck);

    if ((e - s) == 0)
        return e;

    if (e != s)
        sInfo.append(s, e - s);

    if (*e)
        ++ e;
    return e;
}

void MysqlCli::Close()
{
    if (m_pMySql)
        mysql_close(m_pMySql);
    m_pMySql = nullptr;
}

int64_t MysqlCli::Query(const std::string& sSql, MySqlResult* pResult)
{
    if (sSql.empty() || !pResult)
    {
        m_sErr = "check param failed";
        return -1;
    }

    int iRet = mysql_real_query(m_pMySql, sSql.c_str(), sSql.length());
    if (iRet < 0)
    {
        mysql_ping(m_pMySql);
        iRet = mysql_real_query(m_pMySql, sSql.c_str(), sSql.length());
        if (iRet < 0)
        {
            m_sErr = mysql_error(m_pMySql);
            return -1;
        }
    }

    MYSQL_RES* pRes = mysql_store_result(m_pMySql);
    if (pRes)
    {
        int64_t rows = mysql_num_rows(pRes);
        pResult->SetResult(pRes, rows);
        return rows;
    }
    else if (mysql_field_count(m_pMySql) == 0)
    {
        int64_t id = mysql_insert_id(m_pMySql);
        if (id != 0)
            return id;
        return mysql_affected_rows(m_pMySql);
    }
    else
    {
        m_sErr = mysql_error(m_pMySql);
        return -1;
    }

    return 0;
}

int MysqlCli::BeginCommit()
{
    int iRet = mysql_autocommit(m_pMySql, 0);
    if (iRet < 0)
        m_sErr = mysql_error(m_pMySql);
    return iRet;
}

int MysqlCli::EndCommit()
{
    int iRet = mysql_autocommit(m_pMySql, 1);
    if (iRet < 0)
        m_sErr = mysql_error(m_pMySql);
    return iRet;
}

int MysqlCli::Commit()
{
    int iRet = mysql_commit(m_pMySql);
    if (iRet < 0)
        m_sErr = mysql_error(m_pMySql);
    return iRet;
}

int MysqlCli::Rollback()
{
    int iRet = mysql_rollback(m_pMySql);
    if (iRet < 0)
        m_sErr = mysql_error(m_pMySql);
    return iRet;
}

std::string MysqlCli::GetEscapeString(const char* s, int len)
{
    char* pStr = new char[len * 2 + 2];
    if (!pStr)
        return "";

    char* e = pStr;
    *e++ = '\'';
    e += mysql_real_escape_string(m_pMySql, e, s, len);
    *e ++ = '\'';
    *e = 0;
    std::string sStr(pStr, e - pStr);
    delete pStr;
    return sStr;
}
