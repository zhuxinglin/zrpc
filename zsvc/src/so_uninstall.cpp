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
    m_sCoName = "so_uninstall";
}

CSoUninstall::~CSoUninstall()
{
    if (m_pmapSo)
        delete m_pmapSo;
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
        if (m_pmapSo && m_pPlugin->Del(m_pmapSo, true) < 0)
        {
            Yield(10);
            continue;
        }
        break;
    } while (1);
}

void CSoUninstall::Release()
{
    delete this;
}

