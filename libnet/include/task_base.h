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

#ifndef __TASK_BASE_H__
#define __TASK_BASE_H__

#include <stdint.h>
#include <sys/time.h>

#define NEWOBJ(x, v)    x* (*v)()

struct ITaskBase
{
    ITaskBase();
    virtual void Run() = 0;
    virtual void Error(const char* pszExitStr){}
    virtual void Release(){}
    int Yield(uint32_t dwTimeoutMs = 0, int iFd = -1, int iEvent = 0);
    int Yield(uint32_t dwTimeoutMs, int iFd, int iCurrMod, int iFinishMod, int iEvent);
    static uint64_t GenCid(int iFd);
    static int GetFd(uint64_t qwCid);
    static uint32_t GetSubCId(uint64_t qwCid);

// ==========private===============
    void *m_pSp;
    void *m_pContext;
    void *m_pMainCo;
    NEWOBJ(ITaskBase, m_pNewObj);
    void *m_pTaskQueue;
// ===============public read-only==================
    void *m_pData;
// ==========private===============
    uint64_t m_qwBeginTime;
// ===============public read-only==================
    uint64_t m_qwCid;
// ==========private===============
    uint32_t m_dwTimeout;
// ===============public read-only==================
    enum
    {
        STATUS_TIME = 1,
        STATUS_TIMEOUT = 2,
    };
    uint8_t m_wStatus;
// ==========private===============
    enum
    {
        PROTOCOL_TCP = 1,
        PROTOCOL_UNIX = 2,
        PROTOCOL_TCPS = 3,
        PROTOCOL_UDP = 4,
        PROTOCOL_UDPG = 5,
        PROTOCOL_TIMER = 6,
        PROTOCOL_TIMER_FD = 7,
        PROTOCOL_EVENT_FD = 8,
    };
    uint8_t m_wProtocol;
    enum
    {
        RUN_INIT = 1,
        RUN_WAIT = 2,
        RUN_NOW = 3,
        RUN_EXEC = 4,
        RUN_EXIT = 5,
    };
    uint8_t m_wIsRuning;
    uint8_t m_wRunStatus;
};

#endif
