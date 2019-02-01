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

#ifndef __MEMORY__POOL__H__
#define __MEMORY__POOL__H__

namespace znet
{

class CMemoryPool
{
public:
	CMemoryPool(int iSize = 4096, int iMaxNodeCount = 1000);
	~CMemoryPool();

public:
	void* Malloc();
	void Free(void* pAddr);
	void FreeAll();
	int Size();
	void SetSize(int iSize);
	void SetMaxNodeCount(int iMaxNodeCount);
	int Count() { return m_iCurNodeCount; }

	template <typename T>
	void GetUse(T *pThis, int (T::*fun)(void *, void *), void *pData = 0)
	{
		int i = 0;
		while (i < m_iUseCount)
		{
			++i;
			PoolNode *pDel = GetUseing();
			if (!pDel)
				continue;

			if ((pThis->*fun)(pDel->szData, pData) < 0)
			{
				AddUse(pDel);
				Free(pDel->szData);
				break;
			}
			AddUse(pDel);
		}
	}
	
	void GetUse(int (*fun)(void*, void *), void* pData = 0);

private:
	struct PoolNode
	{
		PoolNode* pPrev;
		PoolNode* pNext;
		char szData[0];
	};

	PoolNode* m_pUseHead;
	PoolNode* m_pUseTail;
	int m_iUseCount;
	PoolNode* m_pFreeHead;
	PoolNode* m_pFreeTail;
	int m_iFreeCount;
	int m_iSize;
	int m_iMaxNodeCount;
	int m_iCurNodeCount;
	volatile int m_iSync;

private:
	void AddUse(PoolNode *pNode);
	PoolNode *GetUseing();
};

}

#endif

