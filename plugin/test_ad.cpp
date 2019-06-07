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

#include "test_ad.h"
#include <stdio.h>
#include <sstream>
#include "log.h"

using namespace zplugin;

CTestAd::CTestAd()
{
}

CTestAd::~CTestAd()
{
}

int CTestAd::Initialize(znet::CLog *pLog)
{
    znet::CLog::SetObj(pLog);
    return 0;
}

int CTestAd::GetRouteTable(std::set<uint64_t> &setKey)
{
    LOGI << "/ad/app/download/get";
    setKey.insert(CUtilHash::UriHash("/ad/app/download/get", sizeof("/ad/app/download/get") - 1));
    return 0;
}

int CTestAd::Process(CControllerBase *pController, uint64_t dwKey, std::string *pMessage)
{
    FILE* fp;
    fp = fopen("/root/zxl/zrpc/plugin/makefile", "rb");
    int file_size = 0;
    if (!fp)
        LOGE << "open file '/root/zxl/zrpc/plugin/makefile' failed";
    else
    {
        std::string& sMsg = *pMessage;
        std::string::size_type pos = sMsg.find("offset=");
        long offset = 0;
        if (pos != std::string::npos)
        {
            const char *p = sMsg.c_str() + sizeof("offset=") - 1;
            while(*p != 0 && *p >= '0' && *p <= '9')
            {
                offset = offset * 10 + (*p - '0');
                ++ p;
            }
        }

        fseek(fp, 0L, SEEK_END);
        file_size = ftell(fp);
        if (offset <= file_size)
            fseek(fp, offset, SEEK_SET);
        file_size -= offset;

        if (file_size < 0)
            file_size = 0;
    }

    std::stringstream ssResp;
    ssResp << "HTTP/1.1 200 OK\r\nCache-Control: no-cache\r\nServer: jsvc 1.0\r\nContent-Type: application/download\r\nRet: 0\r\n"
    "Content-Transfer-Encoding: binary\r\nContent-Disposition: attachment;filename=ad.exe\r\nContent-Length: ";
    ssResp << file_size << "\r\n\r\n";
    pController->WriteResp(ssResp.str().c_str(), ssResp.str().size(), 200, 0, false);
    if (!fp)
        return 0;

    char buf[8192];
    while (file_size > 0 && !feof(fp))
    {
        int len = fread(buf, 1, 8192, fp);
        file_size -= len;
        if (pController->WriteResp(buf, len, 200, 0, false) < 0)
        {
            LOGE << "write data failed";
            break;
        }
    }
    fclose(fp);
    return 0;
}

int CTestAd::Process(CControllerBase *pController, uint64_t dwKey, std::string *pReq, std::string *pResp)
{
    return 0;
}

void CTestAd::Release()
{
    delete this;
}

CPluginBase *SoPlugin()
{
    return new CTestAd();
}
