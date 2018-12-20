

#include "socket_fd.h"
#include "event_epoll.h"
#include <stdio.h>
#include <errno.h>


int main(int argc, char const *argv[])
{
    CTcpSvc oTcp;

    int iFd = oTcp.Create(0, 8989, 5);
    if (iFd < 0)
    {
        printf("error : %s\n", oTcp.GetErr().c_str());
        return 0;
    }

    CEventEpoll oEve;
    oEve.Create();

    oEve.SetCtl(iFd, 0, 0, (void*)iFd);
    while(1)
    {
        struct epoll_event ev[128];
        int iCount = oEve.Wait(ev, 128, 5);
        printf("````````````   %d   %d    %d\n", iCount, errno, ETIMEDOUT);
        for (int i = 0; i < iCount; i++)
        {
            printf("ggffffff   %d\n", (int)(long)ev[i].data.ptr);
            iFd = oTcp.Accept();

            struct timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;
            oTcp.SetFdOpt(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            oEve.SetCtl(iFd, 0, 0, (void*)iFd);

            oEve.Close();
        }
        
    }
    return 0;
}

