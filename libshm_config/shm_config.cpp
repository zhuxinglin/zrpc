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
*/

#include "shm_config.h"
#include "shm_struct.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdio.h>

using namespace shmmemory;

ShmConfig g_oShmConfig;

class AutoLock
{
public:
    AutoLock(std::atomic_flag& oLock)
    {m_pLock = &oLock; while(oLock.test_and_set());}

    ~AutoLock()
    {m_pLock->clear();}

private:
    std::atomic_flag* m_pLock;
};

ShmConfig::ShmConfig()
{
    init();
}

ShmConfig::~ShmConfig()
{
    if (m_pShmAddr)
        shmdt(m_pShmAddr);
}

ShmConfig* ShmConfig::Instance()
{
    return &g_oShmConfig;
}

std::string ShmConfig::GetValue(const char* pszKey, const char* pszDefaultValue)
{
    std::string sValue;
    int iRet = searchKey(pszKey, sValue);
    if (iRet == 0)
        return std::move(sValue);

    if (!pszDefaultValue)
        return std::move(std::string());

    return pszDefaultValue;
}

float ShmConfig::GetValueF(const char* pszKey, const float fDefaultValue)
{
    std::string sValue;
    int iRet = searchKey(pszKey, sValue);
    if (iRet != 0)
        return fDefaultValue;

    return strtof(sValue.c_str(), NULL);
}

double ShmConfig::GetValueD(const char* pszKey, const double dDefaultValue)
{
    std::string sValue;
    int iRet = searchKey(pszKey, sValue);
    if (iRet != 0)
        return dDefaultValue;

    return strtod(sValue.c_str(), NULL);
}

int ShmConfig::init()
{
    m_pShmHeader = createShmHeader();
    if (!m_pShmHeader)
        return -1;

    ShmAddrHeader* pShmAddrHeader = reinterpret_cast<ShmAddrHeader*>(m_pShmHeader);
    while (__sync_lock_test_and_set(&pShmAddrHeader->wSycLock, 1));
    m_pShmAddr = createShmData(pShmAddrHeader->dwBlockSize);
    __sync_lock_release(&pShmAddrHeader->wSycLock);
    if (!m_pShmAddr)
        return -1;
    return 0;
}

void* ShmConfig::createShmHeader()
{
    int iShmAddrHeaderId = shmget(SHM_ADDR_HEADER_KEY, sizeof(ShmAddrHeader), 0666);
    if (iShmAddrHeaderId < 0)
        return nullptr;
    return shmat(iShmAddrHeaderId, NULL, 0);
}

void* ShmConfig::createShmData(uint32_t dwBlockSize)
{
    int iShmAddrHeaderId = shmget(SHM_ADDR_DATA_KEY, dwBlockSize, 0666);
    if (iShmAddrHeaderId < 0)
        return nullptr;
    return shmat(iShmAddrHeaderId, NULL, 0);
}

int ShmConfig::searchKey(const char* pszKey, std::string& sValue)
{
    int iRet = -1;
    if (checkParam(pszKey) < 0)
        return iRet;

    int iKeyLen = strlen(pszKey);
    {
        AutoLock oLock(m_oCreateLock);
        ++ m_oReadLock;
    }

    ShmAddrHeader* pShmAddrHeader = reinterpret_cast<ShmAddrHeader*>(m_pShmHeader);
    ShmDataHeader* pHeader = reinterpret_cast<ShmDataHeader*>(m_pShmAddr);
    if (pShmAddrHeader->dwChange != pHeader->dwChange)
    {
        do
        {
            // 如果测试为真，表示服务端正在写操作，使用原来的数据
            if (__sync_lock_test_and_set(&pShmAddrHeader->wSycLock, 1));
                break;

            AutoLock oLock(m_oCreateLock);
            while (m_oReadLock != 1);
            shmdt(m_pShmAddr);

            m_pShmAddr = createShmData(pShmAddrHeader->dwBlockSize);
            __sync_lock_release(&pShmAddrHeader->wSycLock);
            if (!m_pShmAddr)
            {
                -- m_oReadLock;
                return iRet;
            }
            pHeader = reinterpret_cast<ShmDataHeader*>(m_pShmAddr);
        } while (0);
    }

    uint64_t qwHash = ELFHashCode(pszKey);
    if (pHeader->dwHashTableCount == 0)
    {
        -- m_oReadLock;
        return iRet;
    }
    int iIndex = qwHash % pHeader->dwHashTableCount;
    ShmHashTable* pTable = reinterpret_cast<ShmHashTable*>(pHeader->szData);
    uint32_t dwDataAddr = pTable[iIndex].dwDataAddr & HASH_DATA_LENGTH;
    while(dwDataAddr)
    {
        ShmHashData* pData = reinterpret_cast<ShmHashData*>(pHeader->szData + dwDataAddr);
        if (iKeyLen == pData->wKeyLen && strncmp(pszKey, pData->szData, pData->wKeyLen) == 0)
        {
            iRet = 0;
            while (__sync_lock_test_and_set(&pData->wLock, 1));
            sValue.append(pData->szData + pData->wKeyLen, pData->wValueLen);
            __sync_lock_release(&pData->wLock);
            break;
        }

        dwDataAddr = pData->dwDataAddr;
    }

    -- m_oReadLock;
    return iRet;
}

int ShmConfig::checkParam(const char* pszKey)
{
    if (!pszKey)
        return -1;

    if (!m_pShmHeader)
    {
        do
        {
            AutoLock oLock(m_oCreateLock);
            if (m_pShmHeader)
                break;

            if (init() < 0)
                return -1;
        } while (0);
    }

    if (!m_pShmAddr)
    {
        do
        {
            AutoLock oLock(m_oCreateLock);
            if (m_pShmAddr)
                break;

            ShmAddrHeader* pShmAddrHeader = reinterpret_cast<ShmAddrHeader*>(m_pShmHeader);
            if (__sync_lock_test_and_set(&pShmAddrHeader->wSycLock, 1));
                return -1;

            m_pShmAddr = createShmData(pShmAddrHeader->dwBlockSize);
            if (!m_pShmAddr)
                return -1;
        } while (0);
    }
    return 0;
}

void ShmConfig::PrintfAll()
{
    ShmDataHeader* pHeader = reinterpret_cast<ShmDataHeader*>(m_pShmAddr);
    ShmHashTable* pTable = reinterpret_cast<ShmHashTable*>(pHeader->szData);
    for (uint32_t i = 0; i < pHeader->dwHashTableCount; ++ i)
    {
        uint32_t dwDataAddr = pTable[i].dwDataAddr & HASH_DATA_LENGTH;
        while (dwDataAddr)
        {
            ShmHashData* pData = reinterpret_cast<ShmHashData*>(pHeader->szData + dwDataAddr);
            printf("table: %d\tkey: %s\tvalue: %s\n", i, std::string(pData->szData, pData->wKeyLen).c_str(),
                    std::string(pData->szData + pData->wKeyLen, pData->wValueLen).c_str());

            dwDataAddr = pData->dwDataAddr;
        }
    }
}
