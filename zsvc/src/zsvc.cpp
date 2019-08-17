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

#include "zsvc.h"
#include "http_svc.h"
#include <socket_fd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "binary_svc.h"
#include "log.h"
#include "libnet.h"

using namespace zrpc;
using namespace znet;

std::string g_oPluginPath("./plugin");
uint32_t g_dwSoUninstallInterval = 0;

CJSvc::CJSvc()
{
    CNet::GetObj();
}

CJSvc::~CJSvc()
{
    CNet::Release();
}

int CJSvc::Init(CConfig *pCfg)
{
    CConfig::config_info *mapConfi;
    int iKey = 0;
    while ((mapConfi = pCfg->GetConfig(iKey++)))
    {
        if (iKey == 1)
        {
            if (InitGlobal(mapConfi) < 0)
                return -1;
            continue;
        }

        if (Register(mapConfi) < 0)
            return -1;
    }

    m_pMonitorSo = new CMonitorSo;
    // 初始化插件监控
    if (m_pMonitorSo->InitMonitorDir(g_oPluginPath.c_str()) < 0)
    {
        delete m_pMonitorSo;
        LOGE << "initialize plugin monitor fail";
        return -1;
    }

    // 加载插件
    if (m_oPlugin.LoadSo(g_oPluginPath.c_str()) < 0)
    {
        LOGE << "load plugin fail";
        return -1;
    }
    return 0;
}

int CJSvc::Start()
{
    if (m_pMonitorSo->Start(&m_oPlugin) < 0)
    {
        LOGE << "start plugin monitor fail";
        return -1;
    }
    return CNet::GetObj()->Start();
}

void CJSvc::Stop()
{
    CNet::Release();
}

int CJSvc::InitGlobal(CConfig::config_info *pCfg)
{
    if (LogInit(pCfg) < 0)
        return -1;
    
    LOGI << "global init ...";
    uint32_t dwThreadCount = 0;
    CConfig::config_info::iterator it = pCfg->find("work_thread_count");
    if (it != pCfg->end())
        dwThreadCount = atoi(it->second.c_str());

    uint32_t dwStackSize = 1024 * 1024;
    it = pCfg->find("coroutine_sp");
    if (it != pCfg->end())
    {
        LOGI << "sp size : " << it->second;
        const char* c = it->second.c_str();
        uint32_t dwSp = 0;
        while(*c != 0 && *c >= '0' && *c <= '9')
        {
            dwSp = 10 * dwSp + (*c - '0');
            ++c;
        }

        if (*c == 0)
            dwStackSize = dwSp;
        else if ((*c & ~0x20) == 'K' && dwSp >= 8)
            dwStackSize = dwSp * 1024;
        else if ((*c & ~0x20) == 'M' && dwSp != 0)
            dwStackSize = dwSp * dwStackSize;
    }
    else
        LOGI << "sp size : 1M";

    int iRet = CNet::GetObj()->Init(dwThreadCount, dwStackSize);
    if (iRet < 0)
    {
        LOGE << CNet::GetObj()->GetErr();
    }

    it = pCfg->find("so_uninstall_interval");
    if (it != pCfg->end())
    {
        g_dwSoUninstallInterval = atoi(it->second.c_str());
    }

    it = pCfg->find("plugin_dir");
    if (it != pCfg->end())
    {
        g_oPluginPath = it->second;
        const char* s = g_oPluginPath.c_str() + g_oPluginPath.size();
        if (*s != '/')
            g_oPluginPath.append("/");
    }

    const char* s = g_oPluginPath.c_str();
	const char* e = s;
	while (*e)
	{
		if (*e == '/')
		{
			std::string path(s, e - s);
			if (access(path.c_str(), F_OK) != 0)
				mkdir(path.c_str(), 0755);
		}
		++ e;
	}

    LOGI << "global init end";
    return iRet;
}

int CJSvc::Register(CConfig::config_info *pCfg)
{
    uint16_t wProtocol = 0;
    CConfig::config_info::iterator it = pCfg->find("protocol");
    if (it == pCfg->end())
    {
        LOGE << "read protocol empty";
        return -1;
    }

    if (it->second.find("tcp:", 0, 4) != std::string::npos)
        wProtocol = ITaskBase::PROTOCOL_TCP;
    else if (it->second.find("unix:", 0, 5) != std::string::npos)
        wProtocol = ITaskBase::PROTOCOL_UNIX;
    else if (it->second.find("udp:", 0, 4) != std::string::npos)
        wProtocol = ITaskBase::PROTOCOL_UDP;
    else if (it->second.find("udpg:", 0, 5) != std::string::npos)
        wProtocol = ITaskBase::PROTOCOL_UDPG;
    else if (it->second.find("tcps:", 0, 5) != std::string::npos)
        wProtocol = ITaskBase::PROTOCOL_TCPS;
    else
    {
        LOGE << "protocol nonsupport";
        return -1;
    }

    std::string proto = it->second;
    NEWOBJ(ITaskBase, pNewObj);
    if (it->second.find(":http") != std::string::npos)
        pNewObj = CHttpSvc::GetObj;
    else if (it->second.find(":binary") != std::string::npos)
        pNewObj = CBinarySvc::GetObj;
    else
    {
        LOGE << "protocol nonsupport";
        return -1;
    }

    const char* pszServerName = "";
    it = pCfg->find("server_name");
    if (it != pCfg->end())
        pszServerName = it->second.c_str();

    uint32_t dwTimeout = 30e3;
    it = pCfg->find("connect_timeout");
    if (it != pCfg->end())
    {
        uint32_t dwOut = atoi(it->second.c_str());
        if (dwOut != 0)
            dwTimeout = dwOut * 1e3;
    }

    uint16_t wVer = 4;
    it = pCfg->find("tcp_ver");
    if (it != pCfg->end())
    {
        int iVer = atoi(it->second.c_str());
        wVer = (iVer == 4 || iVer == 0) ? 4 : 6;
    }

    it = pCfg->find("listen");
    if (it == pCfg->end())
    {
        LOGE << "parser listen address fail";
        return -1;
    }

    std::string sAddr;
    uint16_t wPort = 0;

    const char* c = it->second.c_str();
    const char* s = c;
    while(*c != 0 && *c != ':') ++c;
    sAddr.assign(s, c - s);
    if (*c != 0)
    {
        ++ c;
        wPort = atoi(c);
    }

    if (wProtocol != ITaskBase::PROTOCOL_UNIX && wPort == 0)
    {
        LOGE << "protocol not uinx, port == 0 fail";
        return -1;
    }

    if (wProtocol == ITaskBase::PROTOCOL_UNIX)
    {
        if (Remove(sAddr.c_str()) < 0)
        {
            return -1;
        }
    }

    s = sAddr.c_str();
    if (sAddr.empty())
        s = 0;

    const char *pszSslCert = 0;
    const char *pszSslKey = 0;
    if (wProtocol == ITaskBase::PROTOCOL_TCPS)
    {
        it = pCfg->find("ssl_cert");
        if (it == pCfg->end())
        {
            LOGE << "error: ssl cert not config";
            return -1;
        }

        if (it->second.empty())
        {
            LOGE << "error: ssl cert not config";
            return -1;
        }
        pszSslCert = it->second.c_str();

        it = pCfg->find("ssl_key");
        if (it == pCfg->end())
        {
            LOGE << "error: ssl key not config";
            return -1;
        }

        if (it->second.empty())
        {
            LOGE << "error: ssl key not config";
            return -1;
        }
        pszSslKey = it->second.c_str();
    }

    if (s)
        LOGI << proto << " server name [" << pszServerName << "] server address [" << s << "] server port [" << wPort << "] timeout [" << dwTimeout << "]";
    else
        LOGI << proto << " server name [" << pszServerName << "] server address [] server port [" << wPort << "] timeout [" << dwTimeout << "]";
    if (CNet::GetObj()->Register(pNewObj, &m_oPlugin, wProtocol, wPort, s, wVer, dwTimeout, pszServerName, pszSslCert, pszSslKey) < 0)
    {
        LOGE << CNet::GetObj()->GetErr();
        return -1;
    }
    return 0;
}

int CJSvc::Remove(const char *pszUnixPath)
{
    if (!pszUnixPath || *pszUnixPath == 0)
    {
        LOGE << "not unix address";
        return -1;
    }

    CUnixCli oCli;
    oCli.SetSync();
    if (oCli.Create(pszUnixPath, 0, 0) < 0)
    {
        remove(pszUnixPath);
        return 0;
    }
    LOGI << "server runing";
    return -1;
}

int CJSvc::LogInit(CConfig::config_info *pCfg)
{
    if (CLog::GetObj()->IsInit())
        return 0;

    CLogConfig oLog;
    CConfig::config_info::iterator it = pCfg->find("log_dir");
    if (it != pCfg->end())
        oLog.sLogDir = it->second;

    it = pCfg->find("log_level");
    if (it != pCfg->end())
    {
        const char* s = it->second.c_str();
        char c = *s;
        if (c == 'o')
            oLog.dwLevel = CLogConfig::LOG_OFF;
        else if (c == 'a')
            oLog.dwLevel = CLogConfig::LOG_ALL_LEVEL;
        else if (c == 'f')
            oLog.dwLevel = CLogConfig::LOG_FATAL_LEVEL;
        else if (c == 'e')
            oLog.dwLevel = CLogConfig::LOG_ERROR_LEVEL;
        else if (c == 'w')
            oLog.dwLevel = CLogConfig::LOG_WARN_LEVEL;
        else if (c == 'i')
            oLog.dwLevel = CLogConfig::LOG_INFO_LEVEL;
        else if (c == 'd')
            oLog.dwLevel = CLogConfig::LOG_DEBUG_LEVEL;
    }

    it = pCfg->find("log_write_mode");
    if (it != pCfg->end())
    {
        const char* s = it->second.c_str();
        uint32_t dwMode = 0;
        while (true)
        {
            const char *c = s;
            while ((*c != 0) && (*c != ':'))
                ++c;

            if (*s == 'f')
                dwMode |= CLogConfig::LOG_FILE;
            else if (*s == 'p')
                dwMode |= CLogConfig::LOG_DEF;
            else if (*s == 't')
            {
                if ((dwMode & (CLogConfig::LOG_UINX | CLogConfig::LOG_UDP)) == 0)
                    dwMode |= CLogConfig::LOG_TCP;
            }
            else if (*s == 'u')
            {
                ++ s;
                if (*s == 'd')
                {
                    if ((dwMode & (CLogConfig::LOG_UINX | CLogConfig::LOG_TCP)) == 0)
                        dwMode |= CLogConfig::LOG_UDP;
                }
                else
                {
                    if ((dwMode & (CLogConfig::LOG_TCP | CLogConfig::LOG_UDP)) == 0)
                        dwMode |= CLogConfig::LOG_UINX;
                }
            }

            if (*c == 0)
                break;

            ++ c;
            s = c;
        }

        if (dwMode != 0)
            oLog.dwWriteMode = dwMode;
    }

    it = pCfg->find("log_file_max_size");
    if (it != pCfg->end())
    {
        const char *c = it->second.c_str();
        uint32_t dwStackSize = 0;
        while (*c != 0 && *c >= '0' && *c <= '9')
        {
            dwStackSize = 10 * dwStackSize + (*c - '0');
            ++c;
        }

        if ((*c & ~0x20) == 'K' && dwStackSize >= 8)
            dwStackSize = dwStackSize * 1024;
        else if ((*c & ~0x20) == 'M' && dwStackSize != 0)
            dwStackSize = 1024 * 1024 * dwStackSize;

        if (dwStackSize > 1024 * 1024)
            oLog.iLogMaxSize = dwStackSize;
    }

    it = pCfg->find("log_net_addr");
    if (it != pCfg->end())
    {
        if ((oLog.dwWriteMode & (CLogConfig::LOG_TCP | CLogConfig::LOG_UDP | CLogConfig::LOG_UINX)) == 0)
        {
            printf("error: not select net mode\n");
            return -1;
        }

        oLog.sNetAddr = it->second;
        if (oLog.sNetAddr.empty())
        {
            printf("error : config net address empty\n");
            return -1;
        }
    }

    it = pCfg->find("log_net_ver");
    if (it != pCfg->end())
    {
        uint16_t wVer = atoi(it->second.c_str());
        if (wVer == 6)
            oLog.m_iVer = 6;
    }

    if (!CLog::GetObj()->Create(&oLog))
    {
        printf("error : create log failed\n");
        return -1;
    }
    return 0;
}
