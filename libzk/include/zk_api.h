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

#ifndef __ZK_API__H__
#define __ZK_API__H__

#include <stdint.h>

namespace zkapi
{

#define ZOO_NOTIFY_OP 0
#define ZOO_CREATE_OP 1
#define ZOO_DELETE_OP 2
#define ZOO_EXISTS_OP 3
#define ZOO_GETDATA_OP 4
#define ZOO_SETDATA_OP 5
#define ZOO_GETACL_OP 6
#define ZOO_SETACL_OP 7
#define ZOO_GETCHILDREN_OP 8
#define ZOO_SYNC_OP 9
#define ZOO_PING_OP 11
#define ZOO_GETCHILDREN2_OP 12
#define ZOO_CHECK_OP 13
#define ZOO_MULTI_OP 14
#define ZOO_CLOSE_OP -11
#define ZOO_SETAUTH_OP 100
#define ZOO_SETWATCHES_OP 101

struct IWatcher
{
    virtual void OnWatcher(int type, int state, const char* path) = 0;
};

struct clientid_t
{
    int64_t client_id;
    char password[16];
};

struct IZkApi
{
    static IZkApi* CreateObj();
    virtual const char* GetErr() = 0;
    virtual int Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId) = 0;
    virtual void Close() = 0;
};

}

#endif
