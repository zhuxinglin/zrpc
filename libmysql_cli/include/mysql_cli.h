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

#ifndef __MYSQL_CLI_H__
#define __MYSQL_CLI_H__

#include "mysql/mysql.h"
#include <string>

namespace mysqlcli
{

class MySqlResult;
class MysqlCli
{
public:
    MysqlCli();
    ~MysqlCli();

public:
    // pszConnectInfo = "用户名:密码@tcp(IP:端口)/数据库?charset=utf8"
    int Connect(const char* pszConnectInfo);
    void Close();
    int64_t Query(const std::string& sSql, MySqlResult* pResult);
    const char* GetErr() const {return m_sErr.c_str();}
    int BeginCommit();
    int EndCommit();
    int Commit();
    int Rollback();
    std::string GetEscapeString(const char* s, int len);

private:
    const char* getConnectInfo(const char* s, char ch, std::string& sInfo, bool bIsCheck = false);

private:
    MYSQL* m_pMySql;
    std::string m_sErr;
};

}

#endif
