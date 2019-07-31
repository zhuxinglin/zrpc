/*
* zk api
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

#include "zk_proto_mgr.h"

using namespace zkapi;

IZkApi* IZkApi::CreateObj()
{
    return new ZkProtoMgr;
}
