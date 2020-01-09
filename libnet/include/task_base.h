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
#include <memory>

#define NEWOBJ(x, v)    x* (*v)()

// 不为c++ 11版本
#if __cplusplus < 201103L
#define FINAL
#else
#define FINAL       final
#endif

namespace znet
{

struct ITaskBase
{
    ITaskBase();
    virtual ~ITaskBase();
    virtual void Run() = 0;
    virtual void Error(const char* pszExitStr){}
    int YieldEventDel(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent);
    int YieldEventRestore(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent);
    int Yield(uint32_t dwTimeoutMs = 0xFFFFFFFF, uint8_t wRunStatus = RUN_WAIT);
    static int Sleep(uint32_t dwTimeoutMs = 0xFFFFFFFF);
    bool IsExitCo() const;
    void CloseCo();

    enum _YieldOpt
    {
        YIELD_ADD = 0,
        YIELD_MOD = 1,
        YIELD_DEL = 2,
    };

    enum _YieldEvent
    {
        YIELD_ET_IN = 0,
        YIELD_ET_OUT = 1,
        YIELD_IN = 2,
        YIELD_OUT = 3,
    };

    int Yield(uint32_t dwTimeoutMs, int iFd, int iSetOpt, int iRestoreOpt, int iSetEvent, int iRestoreEvent, uint8_t wRunStatus);
    static uint64_t GenCid(int iFd);
    static int GetFd(uint64_t qwCid);
    static uint32_t GetSubCId(uint64_t qwCid);

    std::shared_ptr<ITaskBase> m_oPtr;

// ==========private===============
    void *m_pSp;
    void *m_pContext;
    void *m_pMainCo;
    NEWOBJ(ITaskBase, m_pCb);
    void *m_pTaskQueue;
// ===============public read-only==================
    void *m_pData;
    uint64_t m_qwConnectTime;
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
        RUN_NOW = 0x1,  // 立急执行状态，仅在启动时有效，如需要产急执行可手动修改为此状态,对定时无效
        RUN_INIT = 0x2, // 初始化状态，不立急执行，如果检查到超时，大于100ms会自动转RUN_READY，但也不执行
        RUN_WAIT = 0x4, // 被阻塞状态
        RUN_LOCK = 0x8, // 加锁状态
        RUN_SLEEP = 0x10,   // 休眠状态
        RUN_READY = 0x20,   // 就绪状态
        RUN_EXEC = 0x40,    // 正在执行状态
        RUN_EXIT = 0x80,    // 退出状态
    };
    uint8_t m_wIsRuning;
    uint8_t m_wRunStatus;
    uint8_t m_wRunStatusLock;
    // ===============public ===========
    enum
    {
        AUTO_EXIT_MODE  = 0,    // 自动退出，退出后清楚协程对象
        MANUAL_EXIT_MODE = 1,   // 手动模式，退出由应用控制
    };
    uint8_t m_wExitMode;        // 默认自动
};

using SharedTask = std::shared_ptr<ITaskBase>;

}

#endif
