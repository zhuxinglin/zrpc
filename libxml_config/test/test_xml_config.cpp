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

#include "../include/xml_conf.h"


int main (int argc, char* const argv[])
{
    xmlconf::XmlConfHelper oHelper;
    oHelper.SetKey("im");
    oHelper.SetKey("imsdk.redis");
    int iRet = oHelper.Query();
    if (iRet < 0)
    {
        printf("error : %d\n", iRet);
        return 0;
    }

    printf("im.biz.port.1 : %s\n", oHelper.GetValue("im.biz.port.1", "10").c_str());
    printf("imsdk.redis.port.1 : %s\n", oHelper.GetValue("imsdk.redis.port.1", "10").c_str());
    return 0;
}
