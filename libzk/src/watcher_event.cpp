/*
* zk event
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

#include "watcher_event.h"

using namespace zkapi;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"

WatcherEvent::WatcherEvent()
{
    m_wExitMode = ITaskBase::MANUAL_EXIT_MODE;
}

WatcherEvent::~WatcherEvent()
{
    if (m_pWatcher)
        delete m_pWatcher;
}

int WatcherEvent::Init(IWatcher* pWatcher)
{
    m_pWatcher = pWatcher;
    znet::CNet::GetObj()->Register(this, 0, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);
    m_bIsInit = true;
    return 0;
}

void WatcherEvent::Push(ZkEvent& oEv)
{
    m_oChan << oEv;
}

void WatcherEvent::Exit()
{
    if (!m_bIsInit)
        return;

    m_bIsExit = false;
    ZkEvent oEv;
    oEv.type = -1;
    m_oChan << oEv;
    m_oSem.Wait();
}

void WatcherEvent::Run()
{
    while (m_bIsExit)
    {
        ZkEvent oEv;
        m_oChan >> oEv;

        if (m_bIsExit && m_pWatcher)
            m_pWatcher->OnWatcher(oEv.type, this, oEv.oMsg);
    }
    m_oSem.Post();
}

#pragma GCC diagnostic pop