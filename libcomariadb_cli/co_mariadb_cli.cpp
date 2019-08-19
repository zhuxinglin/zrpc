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

#include "include/co_mariadb_cli.h"
#include "mariadb_cli.h"

using namespace comaria;

CoMariaDbCli* CoMariaDbCli::CreateObj()
{
    return new MariaDbDli();
}


