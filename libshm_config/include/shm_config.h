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

#ifndef __SHM_CONFIG_H__
#define __SHM_CONFIG_H__

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <atomic>

#define SHM_CONF    shmmemory::ShmConfig::Instance()

namespace shmmemory
{

class ShmConfig
{
public:
    ShmConfig();
    ~ShmConfig();

public:
    static ShmConfig* Instance();

    std::string GetValue(const char* pszKey, const char* pszDefaultValue = nullptr);
    float GetValueF(const char* pszKey, const float fDefaultValue = 0.0f);
    double GetValueD(const char* pszKey, const double dDefaultValue = 0.0f);

    template<typename TY>
    TY GetValueI(const char* pszKey, TY tDefaultValue = 0)
    {
        std::string sValue;
        int iRet = searchKey(pszKey, sValue);
        if (iRet != 0)
            return tDefaultValue;

        long lValue = strtol(sValue.c_str(), NULL, 10);
        return static_cast<TY>(lValue);
    }

    void PrintfAll();

    void Close();

private:
    int init();
    void* createShmHeader();
    void* createShmData(uint32_t dwBlockSize);
    int searchKey(const char* pszKey, std::string& sValue);
    int checkParam(const char* pszKey);

private:
    void* m_pShmHeader{nullptr};
    void* m_pShmAddr{nullptr};
    std::atomic_flag m_oCreateLock{0}; // 创建锁互斥
    std::atomic_char32_t m_oReadLock{0};   // 读共享计数锁
};

}

#endif
