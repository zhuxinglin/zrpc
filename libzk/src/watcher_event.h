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

#ifndef __WATCHER_EVENT_H_
#define __WATCHER_EVENT_H_

#include "../include/zk_api.h"
#include <string>
#include <libnet.h>
#include <memory>
#include <co_chan.h>

namespace zkapi
{

struct ZkEvent
{
    ZkEvent() = default;
    ZkEvent(std::shared_ptr<char>& o, int t):oMsg(o), type(t)
    {}
    ~ZkEvent() = default;
    ZkEvent(const ZkEvent&) = default;

    std::shared_ptr<char> oMsg;
    int type;
};

class WatcherEvent : public znet::ITaskBase
{
public:
    WatcherEvent();
    ~WatcherEvent();

public:
    int Init(IWatcher* pWatcher);
    void Push(ZkEvent& oEv);

private:
    virtual void Run();

private:
    IWatcher* m_pWatcher;
    znet::CCoChan<ZkEvent> m_oChan;
};

}

#endif
