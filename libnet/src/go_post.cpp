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
#include "context.h"

using namespace znet;
extern CContext* g_pContext;

void CGoPost::Post()
{
    CContext* pCx = g_pContext;
    if (pCx->m_dwExecTheadCount != pCx->m_dwWorkThreadCount)
        pCx->m_oSem.Post();
}
