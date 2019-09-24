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


#ifndef __BINARY__SVC_H__
#define __BINARY__SVC_H__

#include "plugin_base.h"
#include <net_task.h>
#include <string>

namespace zrpc
{

class CBinarySvc : public znet::CNetTask, public zplugin::CBinaryController
{
public:
    CBinarySvc();
    ~CBinarySvc();

public:
    static ITaskBase* GetObj();

private:
    virtual void Go();
    virtual int WriteMsg(const char *pszData, int iDataLen, uint32_t dwTimoutMs);
    virtual int CallPlugin(uint64_t dwKey, std::string *pReq, std::string *pResp);
    virtual void Error(const char* pszExitStr);
    virtual int WriteResp(const char *pszData, int iDataLen, uint16_t wCmd, int16_t iRet, int32_t dwTimoutMs);

private:
    int ReadData(std::shared_ptr<std::string>& oBuf, uint16_t& wCmd);

private:
    char* m_pszRecvBuff;
};

}


#endif
