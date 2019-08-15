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

#include "config_zk.h"
#include <stdio.h>
#include "log.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"

using namespace zplugin;

ConfigZk::ConfigZk(zkapi::IZkApi* zk)
{
    m_pZkApi = zk;
}

ConfigZk::~ConfigZk()
{
}

int ConfigZk::Init(const char* pZkAddrFileName)
{
    FILE* fp = fopen(pZkAddrFileName, "rb");
    if (!fp)
    {
        LOGE_BIZ(AGENT_INIT) << "open config file failed, " << pZkAddrFileName;
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    long lSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char* pBuf = new char[lSize + 1];
    int iLen = fread(pBuf, 1, lSize, fp);
    fclose(fp);

    char* p = pBuf;
    while (*p && iLen > 0)
    {
        char c = *p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            *p = 0;
            break;
        }
        ++p;
    }
    m_sZkAddr = pBuf;
    LOGI_BIZ(AGENT_INIT) << "zk address : " << m_sZkAddr;

    m_pConfWatch = new ConfigWatcher(m_pZkApi);
    if (m_pConfWatch->Init() < 0)
        return -1;

    znet::CNet::GetObj()->Register(this, nullptr, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);
    return 0;
}

void ConfigZk::Run()
{
    if (m_pZkApi->Init(m_sZkAddr.c_str(), m_pConfWatch, 30, 0, nullptr) < 0)
    {
        LOGE_BIZ(INIT) << m_pZkApi->GetErr();
        delete m_pZkApi;
        delete m_pConfWatch;
        return ;
    }
    LOGI_BIZ(INTI) << "connect zookeeper success";
}


#pragma GCC diagnostic pop