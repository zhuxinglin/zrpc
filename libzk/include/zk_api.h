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
#include <vector>
#include <string>
#include "zk_protocol.h"

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

typedef struct zoo_op {
    int type;
    union {
        // CREATE
        struct {
            std::string path;
            std::string data;
            std::vector<zkproto::zk_acl> acl;
            int flags;
        } create_op;

        // DELETE 
        struct {
            std::string path;
            int version;
        } delete_op;
        
        // SET
        struct {
            std::string path;
            std::string data;
            int version;
            struct zkproto::zk_stat *stat;
        } set_op;
        
        // CHECK
        struct {
            std::string path;
            int version;
        } check_op;
    };
} zoo_op_t;

struct zoo_op_result_t
{
    zoo_op_result_t() = default;
    int err;
    std::vector<std::string> value;
    struct zkproto::zk_stat stat;
    std::vector<zkproto::zk_acl> acl;
};

struct IZkApi
{
    static IZkApi* CreateObj();
    static void CreateOpInit(zoo_op_t* op, const char* pszPath, const std::string& sValue,
        const std::vector<zkproto::zk_acl>& acl, int flags);
    static void DeleteOpInit(zoo_op_t *op, const char* pszPath, int version);
    static void SetOpInit(zoo_op_t *op, const char *pszPath, const std::string &sBuffer, 
                int version, zkproto::zk_stat *stat);
    static void CheckOpInit(zoo_op_t *op, const char* pszPath, int version);

public:
    virtual const char* GetErr() = 0;
    virtual int Init(const char *pszHost, IWatcher *pWatcher, uint32_t dwTimeout, const clientid_t *pClientId) = 0;
    virtual void Close() = 0;
    virtual int AddAuth(const char *pszScheme, const std::string sCert) = 0;
    virtual int Create(const char *pszPath, const std::string &sValue, 
                const std::vector<zkproto::zk_acl> *acl, int flags, std::string& sPathBuffer) = 0;
    virtual int Delete(const char* pszPath, int version) = 0;
    virtual int Exists(const char *pszPath, int watch, zkproto::zk_stat *stat) = 0;
    virtual int GetData(const char *pszPath, int watch, std::string &sBuff, zkproto::zk_stat *stat) = 0;
    virtual int SetData(const char* pszPath, const std::string& sBuffer, int version, zkproto::zk_stat *stat) = 0;
    virtual int GetChildern(const char* pszPath, int watch, std::vector<std::string>& str) = 0;
    virtual int GetChildern(const char *pszPath, int watch, std::vector<std::string> &str, zkproto::zk_stat* stat) = 0;
    virtual int GetAcl(const char *pszPath, std::vector<zkproto::zk_acl> &acl, zkproto::zk_stat *stat) = 0;
    virtual int SetAcl(const char* pszPath, int version, const std::vector<zkproto::zk_acl>& acl) = 0;
    virtual int Multi(const std::vector<zoo_op_t>& ops, std::vector<zoo_op_result_t>* result) = 0;
    virtual int Sync(const char* pszPath) = 0;
};

}

#endif
