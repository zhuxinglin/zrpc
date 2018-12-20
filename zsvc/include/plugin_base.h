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

struct CControllerBase
{
    virtual int WriteResp(const char *pszData, int iDataLen, int iCode, int iRet, bool bIsHeader) = 0;
};

struct CHttpController : public CControllerBase
{
    virtual const char* GetHttpHeader(const char* pszKey) = 0;
    virtual void SetHttpHeader(const char* pszKey, const char* pszValue) = 0;
};

struct CPluginBase
{
    virtual int Initialize() = 0;
    virtual int GetRouteTable(std::set<uint64_t>& setKey) = 0;
    virtual int Process(CControllerBase *pController, uint64_t dwKey, std::string *pMessage) = 0;
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

#define SO_PUBILC __attribute__((visibility("default")))
#define SO_PRIVATE __attribute__((visibility("hidden")))

#endif
