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

using namespace znet;

extern CGo *g_pGo;
extern uint32_t g_dwWorkThreadCount;
extern volatile uint32_t* g_pIsPost;

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

    if (__sync_bool_compare_and_swap(&g_pIsPost[dwIn], 1, 1))
        return ;

    g_pGo[dwIn].PushMsg(0, 0, 0, 0);
}
