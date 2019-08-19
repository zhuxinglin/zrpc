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

int MariaDbDli::Connect(const char *pszConnectInfo)
{
    return 0;
}
