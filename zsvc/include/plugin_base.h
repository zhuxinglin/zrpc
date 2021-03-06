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


#ifndef __PLUGIN_BASE_H__
#define __PLUGIN_BASE_H__

#include <string>
#include <set>
#include <stdint.h>
#include <arpa/inet.h>
#include "log.h"
#include <task_base.h>
#include <coroutine.h>
#include <libnet.h>
#include <atomic>

namespace zplugin
{

struct CControllerBase
{
    virtual int WriteMsg(const char *pszData, int iDataLen, uint32_t dwTimoutMs) = 0;
    virtual int CallPlugin(uint64_t dwKey, std::string *pReq, std::string *pResp) = 0;
};

struct CHttpController : public CControllerBase
{
    virtual const char* GetHttpHeader(const char* pszKey) = 0;
    virtual void SetHttpHeader(const char* pszKey, const char* pszValue) = 0;
    virtual int WriteResp(const char *pszData, int iDataLen, int iCode, int iRet, int32_t dwTimoutMs, bool bIsHeader) = 0;
};

struct CBinaryController : public CControllerBase
{
    virtual int WriteResp(const char *pszData, int iDataLen, uint16_t wCmd, int16_t iRet, int32_t dwTimoutMs) = 0;
};

#define HEADER_FLAGE    0x6161      // 数据包头开始 aa   主要作用抓包
#define END_FLAGE   0x03                // 数据包结束3

// 二进制协议头
struct CBinaryProtocolHeader
{
    CBinaryProtocolHeader(int32_t len, uint16_t cmd, int16_t ret) : wHeader(HEADER_FLAGE),
                                                                    iLen(len),
                                                                    wCmd(cmd),
                                                                    iRet(ret)
    {}
    uint16_t wHeader;  // aa=0x6161
    uint16_t wVer;
    int32_t iLen;       // iLen = sizeof(CBinaryProtocolHeader) + data_len + 1
    uint16_t wCmd;
    int16_t iRet;
    char szBody[0];

    void Set(int32_t len, uint16_t cmd, int16_t ret)
    {
        wHeader = HEADER_FLAGE;
        iLen = len;
        wCmd = cmd;
        iRet = ret;
    }

    void Hton()
    {
        wVer = 0;
        iLen = htonl(iLen);
        wCmd = htons(wCmd);
        iRet = htons(iRet);
        wVer = htons(wVer);
    }
};

struct CSharedData
{
    void* pPtr[2]{0, 0};
    std::atomic_int iIndex{0};
};

struct CPluginBase
{
    virtual int Initialize(znet::CLog *pLog, znet::CNet *pN, CSharedData *pProc, CSharedData* pSo) = 0;
    virtual int GetRouteTable(std::set<uint64_t>& setKey) = 0;
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pMessage) = 0;
    virtual int Process(znet::SharedTask& oCo, CControllerBase* pController, uint64_t dwKey, std::string *pReq, std::string *pResp) = 0;
    virtual void Release() = 0;
};

typedef CPluginBase* (*SoExportFunAddr)();


struct CUtilHash
{
    static uint64_t UriHash(const char *pszStr, int iLen)
    {
        uint64_t ddwRet = 0;
        uint16_t *pS;

        if (pszStr == 0)
            return (0);
        iLen = (iLen + 1) / 2;
        pS = (uint16_t *)pszStr;

        for (int i = 0; i < iLen; ++i)
            ddwRet ^= (pS[i] << (i & 0x0f));
        return (ddwRet);
    }
};

}

#define SO_PUBILC __attribute__((visibility("default")))
#define SO_PRIVATE __attribute__((visibility("hidden")))

#endif
