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

#include "include/shm_config_go.h"
#include "include/shm_config.h"
#include <string.h>
#include <stdlib.h>


int GetValue(const char* pszKey, char** pszValue)
{
    *pszValue = 0;
    std::string sVar = SHM_CONF->GetValue(pszKey);
    if (sVar.empty())
        return -1;

    *pszValue = (char*)malloc(sVar.size());
    memcpy(*pszValue, sVar.c_str(), sVar.size());
    return sVar.size();
}

void FreeValue(char* pszValue)
{
    if (pszValue)
        free(pszValue);
}

uint64_t GetValueI(const char* pszKey, uint64_t a)
{
    return SHM_CONF->GetValueI(pszKey, a);
}

void CloseShm()
{
    SHM_CONF->Close();
}
