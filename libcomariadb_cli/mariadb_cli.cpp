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
*
*
 */

#include "mariadb_cli.h"

using namespace comaria;

MariaDbDli::MariaDbDli() : m_pNetClient(nullptr)
{
}

MariaDbDli::~MariaDbDli()
{
}

// pszConnectInfo = "用户名:密码@tcp(IP:端口)/数据库?charset=utf8"
int MariaDbDli::Connect(const char *pszConnectInfo)
{
    return 0;
}
