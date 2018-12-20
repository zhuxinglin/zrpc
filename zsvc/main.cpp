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

#include "zsvc.h"

CConfig g_oConf;

void DoMain(int argc, const char** argv)
{
    CJSvc oSvc;
    if (oSvc.Init(&g_oConf) < 0)
        return ;
    oSvc.Start();
}

void PorcErr(int iErr)
{
    if (iErr == -1)
    {
        printf("process exit\n");
    }
    else
    {
        printf("process runing\n");
    }
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
    }

    if (g_oConf.Parse("./conf/zsvc.cfg") < 0)
    {
        return 1;
    }

    if (iMonitor == 1)
        CDaemon::DoDaemon(DoMain, argc, argv, 0, false);
    else if (iMonitor == 2)
        DoMain(argc, argv);
    else
        CDaemon::DoDaemon(DoMain, argc, argv, PorcErr);
    return 0;
}

