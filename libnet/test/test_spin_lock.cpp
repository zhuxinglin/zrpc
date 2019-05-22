
#include "thread.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

volatile uint32_t g_dwSync = 0;

class CTestSpinLock : public CThread
{
private:
    void Run(uint32_t dwId)
    {
        CSpinLock oLock(&g_dwSync);
        printf("99999999\n");
        sleep(5);
        printf("!1111111\n");
    }
public:
    CTestSpinLock(/* args */);
    ~CTestSpinLock();
};

CTestSpinLock::CTestSpinLock(/* args */)
{
}

CTestSpinLock::~CTestSpinLock()
{
}


int main(int argc, char const *argv[])
{
    CTestSpinLock oTest;
    oTest.Start();
    usleep(10000);

    CSpinLock oLock(g_dwSync);
    printf("sssdfdsdfsdfsdfsdf\n");
    return 0;
}




