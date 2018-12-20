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

#include "memory_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "uconfig.h"

CMemoryPool::CMemoryPool(int iSize, int iMaxNodeCount) : 
m_pUseHead(0),
m_pUseTail(0),
m_iUseCount(0),
m_pFreeHead(0),
m_pFreeTail(0),
m_iFreeCount(0),
m_iSize(iSize),
m_iMaxNodeCount(iMaxNodeCount),
m_iCurNodeCount(0),
m_iSync(0)
{
}

CMemoryPool::~CMemoryPool()
{
	FreeAll();
	while(m_pUseHead)
	{
		PoolNode* pDel = m_pUseHead;
		m_pUseHead = m_pUseHead->pNext;
		free(pDel);
	}
	m_pUseTail = 0;
}

void* CMemoryPool::Malloc()
{
	PoolNode* pNode = 0;
	while (__sync_lock_test_and_set(&m_iSync, 1))_usleep_();
	if (!m_pFreeHead)
	{
		__sync_lock_release(&m_iSync);
		pNode = (PoolNode*)malloc(m_iSize + sizeof(PoolNode));
		if (__builtin_expect(pNode == 0, 1))
			return 0;

		pNode->pNext = 0;
		pNode->pPrev = 0;

		while (__sync_lock_test_and_set(&m_iSync, 1))
			_usleep_();
		++ m_iCurNodeCount;
	}
	else
	{
		pNode = m_pFreeHead;
		m_pFreeHead = m_pFreeHead->pNext;
		if (!m_pFreeHead)
			m_pFreeTail = 0;
		-- m_iFreeCount;
		pNode->pNext = 0;
		pNode->pPrev = 0;
	}
	
	if (m_pUseHead)
	{
		m_pUseTail->pNext = pNode;
		pNode->pPrev = m_pUseTail;
	}
	else
		m_pUseHead = pNode;

	++ m_iUseCount;
	m_pUseTail = pNode;
	__sync_lock_release(&m_iSync);
	return pNode->szData;
}

void CMemoryPool::Free(void* pAddr)
{
	PoolNode* pNode = (PoolNode*)((char*)pAddr - sizeof(PoolNode));
	while (__sync_lock_test_and_set(&m_iSync, 1))
		_usleep_();
	PoolNode* pH = pNode->pPrev;
	PoolNode* pE = pNode->pNext;
	if (pH)
	{
		pH->pNext = pE;
		if (pE)
			pE->pPrev = pH;
		else
			m_pUseTail = pH;
	}
	else if (pE)
	{
		m_pUseHead = pE;
		pE->pPrev = 0;
	}
	else
	{
		m_pUseHead = 0;
		m_pUseTail = 0;
	}
	-- m_iUseCount;

	if (-1 != m_iMaxNodeCount && m_iCurNodeCount > m_iMaxNodeCount)
	{
		--m_iCurNodeCount;
		__sync_lock_release(&m_iSync);
		free(pNode);
		return;
	}

	++ m_iFreeCount;
	if (m_pFreeTail)
	{
		m_pFreeTail->pNext = pNode;
		pNode->pPrev = m_pFreeTail;
	}
	else
	{
		m_pFreeHead = pNode;
		pNode->pPrev = 0;
	}
	m_pFreeTail = pNode;
	pNode->pNext = 0;
	__sync_lock_release(&m_iSync);
}

int CMemoryPool::Size()
{
	return m_iSize;
}

void CMemoryPool::FreeAll()
{
	while(m_pFreeHead)
	{
		PoolNode* pDel = m_pFreeHead;
		m_pFreeHead = m_pFreeHead->pNext;
		free(pDel);
	}
	m_pFreeTail = 0;
}

void CMemoryPool::SetSize(int iSize)
{
	m_iSize = iSize;
}

void CMemoryPool::SetMaxNodeCount(int iMaxNodeCount)
{
	m_iMaxNodeCount = iMaxNodeCount;
}

void CMemoryPool::GetUse(int (*fun)(void *, void *), void *pData)
{
	int i = 0;
	while (i < m_iUseCount)
	{
		++ i;
		PoolNode *pDel = GetUseing();
		if (!pDel)
			continue;

		if (fun(pDel->szData, pData) < 0)
		{
			AddUse(pDel);
			Free(pDel->szData);
			break;
		}

		AddUse(pDel);
	}
}

void CMemoryPool::AddUse(PoolNode *pNode)
{
	while (__sync_lock_test_and_set(&m_iSync, 1))
		_usleep_();

	if (!m_pUseHead)
	{
		m_pUseHead = pNode;
		m_pUseTail = pNode;
	}
	else
	{
		m_pUseTail->pNext = pNode;
		pNode->pPrev = m_pUseTail;
		m_pUseTail = pNode;
	}
	__sync_lock_release(&m_iSync);
}

CMemoryPool::PoolNode *CMemoryPool::GetUseing()
{
	while (__sync_lock_test_and_set(&m_iSync, 1))
		_usleep_();

	PoolNode *pDel = m_pUseHead;
	m_pUseHead = pDel->pNext;
	if (!m_pUseHead)
		m_pUseTail = 0;
	else
		m_pUseHead->pPrev = 0;

	__sync_lock_release(&m_iSync);

	pDel->pPrev = 0;
	pDel->pNext = 0;
	return pDel;
}
