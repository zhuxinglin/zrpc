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

static CJSvc* pSvc = 0;

static std::string GetConfig(const char* pszProcName)
{
    int len = strlen(pszProcName);
    const char* e = pszProcName + (len - 1);
    while (*e && len >= 0)
    {
        if (*e == '/')
            break;
        e --;
        len --;
    }

    e ++;
    std::string sConf("./conf/");
    sConf.append(e).append(".cfg");
    return sConf;
}

void DoMain(int argc, const char** argv)
{
    std::string sConfName = GetConfig(argv[0]);
    CConfig oConf;
    if (oConf.Parse(sConfName.c_str()) < 0)
        return ;

    if (pSvc)
        return ;

    pSvc = new (std::nothrow)CJSvc;
    if (pSvc->Init(&oConf) < 0)
        return ;

    pSvc->Start();

    znet::CLog::DelObj();
    delete pSvc;
    pSvc = 0;
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
        pSvc->Stop();
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
        else if (!strcmp(argv[1], "-h"))
        {
            printf("-m not monitor\n-c terminal operation\n-t check config file\n-v version\n-h help\n");
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