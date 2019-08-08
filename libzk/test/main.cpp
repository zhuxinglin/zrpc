
#include "../include/zk_api.h"
#include "libnet.h"

zkapi::IZkApi* pzk;

class Test : public znet::ITaskBase
{
public:

private:
    virtual void Run()
    {
        int iRet;
        Sleep(1000);
        printf("get start\n");
        std::string v = "test";
        zkproto::zk_stat s;
        zkproto::zk_acl ac;
        ZK_ACL_OPEN(ac);
        //ZK_ACL_WORLD(ac, 0x01);
        std::vector<zkproto::zk_acl> acl;
        acl.push_back(ac);
        std::string r = "{\"***\":\"**\"}";
        // iRet = pzk->Create("/config/cs", v, &acl, 0, r);
        // iRet = pzk->Create("/config/sa", v, &acl, 0, r);
        // iRet = pzk->Delete("/config/cs", -1);
        // iRet = pzk->AddAuth("auth", "123456");
        //iRet = pzk->Exists("/config/sa", 1, &s);
        // iRet = pzk->GetData("/config/cs", 1, r, &s);
        // iRet = pzk->SetData("/config/cs", r, -1, &s);
        // std::vector<std::string> ret;
        // iRet = pzk->GetChildern("/config", 1, ret);
        // iRet = pzk->GetChildern("/config", 1, ret, &s);
        // for (auto it : ret)
        // {
        //     printf("%s\n", it.c_str());
        // }
        // iRet = pzk->GetAcl("/config", acl, &s);
        // iRet = pzk->SetAcl("/config", -1, acl);
        // iRet = pzk->Sync("/config");
        std::vector<zkapi::zoo_op_t> ops;
        zkapi::zoo_op_t op;
        zkapi::IZkApi::CreateOpInit(&op, "/testop", v, acl, 1);
        ops.push_back(op);
        // zkapi::IZkApi::DeleteOpInit(op, );
        // zkapi::IZkApi::SetOpInit(op, );
        // zkapi::IZkApi::CheckOpInit(op, );
        std::vector<zkapi::zoo_op_result_t> result;
        iRet = pzk->Multi(ops, &result);
        printf("%d  %s %s\n", iRet, v.c_str(), pzk->GetErr());
//        exit(0);
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