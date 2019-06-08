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

#ifndef __BINARY_HANDLE__H__
#define __BINARY_HANDLE__H__

#include <task_base.h>
#include <libnet.h>

namespace zrpc
{

class CBinaryHandle : public znet::ITaskBase 
{
public:
    explicit CBinaryHandle(znet::SharedTask& ptr, std::shared_ptr<std::string>& data, uint16_t wCmd);
    ~CBinaryHandle();

private:
    virtual void Run();

private:
    znet::SharedTask m_oMsgPtr;
    uint16_t m_wCmd;
    std::shared_ptr<std::string> m_oData;
};

}

#endif


