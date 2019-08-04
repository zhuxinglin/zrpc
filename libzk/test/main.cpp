
#include "../include/zk_api.h"
#include "libnet.h"
#include <unistd.h>

int main()
{
    znet::CNet::GetObj()->Init(2, 1024 * 10);
    zkapi::IZkApi* pzk = zkapi::IZkApi::CreateObj();
    pzk->Init("220.181.38.150:80,192.169.0.62:2181,192.169.0.63:2181", nullptr, 3U, nullptr);
    znet::CNet::GetObj()->Start();
    return 0;
}