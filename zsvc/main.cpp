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

#include <daemon.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "zsvc.h"

using namespace znet;
using namespace zrpc;

static CJSvc* g_pSvc = 0;

static const char* GetProc(const char* pszProcName)
{
    int len = strlen(pszProcName);
    const char *e = pszProcName + (len - 1);
    while (*e && len >= 0)
    {
        if (*e == '/')
            break;
        e--;
        len--;
    }

    e++;
    return e;
}

static std::string GetConfig(const char* pszProcName)
{
    std::string sConf("./conf/");
    sConf.append(GetProc(pszProcName)).append(".cfg");
    return sConf;
}

static int GetOldPid(const char* pszProcName)
{
    std::string sName = ".";
    sName.append(GetProc(pszProcName));

    FILE* fp = fopen(sName.c_str(), "rb");
    if (!fp)
    {
        printf("process pid file '%s' not exist\n", sName.c_str());
        return 0;
    }
    int pid = 0;
    fscanf(fp, "%d", &pid);
    fclose(fp);
    if (pid == 0)
    {
        printf("process pid file '%s', process pid = 0\n", sName.c_str());
        return 0;
    }

    std::string sComm = "/proc/";
    sComm.append(std::to_string(pid)).append("/comm");
    fp = fopen(sComm.c_str(), "rb");
    if (!fp)
    {
        printf("process not run\n");
        return 0;
    }
    fclose(fp);

    return pid;
}

void DoMain(int argc, const char** argv)
{
    std::string sConfName = GetConfig(argv[0]);
    CConfig oConf;
    if (oConf.Parse(sConfName.c_str()) < 0)
        return ;

    if (g_pSvc)
        return ;

    g_pSvc = new (std::nothrow)CJSvc;
    if (g_pSvc->Init(&oConf) < 0)
        return ;

    g_pSvc->Start();

    znet::CLog::DelObj();
    delete g_pSvc;
    g_pSvc = 0;
}

void PorcErr(int iErr)
{
    if (iErr == -1)
    {
        printf("process exit\n");
    }
    else if (iErr == SIGHUP)
    {
        printf("child restert %d\n", getpid());
        g_pSvc->Stop();
    }
    else
    {
        printf("process runing\n");
    }
}

__attribute__((constructor)) void zrpc_init()
{

}

int main(int argc, char const *argv[])
{
    int iMonitor = 0;
    if (argc > 1)
    {
        if (!strcmp(argv[1], "-m"))
            iMonitor = 1;
        else if (!strcmp(argv[1], "-c"))
            iMonitor = 2;
        else if (!strcmp(argv[1], "-t"))
        {
            std::string sConfName = GetConfig(argv[0]);
            CConfig oConf;
            if (oConf.Parse(sConfName.c_str()) < 0)
                return 1;
            printf("check file '%s' ok\n", sConfName.c_str());
            return 0;
        }
        else if (!strcmp(argv[1], "-HUP"))
        {
            int pid = GetOldPid(argv[0]);
            if (pid == 0)
                return 0;
            kill(pid, SIGHUP);
            printf("send SIGHUP %d success\n", pid);
            return 0;
        }
        else if (!strcmp(argv[1], "-QUIT"))
        {
            int pid = GetOldPid(argv[0]);
            if (pid == 0)
                return 0;
            kill(pid, SIGQUIT);
            printf("send SIGQUIT %d success\n", pid);
            return 0;
        }
        else if (!strcmp(argv[1], "-h"))
        {
            printf("-m not monitor\n");
            printf("-c terminal operation\n");
            printf("-t check config file\n");
            printf("-HUP restart child process\n");
            printf("-QUIT quit process\n");
            printf("-v version\n");
            printf("-h help\n");
            return 0;
        }
        else if (!strcmp(argv[1], "-v"))
        {
            printf("version 1.0\n");
            return 0;
        }
    }

    if (iMonitor == 1)
        CDaemon::DoDaemon(DoMain, argc, argv, 0, false);
    else if (iMonitor == 2)
        DoMain(argc, argv);
    else
        CDaemon::DoDaemon(DoMain, argc, argv, PorcErr);
    return 0;
}

__attribute__((destructor)) void zrpc_exit()
{
    
}