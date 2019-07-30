/*
* zk api
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

#include "../include/zk_api.h"

ZkApi::ZkApi() : m_pWatcher(nullptr),
                 m_iCurrHostIndex(0),
                 m_iHostCount(0)
{

}

ZkApi::~ZkApi()
{

}

int ZkApi::Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId)
{
    if (!pszHost)
    {
        m_sErr = "check host param empty";
        return -1;
    }

    m_oCli.SetConnTimeoutMs(dwTimeout * 1000);
    if (pWatcher)
        m_pWatcher = pWatcher;
    
    if (pClientId)
        memcpy(&m_oClientId, pClientId, sizeof(m_oClientId));
    else
        memcpy(&m_oClientId, 0, sizeof(m_oClientId));

    
    return 0;
}
