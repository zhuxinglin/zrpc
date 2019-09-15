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

#include "http_svc.h"
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "plugin_base.h"
#include "so_plugin.h"
#include "log.h"

using namespace zrpc;

const int g_iReadLen = 8192;
extern map_confog g_mapConfig;

CHttpSvc::CHttpSvc()
{
    m_oParserSetring.on_url = CHttpSvc::OnUri;
    m_oParserSetring.on_status = CHttpSvc::OnStatus;
    m_oParserSetring.on_header_field = CHttpSvc::OnHeaderField;
    m_oParserSetring.on_header_value = CHttpSvc::OnHeaderValue;
    m_oParserSetring.on_body = CHttpSvc::OnBody;
    m_oParserSetring.on_message_complete = CHttpSvc::OnMessageComplete;
    m_oParserSetring.on_chunk_header = CHttpSvc::OnChunkFeader;
    m_oParserSetring.on_chunk_complete = CHttpSvc::OnChunkComplete;
    m_oParserSetring.on_message_begin = CHttpSvc::OnMessageBegin;
    m_oParserSetring.on_headers_complete = CHttpSvc::OnHeadersComplete;

    m_oParser.data = NULL;

    m_pszReadBuf = new char[g_iReadLen];

    m_iComplete = 0;
    m_iKeepAlive = true;
}

CHttpSvc::~CHttpSvc()
{
    if (m_pszReadBuf)
        delete m_pszReadBuf;
    m_pszReadBuf = 0;
}

znet::ITaskBase *CHttpSvc::GetObj()
{
    return new CHttpSvc();
}

void CHttpSvc::Error(const char* pszExitStr)
{
    LOGI << pszExitStr;
}

void CHttpSvc::Go()
{
    CSoPlugin* pPlugin = (CSoPlugin*)m_pData;
    do
    {
        if (ReadHttp() < 0)
            break;

        SetUri();

        int iCode;
        LOGD_BIZ(HTTP_START) << "URL: " << m_oHttpReq.sUri << ", len: " << m_oHttpReq.szBody.length();
        int iRet = pPlugin->ExecSo(m_oPtr, this, zplugin::CUtilHash::UriHash(m_oHttpReq.sUri.c_str(), m_oHttpReq.sUri.size()), &m_oHttpReq.szBody, iCode);
        LOGD_BIZ(HTTP_END) << "URL: " << m_oHttpReq.sUri << ", Code : " << iCode << ", ret: " << iRet;
        if (iCode != 200)
            WriteHttp(0, 0, iCode, iRet, 3e3);
    } while (m_iKeepAlive);
}

int CHttpSvc::CallPlugin(uint64_t dwKey, std::string *pReq, std::string *pResp)
{
    CSoPlugin *pPlugin = (CSoPlugin *)m_pData;
    return pPlugin->InnerSo(m_oPtr, this, dwKey, pReq, pResp);
}

int CHttpSvc::ReadHttp()
{
    http_parser_init(&m_oParser, HTTP_REQUEST);
    m_oParser.data = this;
    m_iComplete = 0;
    while (true)
    {
        int iLen = Read(m_pszReadBuf, g_iReadLen, 0xFFFFFFFF);
        if (iLen < 0)
            return -1;
        if (iLen == 0)
        {
            if (znet::ITaskBase::STATUS_TIMEOUT != m_wStatus)
                continue;
            else
                return -1;
        }

        iLen = http_parser_execute(&m_oParser, &m_oParserSetring, m_pszReadBuf, iLen);
        if (m_oParser.http_errno != HPE_OK)
        {
            return -1;
        }

        if (m_iComplete == 1)
            break;
    }
    return 0;
}

void CHttpSvc::SetUri()
{
    const char* c = m_oHttpReq.sUri.c_str();
    const char* s = c;
    while(*c != 0 && *c != '?')
        ++ c;

    if (m_oHttpReq.sMethod.compare("GET") == 0)
    {
        m_oHttpReq.szBody = ++ c;
        m_oHttpReq.sUri.resize(c - s - 1);
    }
}

int CHttpSvc::OnUri(http_parser *pParser, const char *at, size_t length)
{
    CHttpSvc* pThis = (CHttpSvc*)pParser->data;
    pThis->m_oHttpReq.sUri.append(at, length);
    return 0;
}

int CHttpSvc::OnStatus(http_parser *pParser, const char *at, size_t length)
{
    return 0;
}

int CHttpSvc::OnHeaderField(http_parser *pParser, const char *at, size_t length)
{
    CHttpSvc *pThis = (CHttpSvc *)pParser->data;
    if (pThis->m_oHttpReq.sHeaderField.empty())
        pThis->m_oHttpReq.sHeaderField.assign(at, length);
    else if (!pThis->m_oHttpReq.sHeaderField.empty() && pThis->m_oHttpReq.sHeaderValue.empty())
        pThis->m_oHttpReq.sHeaderField.append(at, length);
    else
    {
        pThis->SetHttpHeader();
        pThis->m_oHttpReq.sHeaderField.assign(at, length);
        pThis->m_oHttpReq.sHeaderValue = "";
    }
    return 0;
}

int CHttpSvc::OnHeaderValue(http_parser *pParser, const char *at, size_t length)
{
    CHttpSvc *pThis = (CHttpSvc *)pParser->data;
    pThis->m_oHttpReq.sHeaderValue.append(at, length);
    return 0;
}

int CHttpSvc::OnBody(http_parser *pParser, const char *at, size_t length)
{
    CHttpSvc *pThis = (CHttpSvc *)pParser->data;
    pThis->m_oHttpReq.szBody.append(at, length);
    return 0;
}

int CHttpSvc::OnMessageComplete(http_parser *pParser)
{
    CHttpSvc *pThis = (CHttpSvc*)pParser->data;
    pThis->m_iComplete = 1;
    pThis->SetHttpHeader();
    pThis->m_iKeepAlive = http_should_keep_alive(pParser);
    return 0;
}

int CHttpSvc::OnChunkFeader(http_parser *pParser)
{
    return 0;
}

int CHttpSvc::OnChunkComplete(http_parser *pParser)
{
    return 0;
}

int CHttpSvc::OnMessageBegin(http_parser *pParser)
{
    CHttpSvc *pThis = (CHttpSvc *)pParser->data;
    pThis->m_oHttpReq.mapHeader.clear();
    pThis->m_oHttpReq.sUri = "";
    pThis->m_oHttpReq.szBody = "";
    pThis->m_oHttpReq.sPrarm = "";
    pThis->m_oHttpReq.sHeaderField = "";
    pThis->m_oHttpReq.sHeaderValue = "";
    return 0;
}

int CHttpSvc::OnHeadersComplete(http_parser *pParser)
{
    CHttpSvc *pThis = (CHttpSvc *)pParser->data;
    pThis->m_oHttpReq.sMethod = http_method_str((http_method)pParser->method);
    return 0;
}

void CHttpSvc::SetHttpHeader()
{
    m_oHttpReq.mapHeader.insert(header_info::value_type(m_oHttpReq.sHeaderField, m_oHttpReq.sHeaderValue));
}

int CHttpSvc::WriteMsg(const char *pszData, int iDataLen, uint32_t dwTimoutMs)
{
    return Write(pszData, iDataLen, dwTimoutMs);
}

int CHttpSvc::WriteResp(const char *pszData, int iDataLen, int iCode, int iRet, int32_t dwTimoutMs, bool bIsHeader)
{
    if (bIsHeader)
        return WriteHttp(pszData, iDataLen, iCode, iRet, dwTimoutMs);

    return Write(pszData, iDataLen, dwTimoutMs);
}

const char *CHttpSvc::GetHttpHeader(const char *pszKey)
{
    header_it it = m_oHttpReq.mapHeader.find(pszKey);
    if (it == m_oHttpReq.mapHeader.end())
        return 0;
    return it->second.c_str();
}

void CHttpSvc::SetHttpHeader(const char *pszKey, const char *pszValue)
{
    m_oHttpHeader.insert(header_info::value_type(pszKey, pszValue));
}

int CHttpSvc::WriteHttp(const char *pszData, int iDataLen, int iCode, int iRet, int32_t dwTimoutMs)
{
    std::stringstream ssResp;
    ssResp << "HTTP/1.1 " << iCode << " OK\r\nCache-Control: no-cache\r\nServer: jsvc 1.0\r\n"
            "Content-Type: application/json; charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Credentials:true\r\n";
    ssResp << "Ret: " << iRet << "\r\n";
    
    for (header_it it = m_oHttpHeader.begin(); it != m_oHttpHeader.end(); ++it)
        ssResp << it->first << it->second << "\r\n";

    if (iDataLen == 0)
        ssResp << "Content-Length: 0\r\n";
    else
    {
        ssResp << "Content-Length: " << iDataLen << "\r\n\r\n";
        ssResp.write(pszData, iDataLen);
    }
    ssResp << "\r\n\r\n";
    if (Write(ssResp.str().c_str(), ssResp.str().length(), dwTimoutMs) < 0)
        return -1;

    return 0;
}