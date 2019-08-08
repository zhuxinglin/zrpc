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
    op->path = pszPath;
    op->data = sValue;
    op->acl = acl;
    op->flags = flags;
}

void IZkApi::DeleteOpInit(zoo_op_t *op, const char *pszPath, int version)
{
    op->type = ZOO_DELETE_OP;
    op->path = pszPath;
    op->version = version;
}

void IZkApi::SetOpInit(zoo_op_t *op, const char *pszPath, const std::string &sBuffer, int version,
                      zkproto::zk_stat *stat)
{
    op->type = ZOO_SETDATA_OP;
    op->path = pszPath;
    op->data = sBuffer;
    op->version = version;
    op->stat = *stat;
}

void IZkApi::CheckOpInit(zoo_op_t *op, const char *pszPath, int version)
{
    op->type = ZOO_CHECK_OP;
    op->path = pszPath;
    op->version = version;
}
