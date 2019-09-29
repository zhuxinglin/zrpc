package shmconfig

/*
#cgo LDFLAGS:-L -lshmconfig
#cgo CFLAGS: -I./
#include "shm_config.h"

#ifdef __cplusplus
extern "C"{
#endif

int GetValue(const char* pszKey, char** pszValue)
{
    *pszValue = 0;
    std::string sVar = SHM_CONF->GetValue(pszKey);
    if (sVar.empty())
        return -1;

    *pszValue = new char[sVar.size()];
    memcpy(*pszValue, sVar.c_str(), sVar.size());
    return sVar.size();
}

void FreeValue(char* pszValue)
{
    if (pszValue)
        delete pszValue;
}

uint64_t GetValueI(const char* pszKey, uint64_t a)
{
    return SHM_CONF->GetValueI(pszKey, a);
}

void CloseShm()
{
    SHM_CONF->Close();
}

#ifdef __cplusplus
}
#endif
*/
import "C"
