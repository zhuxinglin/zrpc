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
*/

#include "../include/shm_config.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        printf("-h help\n");
        printf("-g query key\n");
        printf("-a print all key and value\n");
        return 0;
    }

    if (argc == 2)
    {
        if (strcmp(argv[1], "-a") == 0)
        {
            SHM_CONF->PrintfAll();
            return 0;
        }
        printf("-h help\n");
        printf("-g query key\n");
        printf("-a print all key and value\n");
        return 0;
    }

    if (strcmp(argv[1], "-g") == 0)
    {
        std::string sValue = SHM_CONF->GetValue(argv[2], "");
        printf("key: %s\tvalue: %s\n", argv[2], sValue.c_str());
    }
    return 0;
}

