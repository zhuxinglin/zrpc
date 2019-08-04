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

void IZkApi::CreateOpInit(zoo_op_t *op, const char *pszPath, const std::string &sValue,
                         const std::vector<zkproto::zk_acl> &acl, int flags)
{
    op->type = ZOO_CREATE_OP;
    op->create_op.path = pszPath;
    op->create_op.data = sValue;
    op->create_op.acl = acl;
    op->create_op.flags = flags;
}

void IZkApi::DeleteOpInit(zoo_op_t *op, const char *pszPath, int version)
{
    op->type = ZOO_DELETE_OP;
    op->delete_op.path = pszPath;
    op->delete_op.version = version;
}

void IZkApi::SetOpInit(zoo_op_t *op, const char *pszPath, const std::string &sBuffer, int version,
                      zkproto::zk_stat *stat)
{
    op->type = ZOO_SETDATA_OP;
    op->set_op.path = pszPath;
    op->set_op.data = sBuffer;
    op->set_op.version = version;
    op->set_op.stat = stat;
}

void IZkApi::CheckOpInit(zoo_op_t *op, const char *pszPath, int version)
{
    op->type = ZOO_CHECK_OP;
    op->check_op.path = pszPath;
    op->check_op.version = version;
}
