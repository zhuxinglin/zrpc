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

#define BIN_BUFF    4096
extern map_confog g_mapConfig;

CBinarySvc::CBinarySvc()
{
    m_pszRecvBuff = new char[BIN_BUFF];
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
    while (true)
    {
        int iLen = Read(m_pszRecvBuff, iOffset, 0xFFFFFFFF);
        if (iLen < 0)
            return -1;
        if (iLen == 0)
        {
            if (znet::ITaskBase::STATUS_TIMEOUT != m_wStatus)
                continue;
            else
                return -1;
        }
        iOffset -= iLen;

        oBuf->append(m_pszRecvBuff, iLen);
        if (oBuf->size() == sizeof(zplugin::CBinaryProtocolHeader))
        {
            zplugin::CBinaryProtocolHeader* pH = (zplugin::CBinaryProtocolHeader*)oBuf->c_str();
            if (pH->dwHeader != HEADER_FLAGE)
            {
                LOGE << "error data package";
                return -1;
            }

            iOffset = ntohl(pH->iLen);
            wCmd = ntohs(pH->wCmd);
            break;
        }
    }

    while (iOffset > 0)
    {
        int iLen = Read(m_pszRecvBuff, iOffset, 0xFFFFFFFF);
        if (iLen < 0)
            return -1;
        if (iLen == 0)
        {
            if (znet::ITaskBase::STATUS_TIMEOUT != m_wStatus)
                continue;
            else
                return -1;
        }
        iOffset -= iLen;

        oBuf->append(m_pszRecvBuff, iLen);
    }

    const char* e = oBuf->c_str() + oBuf->size() - 1;
    if (*e != END_FLAGE)
        return -1;

    return 0;
}
