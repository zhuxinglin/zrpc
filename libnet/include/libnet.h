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

#ifndef __LIB_NET_H__
#define __LIB_NET_H__

#include "task_base.h"
#include <string>
#include "net_task.h"
#include "file_fd.h"

namespace znet
{

class CNet
{
private:
    CNet();
    ~CNet();

public:
    static CNet* GetObj();
    static void Set(CNet* pN);
    static void Release();
    int Init(uint32_t dwWorkThread = 0, uint32_t dwSp = 0);
    int Register(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint16_t wPort, const char *pszIP,
                 uint16_t wVer, uint32_t dwTimeoutMs, const char* pszServerName, const char *pszSslCert = 0, const char *pszSslKey = 0, const char* pszPass = 0);
    int Register(NEWOBJ(ITaskBase, pCb), void* pData, uint16_t wProtocol, uint32_t dwTimeoutMs);
    int Register(ITaskBase *pBase, void* pData, uint16_t wProtocol, int iFd, uint32_t dwTimeoutMs);
    int Register(NEWOBJ(ITaskBase, pCb), void* pData, uint16_t wProtocol, int iFd, uint32_t dwTimeoutMs);
    int Start();
    void Stop();
    int Unregister(const char *pszServerName);
    const char* GetErr();
    void SetMaxTaskCount(uint32_t dwMaxTaskCount);
    uint32_t GetCurTaskCount() const;
    uint32_t GetTaskThreadCount() const;
    uint64_t GetCurCid() const;
    ITaskBase* GetCurTask();
    // 本函数不会清理保存栈中的堆内存，这样会发生内存泄漏，设置退
	// 出时会执行一次Co
    void ExitCo(uint64_t qwCid = 0);
    bool IsExitCo(uint64_t qwCid = 0) const;

private:
    CFileFd* GetFd(uint16_t wProtocol, uint16_t wPort, const char *pszIP, uint16_t wVer, const char* pszPass, const char *pszSslCert, const char *pszSslKey);
    int SetUdpTask(NEWOBJ(ITaskBase, pCb), CFileFd* pFd, void *pData, uint16_t wProtocol, const char *pszIP);
    int Go();
    ITaskBase *NewTask(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs = -1);
    ITaskBase *NewTask(ITaskBase *pBase, void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs);
    int AddTimerTask(ITaskBase* pTask, uint32_t dwTimeout);
    int AddFdTask(ITaskBase* pTask, int iFd);
    CFileFd* GetFileFd(uint16_t wProtocol, int iFd);
    int FreeListenFd(void* pEvent, void* pData);
    int RemoveServer(void* pEvent, void* pData);
    void DeleteTask(ITaskBase* pTask);
    void DeleteObj(ITaskBase* pTask);

private:
    static CNet* m_pSelf;
    volatile bool m_bIsMainExit;
    void* m_pContext;
};

}

#endif
