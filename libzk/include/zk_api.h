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
