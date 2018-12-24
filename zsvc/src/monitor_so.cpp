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

#include "monitor_so.h"
#include <sys/inotify.h>
#include <unistd.h>
#include "so_plugin.h"
#include <so_struct.h>
#include "so_uninstall.h"
#include "log.h"

extern uint32_t g_dwSoUninstallInterval;

CMonitorSo::CMonitorSo() : m_iFd(-1),
                           m_iIw(-1),
                           m_pNet(0)
{
}

CMonitorSo::~CMonitorSo()
{
    if (m_iIw != -1)
    {
        inotify_rm_watch(m_iFd, m_iIw);
        close(m_iIw);
    }
    m_iIw = -1;

    if (m_iFd != -1)
        close(m_iFd);
    m_iFd = -1;
}

int CMonitorSo::InitMonitorDir(const char *pszDir, CNet *pNet)
{
    m_iFd = inotify_init1(IN_NONBLOCK);
    if (m_iFd < 0)
    {
        LOGE << "create inotify init fail";
        return -1;
    }

    m_sSoPath = pszDir;
    m_iIw = inotify_add_watch(m_iFd, pszDir, IN_MOVED_TO | IN_CLOSE_WRITE | IN_DELETE);
    if (m_iIw < 0)
    {
        LOGE << "add inotify dir fail";
        return -1;
    }

    m_pNet = pNet;

    return 0;
}

int CMonitorSo::Start(CSoPlugin *pPlugin)
{
    int iRet = m_pNet->Register(this, pPlugin, ITaskBase::PROTOCOL_TIMER, m_iFd, -1);
    if (iRet < 0)
    {
        LOGE << m_pNet->GetErr();
        return -1;
    }
    return 0;
}

void CMonitorSo::Run()
{
    struct inotify_event ie[50];
    CSoPlugin *pSoPlugin = (CSoPlugin*)m_pData;
    map_so_info* pSoInfo;
    while(true)
    {
        pSoInfo = 0;
        int iLen = read(m_iFd, ie, sizeof(ie));
        if (iLen == -1 && errno == EAGAIN)
        {
            Yield();
            continue;
        }

        for (int i = 0; i < iLen / (int)sizeof(struct inotify_event); ++ i)
        {
            struct inotify_event *pIe = &ie[i];
            if (pIe->len == 0)
                continue;

            if (pIe->mask & (IN_MOVED_TO | IN_CLOSE_WRITE))
            {
                if (0 == strstr(pIe->name, ".so.V"))
                    continue;

                if (!pSoInfo)
                    pSoInfo = new map_so_info;
                std::string sSoPath(m_sSoPath);
                sSoPath.append(pIe->name);
                if (pSoPlugin->UpdateSo(sSoPath.c_str(), pSoInfo) < 0)
                {
                    LOGE << "load '" << pIe->name << "' fail";
                }

                if (pSoInfo->size() == 0)
                {
                    delete pSoInfo;
                    pSoInfo = 0;
                    continue;
                }
            }
            else if (IN_DELETE & pIe->mask)
            {
                if (!strstr(pIe->name, ".so.V"))
                    continue;

                if (!pSoInfo)
                    pSoInfo = new map_so_info;

                std::string sSoPath(m_sSoPath);
                sSoPath.append(pIe->name);

                if (pSoPlugin->Uninstall(pSoInfo, sSoPath.c_str()) < 0)
                {
                    delete pSoInfo;
                    pSoInfo = 0;
                    continue;
                }
            }
        }

        if (!pSoInfo)
            continue;

        pSoPlugin->Swap(&pSoInfo);
        CSoUninstall* pSoUninstall = new CSoUninstall();
        pSoUninstall->SetSoMap(pSoInfo);
        pSoUninstall->SetPluginObj(pSoPlugin);
        m_pNet->Register(pSoUninstall, pSoPlugin, ITaskBase::PROTOCOL_TIMER, -1, g_dwSoUninstallInterval * 1e3);
    }
}