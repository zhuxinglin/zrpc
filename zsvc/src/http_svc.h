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

#ifndef __HTTP_SVC_H__
#define __HTTP_SVC_H__

#include <net_task.h>
#include "http_parser.h"
#include "plugin_base.h"
#include <map>
#include <string>

namespace zrpc
{

class CHttpSvc : public znet::CNetTask, public zplugin::CHttpController
{
public:
    CHttpSvc();
    ~CHttpSvc();

public:
    static ITaskBase* GetObj();

private:
    virtual void Go();
    virtual void Release();
    int ReadHttp();
    int WriteHttp(const char *pszData, int iDataLen, int iCode, int iRet);
    void SetHttpHeader();
    void SetUri();

private:
    virtual int WriteResp(const char *pszData, int iDataLen, int iCode, int iRet, bool bIsHeader);
    virtual int CallPlugin(uint64_t dwKey, std::string *pReq, std::string *pResp);
    virtual const char *GetHttpHeader(const char *pszKey);
    virtual void SetHttpHeader(const char *pszKey, const char *pszValue);
    virtual void Error(const char* pszExitStr);

private:
    static int OnUri(http_parser *, const char *at, size_t length);
    static int OnStatus(http_parser *, const char *at, size_t length);
    static int OnHeaderField(http_parser *, const char *at, size_t length);
    static int OnHeaderValue(http_parser *, const char *at, size_t length);
    static int OnBody(http_parser *, const char *at, size_t length);
    static int OnMessageComplete(http_parser *);
    static int OnChunkFeader(http_parser *);
    static int OnChunkComplete(http_parser *);
    static int OnMessageBegin(http_parser *);
    static int OnHeadersComplete(http_parser *);

private:
    typedef std::map<std::string, std::string>  header_info;
    typedef header_info::iterator header_it;
    typedef struct _HttpReq
    {
        std::string sMethod;
        header_info mapHeader;
        std::string sUri;
        std::string sPrarm;
        std::string sHeaderField;
        std::string sHeaderValue;
        std::string szBody;
    }CHttpReq;
/*
    typedef struct _HttpResp
    {
        int iCode;
        std::string sPhrase;
        header_info mapHeader;
        std::string szBody;
    }CHttpResp;*/

private:
    http_parser m_oParser;
    http_parser_settings m_oParserSetring;
    char *m_pszReadBuf;
    CHttpReq m_oHttpReq;
//    CHttpResp m_oHttpResp;
    header_info m_oHttpHeader;
    int m_iComplete;
    bool m_iKeepAlive;
};

}

#endif
