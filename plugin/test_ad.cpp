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

CTestAd::CTestAd()
{
}

CTestAd::~CTestAd()
{
}

int CTestAd::Initialize()
{
    return 0;
}

int CTestAd::GetRouteTable(std::set<uint64_t> &setKey)
{
    setKey.insert(CUtilHash::UriHash("/test/addinfo/test", sizeof("/test/addinfo/test")));
    return 0;
}

int CTestAd::Process(CControllerBase *pController, uint64_t dwKey, std::string *pMessage)
{
    std::string sResp;
    sResp = "<!DOCTYPE html>"
"<html>"
"<head>"
"    <title>test</title>"
"</head>"
"<body>"
"test http"
"</body>"
"</html>";
    pController->WriteResp(sResp.c_str(), sResp.size(), 200, 0, true);
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
