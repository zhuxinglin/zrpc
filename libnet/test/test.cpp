/*
*
*
*
*
*
*
*
*/



#include "core/coroutine.h"
#include "task_base.h"
#include <stdio.h>
#include <unistd.h>

class Ctest : public ITaskBase
{
public:
	Ctest();
	~Ctest();

	virtual void Run();
};
Ctest *pT1;
Ctest *pT2;

Ctest::Ctest()
{
}

Ctest::~Ctest()
{
}

void Ctest::Run()
{
	int i = 0, j = 0;
	while (true)
	{
		printf("2222222222222222  %u  %d\n", m_dwTid, i);
		sleep(1);

		if (j == 10)
		{
			CCoroutine::GetObj()->Swap(this, 1);
		}

		++j;
		/*
		void* pSp = 0;
		void* p = 0;
		__asm__ __volatile__("movq $~(1048576), %%rax\n"
						"andq %%rsp, %%rax\n"
						"movq %%rax, %0\n"
						"movq %%rsp, %1"
						:"=r"(pSp),"=r"(p));
		printf("[[[[[[[[[[[[[ %p    %p\n", pSp, p);*/
		if (i == 0)
		{
			CCoroutine::GetObj()->Swap(pT2, this);
			i = 1;
		}
		else
		{
			CCoroutine::GetObj()->Swap(pT1, this);
			i = 0;
		}
	}
}

int main()
{
	pT1 = new Ctest();
	pT2 = new Ctest();
	CCoroutine *pCon = CCoroutine::GetObj();
	pCon->SetContext();
	pCon->Create(pT1);
	printf("*****************###********* %s\n", pCon->GetErr());
	pCon->Create(pT2);
	printf("************************** %s\n", pCon->GetErr());
	pCon->Swap(pT1);
	pCon->Release();
	pT1->Release();
	pT2->Release();
	return 0;
}


