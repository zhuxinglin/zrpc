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
#include <co_sem.h>

namespace zkapi
{

struct ZkEvent
{
    ZkEvent() = default;
    ZkEvent(int t, std::string& o):type(t), oMsg(o)
    {}
    ~ZkEvent() = default;
    ZkEvent(const ZkEvent&) = default;

    int type;
    std::string oMsg;
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
    bool IsInit()const {return m_bIsInit;}

private:
    virtual void Run();

private:
    volatile bool m_bIsExit = true;
    IWatcher* m_pWatcher;
    znet::CCoChan<ZkEvent> m_oChan;
    bool m_bIsInit = false;
    znet::CCoSem m_oSem;
};

}

#endif
