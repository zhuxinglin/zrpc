
#include "libnet.h"
#include "co_chan.h"

using namespace znet;

CCoChan<int> g_oTest(1);

class CTestReadChan : public ITaskBase
{
private:
    virtual void Run()
    {
        long i = reinterpret_cast<long>(m_pData);
        while (1)
        {
            int a;
            g_oTest >> a;
            if (i == 0)
                printf("RRRRRRRRRRRRrrrrrrr %d      %li\n", a, i);
            else
                printf("============rrrrrrr %d       %li\n", a, i);
        }
    }

    virtual void Error(const char *pszExitStr) 
    {
        printf("%s\n", pszExitStr);
    }
};

class CTestWriteChan : public ITaskBase
{
private:
    virtual void Run()
    {
        while (1)
        {
            int a = 100;
            g_oTest << a;
            printf("@@@@@@@@@@@@@@@@@@@@@@\n");
        }
    }

    virtual void Error(const char *pszExitStr)
    {
        printf("%s\n", pszExitStr);
    }
};

ITaskBase* NewReadObj()
{
    return new CTestReadChan;
}

ITaskBase *NewWriteObj()
{
    return new CTestWriteChan;
}

int main()
{
    CNet::GetObj()->Init(2, 100000);

    CNet::GetObj()->Register(NewReadObj, 0, ITaskBase::PROTOCOL_TIMER, -1, 0);
    CNet::GetObj()->Register(NewReadObj, reinterpret_cast<void*>(1L), ITaskBase::PROTOCOL_TIMER, -1, 0);

    CNet::GetObj()->Register(NewWriteObj, 0, ITaskBase::PROTOCOL_TIMER, -1, 0);

    CNet::GetObj()->Start();
    return 0;
}
