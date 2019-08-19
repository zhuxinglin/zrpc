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

#ifndef __MARIADB_CLI_H__
#define __MARIADB_CLI_H__

#include "include/co_mariadb_cli.h"
#include <libnet.h>
#include <net_client.h>

namespace comaria
{

class MariaDbDli : public CoMariaDbCli
{
public:
    MariaDbDli();
    ~MariaDbDli();

private:
    // pszConnectInfo = "用户名:密码@tcp(IP:端口)/数据库?charset=utf8"
    virtual int Connect(const char *pszConnectInfo);

private:
    znet::CNetClient* m_pNetClient;
};

}

#endif
