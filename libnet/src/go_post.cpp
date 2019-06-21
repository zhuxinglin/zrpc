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
#include <atomic>

using namespace znet;

extern std::atomic_uint g_dwExecTheadCount;
extern uint32_t g_dwWorkThreadCount;
extern CSem g_oSem;

void CGoPost::Post()
{
    if (g_dwExecTheadCount != g_dwWorkThreadCount)
        g_oSem.Post();
}
