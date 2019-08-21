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

#ifndef _CONFIG_SERVER_H_
#define _CONFIG_SERVER_H_


#include "plugin_base.h"
#include "zk_config.h"
#include "dao_config.h"
#include <map>

namespace zplugin
{

class ConfigServer : public zplugin::CPluginBase
{
public:
    ConfigServer(/* args */);
    ~ConfigServer();

private:
    virtual int Initialize(znet::CLog *pLog, znet::CNet *pN, CSharedData *pProc, CSharedData* pSo);
    virtual int GetRouteTable(std::set<uint64_t>& setKey);
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pMessage);
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp);
    virtual void Release();

private:
    int getConnectConfig(std::string& sZkConfig, std::string& sMysqlConfig);
    int getValue(const char* pszKey, int iKeyLen, std::string& sStr);

private:
    int Add(CControllerBase* pController, std::string* pMessage);
    int Del(CControllerBase* pController, std::string* pMessage);
    int Mod(CControllerBase* pController, std::string* pMessage);
    int Query(CControllerBase* pController, std::string* pMessage);

private:
    void WriteError(CControllerBase* pController, int iCode, std::string sMsg);
    int CheckAdd(CControllerBase* pController, std::string* pMessage, dao::ShmConfigTable* pConf);
    int AddZk(dao::ShmConfigTable* pConf);
    int IsExist(dao::ShmConfigTable* pConf);
    int UpdateZk(dao::ShmConfigTable* pConf);
    int DeleteZk(std::string& sKey);
    const char* getParam(const char* pszParam, const char* pszKey, std::string& sStr);

private:
    zkapi::IZkApi* m_pZkApi;
    dao::DaoConfig m_oDao;
    typedef int (ConfigServer::*Fun)(CControllerBase*, std::string*);
    using MAP_FUN = std::map<uint64_t, Fun>;
    MAP_FUN m_mapFun;
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
