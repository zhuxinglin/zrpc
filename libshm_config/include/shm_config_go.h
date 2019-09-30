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

#ifndef __SHM_CONFIG_GO_H__
#define __SHM_CONFIG_GO_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int GetValue(const char* pszKey, char** pszValue);

void FreeValue(char* pszValue);

uint64_t GetValueI(const char* pszKey, uint64_t a);

void CloseShm();

#ifdef __cplusplus
}
#endif

#endif // __SHM_CONFIG_GO_H__
