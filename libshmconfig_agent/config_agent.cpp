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


#include "config_agent.h"
#include "config_zk.h"

using namespace zplugin;

zkapi::IZkApi* g_pZkApi;

ConfigAgent::ConfigAgent()
{
}

ConfigAgent::~ConfigAgent()
{
}

int ConfigAgent::Initialize(znet::CLog *pLog, znet::CNet *pN, CSharedData *pProc, CSharedData* pSo)
{
    znet::CLog::SetObj(pLog);
    znet::CNet::Set(pN);
    LOGI_BIZ(AGENT) << "config agent";
    g_pZkApi = zkapi::IZkApi::CreateObj();
    ConfigZk* pConfigZk = new ConfigZk(g_pZkApi);
    return pConfigZk->Init("./conf/zk.cfg");
}

int ConfigAgent::GetRouteTable(std::set<uint64_t>& setKey)
{
    return 0;
}

int ConfigAgent::Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pMessage)
{
    return 0;
}

int ConfigAgent::Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp)
{
    return 0;
}

void ConfigAgent::Release()
{
    LOGI_BIZ(CLOSE) << "libconfig_agent exit ..... " << g_pZkApi;
    if (g_pZkApi)
        g_pZkApi->Close();
    g_pZkApi = nullptr;
}

zplugin::CPluginBase *SoPlugin()
{
    return new ConfigAgent();
}

