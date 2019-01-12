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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "thread.h"
#include "uconfig.h"
#include <stdio.h>

struct CThreadParam
{
    CThread* m_pThis;
    uint32_t m_dwId;
    volatile bool m_bIsDone;
};

CThread::CThread(): m_bExit(true)
{
}

CThread::~CThread()
{
}

int CThread::Start(void *pUserData, uint32_t dwId)
{
    if (Initialize(pUserData) < 0)
    {
        return -1;
    }

    CThreadParam oParam;
    oParam.m_pThis = this;

    pthread_t tid;
    oParam.m_dwId = dwId;
    oParam.m_bIsDone = true;
    if (pthread_create(&tid, 0, CThread::WorkThread, &oParam) != 0)
    {
        return -1;
    }

    while(oParam.m_bIsDone)
        usleep(0);

    return 0;
}

void CThread::Exit()
{
    m_bExit = false;
}

int CThread::PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg)
{
    return 0;
}

int CThread::Initialize(void* pUserData)
{
    return 0;
}

void CThread::Release()
{
    delete this;
}

std::string CThread::GetErr()
{
    return "";
}

void CThread::SetAffinity(uint32_t dwId)
{
    int iSumCpu = sysconf(_SC_NPROCESSORS_CONF);
	int iCpu = dwId % iSumCpu;
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(iCpu, &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
}

void CThread::SetSuccess(void* p)
{
    CThreadParam* pParam = (CThreadParam*)p;
    pParam->m_bIsDone = false;
}

void* CThread::WorkThread(void* pParam)
{
    CThreadParam oTreadParam;
    memcpy(&oTreadParam, pParam, sizeof(oTreadParam));
    pthread_detach(pthread_self());

    oTreadParam.m_pThis->SetSuccess(pParam);
    oTreadParam.m_pThis->SetAffinity(oTreadParam.m_dwId);
    oTreadParam.m_pThis->Run(oTreadParam.m_dwId);
    pthread_exit(0);
    return 0;
}

CSem::CSem()
{
    sem_init(&m_Sem, 0, 0);
}

CSem::~CSem()
{
    sem_destroy(&m_Sem);
}

void CSem::Wait()
{
    sem_wait(&m_Sem);
}

bool CSem::Wait(unsigned long timeout /* = -1*/)
{
    int ret;
    struct timespec ti;
    if ((unsigned long)-1 == timeout)
        ti.tv_sec = 0x7FFFFFFF;
    else
        ti.tv_sec = time(0) + timeout;
    ti.tv_nsec = 0;
    while (1)
    {
        ret = sem_timedwait(&m_Sem, &ti);
        if (ret == -1 && errno == EINTR)
        {
            errno = 0;
            continue;
        }

        if (ret == -1 && errno == ETIMEDOUT)
        {
            errno = 0;
            return false;
        }
        break;
    }
    return true;
}

void CSem::Post()
{
    sem_post(&m_Sem);
}

CLock::CLock()
{
    pthread_mutex_init(&m_Mutex, 0);
}

CLock::~CLock()
{
    pthread_mutex_destroy(&m_Mutex);
}

void CLock::Lock()
{
    pthread_mutex_lock(&m_Mutex);
}

void CLock::Unlock()
{
    pthread_mutex_unlock(&m_Mutex);
}

CPadlock::CPadlock(CLock *pLock)
{
    m_pLock = pLock;
    if (pLock)
        pLock->Lock();
}

CPadlock::CPadlock(CLock &oLock)
{
    m_pLock = &oLock;
    oLock.Lock();
}

CPadlock::~CPadlock()
{
    m_pLock->Unlock();
}

CSpinLock::CSpinLock(volatile uint32_t &dwSync)
{
    m_pSync = &dwSync;
    while (__sync_lock_test_and_set(&dwSync, 1))
        _usleep_();
}

CSpinLock::CSpinLock(volatile uint32_t *pSync)
{
    m_pSync = pSync;
    while (__sync_lock_test_and_set(pSync, 1))
        _usleep_();
}

CSpinLock::~CSpinLock()
{
    __sync_lock_release(m_pSync);
}

CCond::CCond()
{
    pthread_mutex_init(&m_Mutex, NULL);
    pthread_cond_init(&m_Cond, NULL);
}

CCond::~CCond()
{
    pthread_mutex_destroy(&m_Mutex);
    pthread_cond_destroy(&m_Cond);
}

void CCond::Signal()
{
    pthread_mutex_lock(&m_Mutex);
    pthread_cond_signal(&m_Cond);
    pthread_mutex_unlock(&m_Mutex);
}

void CCond::Broadcast()
{
    pthread_mutex_lock(&m_Mutex);
    pthread_cond_broadcast(&m_Cond);
    pthread_mutex_unlock(&m_Mutex);
}

void CCond::Wait()
{
    pthread_mutex_lock(&m_Mutex);
    pthread_cond_wait(&m_Cond, &m_Mutex);
    pthread_mutex_unlock(&m_Mutex);
}

bool CCond::Wait(uint64_t ddwTimeout)
{
    struct timespec outtime;
    pthread_mutex_lock(&m_Mutex);
    outtime.tv_sec = time(0) + ddwTimeout;
    outtime.tv_nsec = 0;
    while(1)
    {
        int ret = pthread_cond_timedwait(&m_Cond, &m_Mutex, &outtime);
        if (ret == -1 && errno == EINTR)
        {
            errno = 0;
            continue;
        }

        if (ret == -1 && errno == ETIMEDOUT)
        {
            errno = 0;
            return false;
        }
        break;
    }
    
    pthread_mutex_unlock(&m_Mutex);
    return true;
}

