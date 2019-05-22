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

#include "so_uninstall.h"
#include "so_plugin.h"
#include <stdio.h>

using namespace zrpc;

CSoUninstall::CSoUninstall() : m_pPlugin(0), m_pmapSo(0)
{
}

CSoUninstall::~CSoUninstall()
{
}

void CSoUninstall::SetPluginObj(CSoPlugin *pPlugin)
{
    m_pPlugin = pPlugin;
}

void CSoUninstall::SetSoMap(map_so_info *pmapSo)
{
    m_pmapSo = pmapSo;
}

void CSoUninstall::Run()
{
    
    do
    {
        if (m_pPlugin->Del(m_pmapSo, true) < 0)
            Yield();
    } while (1);
    delete m_pmapSo;
}

void CSoUninstall::Release()
{
    delete this;
}

