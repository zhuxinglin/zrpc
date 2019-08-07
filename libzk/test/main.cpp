
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
        /*zkproto::zk_stat s;
        zkproto::zk_acl ac;
        ac.perms = 7;
        ac.id.scheme = "ip";
        ac.id.id = "127.0.0.1";
        std::vector<zkproto::zk_acl> acl;
        acl.push_back(ac);
        std::string r;
        int iRet = pzk->Create("/config/cs", v, &acl, 4, r);*/
        int iRet = pzk->Delete("/config/cs", -1);
        printf("%d  %s %s\n", iRet, v.c_str(), pzk->GetErr());
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
    pzk->Init("127.0.0.1:2181", new Watch, 3U, 0, nullptr);
    znet::CNet::GetObj()->Register(new Test, 0, znet::ITaskBase::PROTOCOL_TIMER, -1, 0);
    znet::CNet::GetObj()->Start();
    return 0;
}