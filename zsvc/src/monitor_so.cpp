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
#include "so_struct.h"
#include "so_uninstall.h"
#include "log.h"

using namespace zrpc;
using namespace znet;

extern uint32_t g_dwSoUninstallInterval;

CMonitorSo::CMonitorSo() : m_iFd(-1),
                           m_iIw(-1)
{
    m_wRunStatus = RUN_NOW;
    m_sCoName = "monitor_so";
}

CMonitorSo::~CMonitorSo()
{
    // if (m_iIw != -1)
    //     inotify_rm_watch(m_iFd, m_iIw);
    m_iIw = -1;

    if (m_iFd != -1)
        close(m_iFd);
    m_iFd = -1;
}

int CMonitorSo::InitMonitorDir(const char *pszDir)
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
    return 0;
}

int CMonitorSo::Start(CSoPlugin *pPlugin)
{
    int iRet = CNet::GetObj()->Register(this, pPlugin, ITaskBase::PROTOCOL_TIMER, m_iFd, -1);
    if (iRet < 0)
    {
        LOGE << CNet::GetObj()->GetErr();
        return -1;
    }
    return 0;
}

void CMonitorSo::Run()
{
    LoadSo();
    MonitorLoadSo();
}

void CMonitorSo::LoadSo()
{
    CSoPlugin *pSoPlugin = (CSoPlugin*)m_pData;
    // 加载插件
    pSoPlugin->LoadSo(m_sSoPath.c_str());
}

void CMonitorSo::MonitorLoadSo()
{
    char *ie = new char[1024];
    CSoPlugin *pSoPlugin = (CSoPlugin*)m_pData;
    map_so_info* pSoInfo;
    while(m_bIsExit)
    {
        pSoInfo = 0;
        memset(ie, 0, 1024);
        int iLen = read(m_iFd, ie, 1024);
        if (iLen == -1 && errno == EAGAIN)
        {
            Yield();
            continue;
        }

        for (struct inotify_event *pIe = reinterpret_cast<struct inotify_event *>(ie);
             reinterpret_cast<char*>(pIe) < (ie + iLen);)
        {
            do
            {
                if (pIe->len == 0)
                    break;

                if (pIe->mask & (IN_MOVED_TO | IN_CLOSE_WRITE))
                {
                    if (0 == strstr(pIe->name, SO_VERSION))
                        break;

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
                        break;
                    }
                }
                else if (IN_DELETE & pIe->mask)
                {
                    if (!strstr(pIe->name, SO_VERSION))
                        break;

                    if (!pSoInfo)
                        pSoInfo = new map_so_info;

                    std::string sSoPath(m_sSoPath);
                    sSoPath.append(pIe->name);

                    if (pSoPlugin->Uninstall(pSoInfo, sSoPath.c_str()) < 0)
                    {
                        delete pSoInfo;
                        pSoInfo = 0;
                        break;
                    }
                }
            } while (0);
            pIe += pIe->len;
        }

        if (!pSoInfo)
            continue;

        pSoPlugin->Swap(&pSoInfo);
        CSoUninstall* pSoUninstall = new CSoUninstall();
        pSoUninstall->SetSoMap(pSoInfo);
        pSoUninstall->SetPluginObj(pSoPlugin);
        CNet::GetObj()->Register(pSoUninstall, pSoPlugin, ITaskBase::PROTOCOL_TIMER, -1, g_dwSoUninstallInterval * 1e3);
    }
    delete []ie;
}

void CMonitorSo::Stop(CSoPlugin *pPlugin)
{
    LOGI << "monitorso exit ..............";
    m_bIsExit = false;
    map_so_info* pSoInfo = pPlugin->DelAll();
    if (!pSoInfo)
        return;
    CSoUninstall* pSoUninstall = new CSoUninstall();
    pSoUninstall->SetSoMap(pSoInfo);
    pSoUninstall->SetPluginObj(pPlugin);
    CNet::GetObj()->Register(pSoUninstall, pPlugin, ITaskBase::PROTOCOL_TIMER, -1, 0);
    // 等待退出
    while (pPlugin->IsExit()) usleep(10000);
    CNet::GetObj()->ExitCo(m_qwCid);
    usleep(100);
}