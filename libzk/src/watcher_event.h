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
    ZkEvent(int t, int s):type(t), state(s)
    {}
    ~ZkEvent() = default;
    ZkEvent(const ZkEvent&) = default;

    std::shared_ptr<char> oMsg;
    int type;
    int state;
    int msg_len{0};
};

class WatcherEvent : public znet::ITaskBase
{
public:
    WatcherEvent();
    ~WatcherEvent();

public:
    int Init(IWatcher* pWatcher);
    void Push(ZkEvent& oEv);
    void Exit();

private:
    virtual void Run();

private:
    bool m_bIsExit = true;
    IWatcher* m_pWatcher;
    znet::CCoChan<ZkEvent> m_oChan;
};

}

#endif
