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


#ifndef __CONFIG_SHM_H__
#define __CONFIG_SHM_H__

#include "plugin_base.h"
#include <set>
#include <map>
#include <string>
#include <vector>
#include "./include/shm_struct.h"
#include "co_lock.h"

namespace zplugin
{

class ConfigShm
{
public:
    ConfigShm();
    ~ConfigShm();

public:
    int Init();
    int CompareShmKey(std::map<std::string, std::string>& mapConfig);
    int WriteShmMemory();
    int WriteShmData(const std::string& sKey, const std::string& sValue);
    void AddShmKey(std::vector<std::string>& vKey);
    void UpdateMap(std::map<std::string, std::string>& mapConfig);
    void DeleteKey(const std::string& path);

private:
    int CreateShmMem(uint32_t dwBlockSize, uint32_t dwFlags = 0);
    void GetShmDataKey(char* pszBlockAddr, uint32_t dwLen);
    const bool IsDone() const{return m_bIsSuccessStatus;}
    int writeShmData();
    void insertData(const std::string& sKey, const std::string& sValue, uint32_t& dwInsertOffset, uint32_t dwCount);
    void writeData(const std::string& sKey, const std::string& sValue, uint32_t& dwInsertOffset);
    int AddData(const std::string& sKey, const std::string& sValue);

private:
    std::map<std::string, std::string> m_mapConfigInfo;
    ShmAddrHeader *m_pShmAddrHeader;
    int m_iShmDataId{0};        // 共享块ID
    ShmDataHeader* m_pBlockPtr{0};               // 共享块地址指针
    bool m_bIsSuccessStatus{true};
};

}

#endif

