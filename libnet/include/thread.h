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

class CThread
{
public:
    CThread();
    virtual ~CThread();

public:
    int Start(void* pUserData = 0, uint32_t dwId = 0);
    void Exit();
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
    volatile bool m_bExit;
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

private:
    pthread_mutex_t m_Mutex;
};

class CPadlock
{
public:
    explicit CPadlock(CLock *pLock);
    explicit CPadlock(CLock &oLock);
    ~CPadlock();

private:
    CLock *m_pLock;
};

class CSpinLock
{
public:
    explicit CSpinLock(volatile uint32_t& dwSync);
    explicit CSpinLock(volatile uint32_t *pSync);
    ~CSpinLock();

private:
    volatile uint32_t* m_pSync;
};

class CCond
{
public:
    CCond();
    ~CCond();

public:
    void Signal();
    void Broadcast();
    void Wait();
    bool Wait(uint64_t ddwTimeout);

private:
    pthread_mutex_t m_Mutex;
    pthread_cond_t m_Cond;
};

#endif
