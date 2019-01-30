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

#include "go_post.h"
#include "go.h"
#include <stdint.h>

extern CGo *g_pGo;
extern uint32_t g_dwWorkThreadCount;

void CGoPost::Post()
{
    static uint32_t dwGoIndex = 0;
    static volatile uint32_t dwIndexSync = 0;
    uint32_t dwIn;
    {
        CSpinLock oLock(dwIndexSync);
        dwIn = dwGoIndex ++;
        if (dwGoIndex >= g_dwWorkThreadCount)
            dwGoIndex = 0;
    }
    g_pGo[dwIn].PushMsg(0, 0, 0, 0);
}
