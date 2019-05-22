

#include "socket_fd.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    CTcpCli oCli;
    oCli.Create("127.0.0.1", 8989);

    while(1)sleep(-1);
    return 0;
}

