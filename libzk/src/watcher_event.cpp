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
    return 0;
}

void WatcherEvent::Push(ZkEvent& oEv)
{
    m_oChan << oEv;
}

void WatcherEvent::Exit()
{
    m_bIsExit = false;
    ZkEvent oEv;
    oEv.type = -1;
    m_oChan << oEv;
    m_oChan >> oEv;
}

void WatcherEvent::Run()
{
    while (m_bIsExit)
    {
        ZkEvent oEv;
        m_oChan >> oEv;

        if (m_pWatcher)
            m_pWatcher->OnWatcher(oEv.type, this, oEv.oMsg);
    }

    ZkEvent oEv;
    m_oChan << oEv;
}

#pragma GCC diagnostic pop