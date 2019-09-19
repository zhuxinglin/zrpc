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

#include "binary_handle.h"
#include "so_plugin.h"
#include <net_task.h>
#include "plugin_base.h"
#include "binary_svc.h"
#include "log.h"

using namespace zrpc;

CBinaryHandle::CBinaryHandle(znet::SharedTask& ptr, std::shared_ptr<std::string>& data, uint16_t wCmd) : m_oMsgPtr(ptr),
m_wCmd(wCmd),
m_oData(data)
{

}

CBinaryHandle::~CBinaryHandle()
{

}

void CBinaryHandle::Run()
{
    CSoPlugin* pPlugin = (CSoPlugin*)m_pData;
    int iCode;
    CBinarySvc* pBin = (CBinarySvc*)m_oMsgPtr.get();
    LOGI_BIZ(BIN_START) << "cmd: " << m_wCmd << ", len: " << m_oData->length();
    int iRet = pPlugin->ExecSo(m_oMsgPtr, pBin, (uint64_t)m_wCmd, m_oData.get(), iCode);
    LOGI_BIZ(BIN_END) << "cmd: " << m_wCmd;
    if (iCode != 200)
    {
        CBinarySvc* pSvc = (CBinarySvc*)m_oMsgPtr.get();
        char szBuf[sizeof(zplugin::CBinaryProtocolHeader) + 1];
        zplugin::CBinaryProtocolHeader* pBin = (zplugin::CBinaryProtocolHeader*)szBuf;
        pBin->Set(0, m_wCmd, iRet);
        pBin->Hton();
        *((uint8_t*)pBin->szBody) = (uint8_t)END_FLAGE;
        pSvc->Write((const char*)szBuf, sizeof(zplugin::CBinaryProtocolHeader) + 1, 3e3);
        pSvc->Close();
    }
}