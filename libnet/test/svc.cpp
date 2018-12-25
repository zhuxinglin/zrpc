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


#include "daemon.h"
#include "libnet.h"
#include <stdio.h>
#include <signal.h>

class CSvc : public CNetTask
{
public:
    CSvc();
    ~CSvc();

private:
    virtual void Go();
    virtual void Error(const char *pszExitStr);
    virtual void Release();
};

CSvc::CSvc()
{
}

CSvc::~CSvc()
{
}

void CSvc::Go()
{
    char szBuff[1024];
    memset(szBuff, 0, sizeof(szBuff));
    Read(szBuff, 1024);
    printf("%s\n", szBuff);
    Write("222222222222222", sizeof("222222222222222"));
}

void CSvc::Error(const char *pszExitStr)
{
    if (pszExitStr)
        printf("++++++++++++++++  %s\n", pszExitStr);
}

void CSvc::Release()
{
    delete this;
    printf("svc   release ----------------------- %p\n", this);
}

ITaskBase* NewSvc()
{
    return new CSvc();
}

int main(int argc, char const *argv[])
{
    CNet oNet;

//    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if (oNet.Init() < 0)
    {
        printf("---------------  %s\n", oNet.GetErr());
        return 0;
    }

    if (oNet.Register(NewSvc, 0, ITaskBase::PROTOCOL_TCP, 9898, 0, 4, 3000, "ssd", 0, 0) < 0)
    {
        printf("************** %s\n", oNet.GetErr());
        return 0;
    }

    if (oNet.Start() < 0)
    {
        printf("^^^^^^^^^^^^^^  %s\n", oNet.GetErr());
        return 0;
    }
    printf("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
    return 0;
}

