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

#include "config_shm.h"
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace zplugin;

constexpr const uint8_t wBackSize = 64;

ConfigShm::ConfigShm()
{
    m_pShmAddrHeader = nullptr;
}

ConfigShm::~ConfigShm()
{
    // 断开连接
    if (m_pBlockPtr)
        shmdt(m_pBlockPtr);
}

int ConfigShm::Init()
{
    int iShmAddrHeaderId = shmget(SHM_ADDR_HEADER_KEY, sizeof(ShmAddrHeader), 0666);
    if (iShmAddrHeaderId < 0)
    {
        iShmAddrHeaderId = shmget(SHM_ADDR_HEADER_KEY, sizeof(ShmAddrHeader), 0666 | IPC_CREAT);
        if (iShmAddrHeaderId < 0)
        {
            LOGE_BIZ(SHM_CACHE) << "get shm addr header failed";
            return -1;
        }

        m_pShmAddrHeader = reinterpret_cast<ShmAddrHeader*>(shmat(iShmAddrHeaderId, NULL, 0));
        memset(m_pShmAddrHeader, 0, sizeof(ShmAddrHeader));

        return 0;
    }

    m_pShmAddrHeader = reinterpret_cast<ShmAddrHeader*>(shmat(iShmAddrHeaderId, NULL, 0));

    if (CreateShmMem(m_pShmAddrHeader->dwBlockSize) < 0)
        return -1;

    if (m_pShmAddrHeader->dwBlockSize == 0)
        return 0;       // 不需要加载

    ShmHashTable* pTable = reinterpret_cast<ShmHashTable*>(m_pBlockPtr->szData);
    for (uint32_t i = 0; i < m_pBlockPtr->dwHashTableCount; ++ i)
    {
        if (pTable[i].dwDataAddr == 0)
            continue;

        // 读取所有KEY
        GetShmDataKey(m_pBlockPtr->szData, pTable[i].dwDataAddr);
    }

    return 0;
}

// 连接共享内存
int ConfigShm::CreateShmMem(uint32_t dwBlockSize, uint32_t dwFlags)
{
    if (dwBlockSize == 0)
        return 0;       // 不需要连接

    m_iShmDataId = shmget(SHM_ADDR_DATA_KEY, dwBlockSize, 0666 | dwFlags);
    if (m_iShmDataId < 0)
    {
        LOGE_BIZ(SHM_CACHE) << "open shm chacke failed, key1 :" << SHM_ADDR_DATA_KEY << ", size : " << dwBlockSize;
        return -1;
    }

    m_pBlockPtr = reinterpret_cast<ShmDataHeader*>(shmat(m_iShmDataId, NULL, 0));
    if (dwFlags != 0)
        memset(m_pBlockPtr, 0, dwBlockSize);

    return 0;
}

void ConfigShm::GetShmDataKey(char* pszBlockAddr, uint32_t dwLen)
{
    uint32_t dwMark = dwLen & HASH_COLLIDE_MARK;
    uint32_t dwOffset = dwLen & HASH_DATA_LENGTH;
    do
    {
        ShmHashData* pData = reinterpret_cast<ShmHashData*>(pszBlockAddr + dwOffset);

        uint16_t wKeyLen = pData->wKeyLen;
        m_mapConfigInfo.insert(std::map<std::string, std::string>::value_type(std::string(pData->szData, wKeyLen),
                                std::string(pData->szData + wKeyLen, pData->wValueLen)));
        LOGD_BIZ(SHM_CACHE) << "key: " << std::string(pData->szData, wKeyLen) << ", value: " << std::string(pData->szData + wKeyLen, pData->wValueLen);
        if (dwMark && pData->dwDataAddr != 0)
            dwOffset = pData->dwDataAddr;
        else
            dwMark = 0;
    } while(dwMark);
}

int ConfigShm::CompareShmKey(std::map<std::string, std::string>& mapConfig)
{
    int iRet = 0;
    for (auto it = m_mapConfigInfo.begin(); it != m_mapConfigInfo.end();)
    {
        auto iter = mapConfig.find(it->first);
        if (iter == mapConfig.end())
        {
            // 已经删除, 需要更新共享内存
            iRet = -1;
            m_mapConfigInfo.erase(it ++);
            continue;
        }

        if (iter->second.compare(it->second) != 0)
        {
            // 值发生变化，需要更新共享内存
            it->second = iter->second;
            iRet = -1;
        }

        // 清除临时数据
        mapConfig.erase(iter);
        ++ it;
    }

    if (mapConfig.empty())
        return iRet;

    iRet = -1;
    for (auto it = mapConfig.begin(); it != mapConfig.end(); ++it)
        m_mapConfigInfo.insert(std::map<std::string, std::string>::value_type(it->first, it->second));

    return iRet;
}

int ConfigShm::WriteShmMemory()
{
    return writeShmData();
}

int ConfigShm::writeShmData()
{
    // 计算需要的内存大小
    uint32_t dwShmMemorySize = 0;
    // 数据大小
    uint32_t dwDataSize = 0;
    // hash表大小
    uint32_t dwHashTable = m_mapConfigInfo.size() * sizeof(ShmHashTable);
    // 计算数据部分大小
    for (auto it = m_mapConfigInfo.begin(); it != m_mapConfigInfo.end(); ++ it)
        dwDataSize += it->first.length() + it->second.length() + sizeof(ShmHashData) + wBackSize;

    // 计算hash部分大小
    dwShmMemorySize = sizeof(ShmDataHeader) + dwDataSize + dwHashTable;

    // 存在当上一次更改了内存，客户端此时检查到dwChange已经变更, 现在马上又要更改，
    // 此时应该保证客户端成功，不应该去删除共享内存，让客户端去连接上一次
    while(__sync_lock_test_and_set(&m_pShmAddrHeader->wSycLock, 1));
    m_pShmAddrHeader->dwBlockSize = dwShmMemorySize;
    if (m_iShmDataId >= 0 && m_pBlockPtr)
    {
        // 断开连接
        shmdt(m_pBlockPtr);
        m_pBlockPtr = nullptr;
        // 删除共享内存
        // 删除后还存在连接的KEY，将变为0
        shmctl(m_iShmDataId, IPC_RMID, nullptr);
        m_iShmDataId = -1;
    }

    // 失败尝试10次
    int iCount = 10;
    while (iCount > 0)
    {
        if (CreateShmMem(dwShmMemorySize, IPC_CREAT) < 0)
        {
            -- iCount;
            continue;
        }
        break;
    }
    // 如果失败了，同步锁wSycLock不能清0，客户端仍然使用上一次的变更
    if (iCount < 0)
        return -1;

    uint32_t dwChange = m_pShmAddrHeader->dwChange;
    ++ dwChange;
    // 填冲头
    m_pBlockPtr->dwChange = dwChange;
    m_pBlockPtr->dwHashTableCount = m_mapConfigInfo.size();

    for (auto it = m_mapConfigInfo.begin(); it != m_mapConfigInfo.end(); ++ it)
        insertData(it->first, it->second, dwHashTable, m_mapConfigInfo.size());

    // 通知更新成功
    m_pShmAddrHeader->dwChange = dwChange;
    __sync_lock_release(&m_pShmAddrHeader->wSycLock);
    return 0;
}

void ConfigShm::insertData(const std::string& sKey, const std::string& sValue, uint32_t& dwInsertOffset, uint32_t dwCount)
{
    uint64_t qwHash = ELFHashCode(sKey.c_str());
    int iIndex = qwHash % dwCount;

    ShmHashTable* pTable = reinterpret_cast<ShmHashTable*>(m_pBlockPtr->szData);
    ShmHashData* pData;
    uint32_t dwDataAddr = pTable[iIndex].dwDataAddr;
    if (dwDataAddr == 0)
    {
        pTable[iIndex].dwDataAddr = dwInsertOffset;
        writeData(sKey, sValue, dwInsertOffset);
        return ;
    }

    // 添加冲突位
    pTable[iIndex].dwDataAddr = dwDataAddr | HASH_COLLIDE_MARK;

    pData = reinterpret_cast<ShmHashData*>(m_pBlockPtr->szData + (dwDataAddr & HASH_DATA_LENGTH));
    while (pData->dwDataAddr != 0)
        pData = reinterpret_cast<ShmHashData*>(m_pBlockPtr->szData + pData->dwDataAddr);

    // 添加冲突长度
    pData->dwDataAddr = dwInsertOffset;
    writeData(sKey, sValue, dwInsertOffset);
}

void ConfigShm::writeData(const std::string& sKey, const std::string& sValue, uint32_t& dwInsertOffset)
{
    ShmHashData* pData = reinterpret_cast<ShmHashData*>(m_pBlockPtr->szData + dwInsertOffset);
    pData->wKeyLen = sKey.length();
    pData->wValueLen = sValue.length();
    pData->wBackLen = wBackSize;
    pData->dwDataAddr = 0;
    dwInsertOffset += sizeof(ShmHashData) + sKey.length() + sValue.length() + wBackSize;
    memcpy(pData->szData, sKey.c_str(), sKey.length());
    memcpy(pData->szData + sKey.length(), sValue.c_str(), sValue.length());
}

int ConfigShm::WriteShmData(const std::string& sKey, const std::string& sValue)
{
    if (AddData(sKey, sValue) == 0)
        return 0;

    writeShmData();
    return 0;
}

int ConfigShm::AddData(const std::string& sKey, const std::string& sValue)
{
    uint64_t qwHash = ELFHashCode(sKey.c_str());
    int iIndex = qwHash % m_mapConfigInfo.size();
    ShmHashTable* pTable = reinterpret_cast<ShmHashTable*>(m_pBlockPtr->szData);

    uint32_t dwDataAddr = pTable[iIndex].dwDataAddr;
    if (dwDataAddr == 0)
    {
        LOGW_BIZ(SHM_CACHE) << "find shm key failed, key :" << sKey << ", value : " << sValue;
        return 0;
    }

    {
        auto it = m_mapConfigInfo.find(sKey);
        if (it == m_mapConfigInfo.end())
        {
            LOGW_BIZ(SHM_CACHE) << "find key failed, key :" << sKey << ", value : " << sValue;
            return 0;
        }
        it->second = sValue;
    }

    ShmHashData* pData;
    pData = reinterpret_cast<ShmHashData*>(m_pBlockPtr->szData + (dwDataAddr & HASH_DATA_LENGTH));
    if (dwDataAddr & HASH_COLLIDE_MARK)
    {
        dwDataAddr &= HASH_DATA_LENGTH;
        do
        {
            if (strncmp(sKey.c_str(), pData->szData, pData->wKeyLen) == 0)
                break;

            while (pData && pData->dwDataAddr != 0)
            {
                pData = reinterpret_cast<ShmHashData*>(m_pBlockPtr->szData + pData->dwDataAddr);

                if (strncmp(sKey.c_str(), pData->szData, pData->wKeyLen) == 0)
                    break;
            }
        }while(0);
    }

    // 是否有hash冲突
    if (sValue.length() > (pData->wValueLen + pData->wBackLen))
        return -1;

    if (pData->wValueLen > sValue.length())
        pData->wBackLen += (pData->wValueLen - sValue.length());
    else
        pData->wBackLen -= (sValue.length() - pData->wValueLen);

    while (__sync_lock_test_and_set(&pData->wLock, 1));
    pData->wValueLen = sValue.length();
    memcpy(pData->szData + pData->wKeyLen, sValue.c_str(), sValue.length());
    __sync_lock_release(&pData->wLock);

    return 0;
}

void ConfigShm::AddShmKey(std::vector<std::string>& vKey)
{
    for (auto iter = vKey.begin(); iter != vKey.end();)
    {
        auto it = m_mapConfigInfo.find(*iter);
        if (it != m_mapConfigInfo.end())
        {
            iter = vKey.erase(iter);
            continue;
        }
        ++ iter;
    }
}

void ConfigShm::UpdateMap(std::map<std::string, std::string>& mapConfig)
{
    for (auto it = mapConfig.begin(); it != mapConfig.end(); ++ it)
    {
        auto iter = m_mapConfigInfo.find(it->first);
        if (iter != m_mapConfigInfo.end())
        {
            it->second = iter->second;
            continue;
        }

        m_mapConfigInfo.insert(std::map<std::string, std::string>::value_type(it->first, it->second));
    }
}

void ConfigShm::DeleteKey(const std::string& path)
{
    m_mapConfigInfo.erase(path);
}
