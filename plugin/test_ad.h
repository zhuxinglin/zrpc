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

#ifndef __Test_AD_H__
#define __Test_AD_H__

#include <plugin_base.h>

namespace zplugin
{

class CTestAd : public CPluginBase
{
public:
    CTestAd();
    ~CTestAd();

private:
    virtual int Initialize(znet::CLog* pLog);
    virtual int GetRouteTable(std::set<uint64_t>& setKey);
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pMessage);
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp);
    virtual void Release();
};

}

#ifdef __cplusplus
extern "C" {
#endif

SO_PUBILC zplugin::CPluginBase *SoPlugin();

#ifdef __cplusplus
}
#endif

#endif
