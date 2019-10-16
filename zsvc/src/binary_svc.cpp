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

#include "binary_svc.h"
#include "plugin_base.h"
#include "so_plugin.h"
#include "log.h"
#include "libnet.h"
#include "binary_handle.h"

using namespace zrpc;

extern map_confog g_mapConfig;

CBinarySvc::CBinarySvc()
{
    m_pszRecvBuff = new char[sizeof(zplugin::CBinaryProtocolHeader)];
}

CBinarySvc::~CBinarySvc()
{
    if (m_pszRecvBuff)
        delete m_pszRecvBuff;
    m_pszRecvBuff = 0;
}

znet::ITaskBase* CBinarySvc::GetObj()
{
    return new CBinarySvc();
}

void CBinarySvc::Go()
{
    while (1)
    {
        std::shared_ptr<std::string> oBuf(new std::string());
        uint16_t wCmd;
        if (ReadData(oBuf, wCmd) < 0)
            wCmd = 0xFFFF;

        CBinaryHandle* pHandle = new (std::nothrow) CBinaryHandle(m_oPtr, oBuf, wCmd);
        if (!pHandle)
        {
            LOGE << "new CBinaryHandle object failed";
            continue;
        }

        if (znet::CNet::GetObj()->Register(pHandle, m_pData, znet::ITaskBase::PROTOCOL_TIMER, -1, 0) < 0)
            LOGE << "create coroutine failed : " << znet::CNet::GetObj()->GetErr();

        if (wCmd == 0xFFFF)
            break;
    }
}

int CBinarySvc::WriteMsg(const char *pszData, int iDataLen, uint32_t dwTimoutMs)
{
    return Write(pszData, iDataLen, dwTimoutMs);
}

int CBinarySvc::WriteResp(const char *pszData, int iDataLen, uint16_t wCmd, int16_t iRet, int32_t dwTimoutMs)
{
    std::string sData;
    sData.resize(sizeof(zplugin::CBinaryProtocolHeader));
    zplugin::CBinaryProtocolHeader* pHeader = (zplugin::CBinaryProtocolHeader*)sData.c_str();
    sData.append(pszData, iDataLen);
    pHeader->wHeader = HEADER_FLAGE;
    pHeader->iLen = iDataLen + 1;
    pHeader->wCmd = wCmd;
    pHeader->iRet = iRet;
    pHeader->Hton();
    sData.append(END_FLAGE, 1);
    return Write(sData.c_str(), sData.size(), dwTimoutMs);
}

int CBinarySvc::CallPlugin(uint64_t dwKey, std::string *pReq, std::string *pResp)
{
    CSoPlugin *pPlugin = (CSoPlugin *)m_pData;
    return pPlugin->InnerSo(m_oPtr, this, dwKey, pReq, pResp);
}

void CBinarySvc::Error(const char* pszExitStr)
{
    LOGI << pszExitStr;
}

int CBinarySvc::ReadData(std::shared_ptr<std::string>& oBuf, uint16_t& wCmd)
{
    int iOffset = sizeof(zplugin::CBinaryProtocolHeader);
    char *pBuf = m_pszRecvBuff;
    while (true)
    {
        int iLen = Read(pBuf, iOffset, 0xFFFFFFFF);
        if (iLen < 0)
        {
            // if (iLen == -2) // ³¬Ê±
            //     return -1;
            return -1;
        }

        iOffset -= iLen;
        pBuf += iLen;

        if (iOffset == 0)
        {
            zplugin::CBinaryProtocolHeader* pH = (zplugin::CBinaryProtocolHeader*)oBuf->c_str();
            if (pH->wHeader != HEADER_FLAGE)
            {
                LOGE << "error data package";
                return -1;
            }

            iOffset = ntohl(pH->iLen);
            wCmd = ntohs(pH->wCmd);
            break;
        }
    }

    oBuf->resize(iOffset);
    pBuf = (char*)oBuf->c_str();
    memcpy(pBuf, m_pszRecvBuff, sizeof(zplugin::CBinaryProtocolHeader));
    pBuf += sizeof(zplugin::CBinaryProtocolHeader);
    iOffset -= sizeof(zplugin::CBinaryProtocolHeader);

    while (iOffset > 0)
    {
        int iLen = Read(pBuf, iOffset, 0xFFFFFFFF);
        if (iLen < 0)
        {
            // if (iLen == -2) // ³¬Ê±
            //     return -1;
            return -1;
        }

        iOffset -= iLen;
        pBuf += iLen;
    }

    -- pBuf;
    if (*pBuf != END_FLAGE)
        return -1;

    return 0;
}
