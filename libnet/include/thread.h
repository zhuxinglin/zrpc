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
*/

#ifndef __THREAD__H__
#define __THREAD__H__

#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <inttypes.h>
#include <string>
#include "uconfig.h"

namespace znet
{

class CThread
{
public:
    CThread();
    virtual ~CThread();

public:
    int Start(const std::string& sThreadName = "", bool bExitMode = false, void* pUserData = nullptr, uint32_t dwId = 0);
    void Exit(void (*Notif)(void*) = nullptr, void* p = nullptr);
    virtual int PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg);
    virtual void Release();
    virtual std::string GetErr();

protected:
    virtual int Initialize(void* pUserData);
    virtual void Run(uint32_t dwId) = 0;
    void SetAffinity(uint32_t dwId);
    void SetSuccess(void* p);

private:
    static void* WorkThread(void* pParam);

protected:
    bool volatile m_bExit;
    pthread_t m_tid;
    std::string m_sName;
};

class CSem
{
public:
    CSem();
    ~CSem();

public:
    void Wait();
    bool Wait(uint64_t ddwTimeout);
    void Post();

private:
    sem_t m_Sem;
};

class CLock
{
public:
    CLock();
    ~CLock();

public:
    void Lock();
    void Unlock();
    pthread_mutex_t* Get(){return &m_Mutex;}

private:
    pthread_mutex_t m_Mutex;
};

template<class T>
class CPadlock
{
public:
    explicit CPadlock(T *pLock)
    {
        m_pLock = pLock;
        if (pLock)
            pLock->Lock();
    }
    explicit CPadlock(T &oLock)
    {
        m_pLock = &oLock;
        oLock.Lock();
    }
    ~CPadlock()
    {
        m_pLock->Unlock();
    }

private:
    T *m_pLock;
};

template <class T = uint32_t>
class CSpinLock
{
public:
    explicit CSpinLock(volatile T& dwSync)
    {
        m_pSync = &dwSync;
        while (__sync_lock_test_and_set(&dwSync, 1))
            _usleep_();
    }

    explicit CSpinLock(volatile T *pSync)
    {
        m_pSync = pSync;
        while (__sync_lock_test_and_set(pSync, 1))
            _usleep_();
    }

    ~CSpinLock()
    {
        __sync_lock_release(m_pSync);
    }

private:
    volatile T* m_pSync;
};

class CCond
{
public:
    CCond();
    ~CCond();

public:
    void Signal();
    void Broadcast();
    void Wait(CLock* pLock);
    bool Wait(CLock* pLock, uint64_t ddwTimeout);

private:
    pthread_cond_t m_Cond;
};

}

#endif
