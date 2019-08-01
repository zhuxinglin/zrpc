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


WatcherEvent::WatcherEvent()
{

}

WatcherEvent::~WatcherEvent()
{

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

void WatcherEvent::Run()
{
    while (1)
    {
        ZkEvent oEv;
        m_oChan >> oEv;
    }
}