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
#include "src/zsvc.h"

using namespace znet;
using namespace zrpc;

static CJSvc* g_pSvc = 0;

static std::string GetConfig(const char* pszProcName)
{
    std::string sConf("./conf/");
    sConf.append(CDaemon::GetProc(pszProcName)).append(".cfg");
    return sConf;
}

static int Init(const char* argv)
{
    std::string sConfName = GetConfig(argv);
    CConfig oConf;
    if (oConf.Parse(sConfName.c_str()) < 0)
        return -1;

    if (g_pSvc)
        return -1;

    g_pSvc = new (std::nothrow)CJSvc;
    if (!g_pSvc)
        return -1;

    if (g_pSvc->Init(&oConf) < 0)
        return -1;
    return 0;
}

void DoMain(int argc, const char** argv)
{
    if (Init(argv[0]) < 0)
        return ;

    g_pSvc->Start();

    znet::CLog::DelObj();
    delete g_pSvc;
    g_pSvc = 0;
    exit(0);
}

void PorcErr(int iErr)
{
    if (iErr == -1)
    {
        fprintf(stderr, "process exit\n");
    }
    else if (iErr == SIGHUP)
    {
        fprintf(stderr, "child restert %d\n", getpid());
        g_pSvc->Stop();
    }
    else if (iErr == SIGINT)
    {
        g_pSvc->Stop();
    }
    else
    {
        fprintf(stderr, "process runing\n");
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
            CDaemon::RestartChildProcess(argv[0]);
            return 0;
        }
        else if (!strcmp(argv[1], "-QUIT"))
        {
            CDaemon::Quit(argv[0]);
            return 0;
        }
        else if (!strcmp(argv[1], "-h"))
        {
            printf("-m not monitor\n");
            printf("-c terminal operation\n");
            printf("-t check config file\n");
            printf("-HUP restart child process(no guardian process, stop process)\n");
            printf("-QUIT quit process(only guardian process)\n");
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

    CDaemon::DoDaemon(DoMain, argc, argv, PorcErr, iMonitor);
    return 0;
}

__attribute__((destructor)) void zrpc_exit()
{
    
}