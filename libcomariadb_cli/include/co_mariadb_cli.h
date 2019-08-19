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

#ifndef __CO_MARIADB_CLI__
#define __CO_MARIADB_CLI__

namespace comaria
{

struct CoMariaDbCli
{
    static CoMariaDbCli* CreateObj();

public:
    // pszConnectInfo = "用户名:密码@tcp(IP:端口)/数据库?charset=utf8"
    virtual int Connect(const char* pszConnectInfo) = 0;
};


}


#endif
