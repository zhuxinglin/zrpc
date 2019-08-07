
#include "../include/zk_api.h"
#include "libnet.h"

zkapi::IZkApi* pzk;

class Test : public znet::ITaskBase
{
public:

private:
    virtual void Run()
    {
        Sleep(1000);
        printf("get start\n");
        std::string v = "test";
        //zkproto::zk_stat s;
        zkproto::zk_acl ac;
        ac.perms = 7;
        ac.id.scheme = "world";
        ac.id.id = "anyone";
        std::vector<zkproto::zk_acl> acl;
        acl.push_back(ac);
        std::string r;
        int iRet = pzk->Create("/config", v, &acl, 1, r);
        printf("%d  %s %s\n", iRet, r.c_str(), pzk->GetErr());
    }
};

class Watch : public zkapi::IWatcher
{
public:
private:
    virtual void OnWatcher(int type, int state, const char* path)
    {
        printf("path: %s\n", path);
    }
};

int main()
{
    znet::CNet::GetObj()->Init(2, 1024 * 10);
    pzk = zkapi::IZkApi::CreateObj();
    pzk->Init("192.169.0.60:3181", new Watch, 3U, 0, nullptr);
    znet::CNet::GetObj()->Register(new Test, 0, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);
    znet::CNet::GetObj()->Start();
    return 0;
}