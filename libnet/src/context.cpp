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

#include "context.h"
#include "go_post.h"

using namespace znet;

CContext::CContext() : m_pCo(0), m_pGo(0), m_pTaskQueue(0), m_pSchedule(0), m_oNetPool(sizeof(CNetEvent), 8)
{
}

CContext::~CContext()
{
    CFileFd &oFd(m_oEvent);
    oFd.Close();

    if (m_pSchedule)
    {
        CThread *pSch = dynamic_cast<CThread *>(m_pSchedule);
        pSch->Exit();
    }

    if (m_pGo)
    {
        for (uint32_t i = 0; i < m_dwWorkThreadCount; ++i)
            m_pGo[i].Exit();

        for (uint32_t i = 0; i < m_dwWorkThreadCount; ++i)
            m_pGo[i].Exit([](void *p) {
                CContext* th = reinterpret_cast<CContext*>(p);
                for (uint32_t i = 0; i < th->m_dwWorkThreadCount; ++i)
                    CGoPost::Post();
            }, this);
    }

    if (m_pCo)
        delete m_pCo;
    m_pCo = nullptr;

    if (m_pTaskQueue)
        delete m_pTaskQueue;
    m_pTaskQueue = nullptr;

    if (m_pGo)
        delete [] m_pGo;
    m_pGo = nullptr;

    if (m_pSchedule)
        delete m_pSchedule;
    m_pSchedule = nullptr;
}

int CContext::Init(uint32_t dwWorkThreadCount)
{
    m_dwWorkThreadCount = dwWorkThreadCount;

    m_pCo = new (std::nothrow) CCoroutine;
    if (!m_pCo)
    {
        m_sErr = "create coroutine object fail";
        return -1;
    }

    m_pTaskQueue = new (std::nothrow) CTaskQueue(dwWorkThreadCount);
    if (!m_pTaskQueue)
    {
        m_sErr = "create task queue object fail";
        return -1;
    }

    m_pGo = new (std::nothrow) CGo[dwWorkThreadCount];
    if (!m_pGo)
    {
        m_sErr = "create go thread object failed";
        return -1;
    }

    m_pSchedule= new (std::nothrow) CSchedule;
    if (!m_pSchedule)
    {
        m_sErr = "create schedule object failed";
        return -1;
    }

    return 0;
}
