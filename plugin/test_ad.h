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

class CTestAd : public CPluginBase
{
public:
    CTestAd();
    ~CTestAd();

private:
    virtual int Initialize();
    virtual int GetRouteTable(std::set<uint64_t>& setKey);
    virtual int Process(CControllerBase *pController, uint64_t dwKey, std::string *pMessage);
    virtual void Release();
};

#ifdef __cplusplus
extern "C" {
#endif

SO_PUBILC CPluginBase *SoPlugin();

#ifdef __cplusplus
}
#endif

#endif
