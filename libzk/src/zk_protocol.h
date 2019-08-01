/*
* zookeeper 协议
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

#ifndef __ZK_PROTOCOL__H__
#define __ZK_PROTOCOL__H__

#include <stdint.h>
#include <arpa/inet.h>
#include <string>
#include <list>

#define WATCHER_EVENT_XID -1 
#define PING_XID -2
#define AUTH_XID -4
#define SET_WATCHES_XID -8

static inline int64_t htonll(int64_t v)
{
    if (htonl(1) == 1)
        return v;

    char* s = reinterpret_cast<char*>(&v);
    for (int i = 0; i < 4; ++ i)
    {
        int tmp = s[i];
        s[i] = s[8 - i - 1];
        s[8 - i - 1] = tmp;
    }
    return v;
}

static inline int64_t ntohll(int64_t v)
{
    if (ntohl(1) == 1)
        return v;

    char* s = reinterpret_cast<char*>(&v);
    for (int i = 0; i < 4; ++ i)
    {
        int tmp = s[8 - i - 1];
        s[8 - i - 1] = s[i];
        s[i] = tmp;
    }
    return v;
}

struct zk_len
{
    uint32_t len;
};

struct zk_auth_packet : public zk_len
{
    int type;
    std::string scheme;
    std::string auth;

    void Hton()
    {
        len = htonl(len);
        type = htonl(type);
    }
};

struct zk_check_version_request : public zk_len
{
    std::string path;
    int version;
};

///********************************************************
struct zk_connect_request : public zk_len
{
    int protocol_version;
    int64_t last_zxid_seen;
    int timeout;
    int64_t session_id;
    char passwd[16];

    void Hton()
    {
        len = htonl(len);
        protocol_version = htonl(protocol_version);
        last_zxid_seen = htonll(last_zxid_seen);
        timeout = htonl(timeout);
        session_id = htonll(session_id);
    }
};

struct zk_connect_response : public zk_len
{
    int protocol_version;
    int timeout;
    long session_id;
    std::string passwd;
};

///********************************************************
struct zk_id
{
    std::string scheme;
    std::string id;
};

struct zk_acl
{
    int perms;
    zk_id id;
};

struct zk_create_request : public zk_len
{
    std::string path;
    std::string data;
    zk_acl acl;
    int flags;
};

struct zk_create_response : public zk_len
{
    std::string path;
};

///********************************************************
struct zk_delete_request : public zk_len
{
    std::string path;
    int version;
};

///********************************************************
struct zk_error_response : public zk_len
{
    int err;
};

///********************************************************
struct zk_stat
{
    long czxid;
    long mzxid;
    long ctime;
    long mtime;
    int version;
    int cversion;
    int aversion;
    long ephemeral_owner;
    int data_length;
    int num_children;
    long pzxid;
};

struct zk_exists_request : public zk_len
{
    std::string path;
    bool watch;
};

struct zk_exists_response : public zk_len
{
    zk_stat stat;
};
///********************************************************
struct zk_get_acl_request : public zk_len
{
    std::string path;
};

struct zk_get_acl_response : public zk_len
{
    zk_acl acl;
    zk_stat stat;
};

///********************************************************
struct zk_get_children2_request : public zk_len
{
    std::string path;
    bool watch;
};

struct zk_get_children2_response : public zk_len
{
    std::list<std::string> children;
    zk_stat stat;
};

///********************************************************
struct zk_get_children_request : public zk_len
{
    std::string path;
    bool watch;
};

struct zk_get_children_response : public zk_len
{
    std::list<std::string> children;
};

///********************************************************
struct zk_get_data_request : public zk_len
{
    std::string path;
    bool watch;
};

struct zk_get_data_response : public zk_len
{
    std::string data;
    zk_stat stat;
};

///********************************************************
struct zk_get_max_children_request : public zk_len
{
    std::string path;
};

struct zk_get_max_children_response : public zk_len
{
    int max;
};
///********************************************************
struct zk_get_sasl_request : public zk_len
{
    std::string token;
};

///********************************************************
struct zk_multi_header : public zk_len
{
    int type;
    bool done;
    int err;
};

///********************************************************
struct zk_reply_header
{
    int xid;
    long zxid;
    int err;
    char data[0];

    void Ntoh()
    {
        xid = ntohl(xid);
        zxid = ntohll(zxid);
        err = ntohl(err);
    }
};
///********************************************************
struct zk_request_header : public zk_len
{
    int zid;
    int type;
};
///********************************************************
struct zk_set_acl_request : public zk_len
{
    std::string path;
    zk_acl acl;
    int version;
};

struct zk_set_acl_response : public zk_len
{
    zk_stat stat;
};

///********************************************************
struct zk_set_data_request : public zk_len
{
    std::string path;
    std::string data;
    int version;
};

struct zk_set_data_response : public zk_len
{
    zk_stat stat;
};
///********************************************************
struct zk_set_max_children_request : public zk_len
{
    std::string path;
    int max;
};

///********************************************************
struct zk_set_sasl_request : public zk_len
{
    std::string token;
};

struct zk_set_sasl_response : public zk_len
{
    std::string token;
};
///********************************************************
struct zk_set_watches : public zk_len
{
    long relative_zxid;
    std::list<std::string> data_watches;
    std::list<std::string> exist_watches;
    std::list<std::string> child_watches;
};
///********************************************************
struct zk_sync_request : public zk_len
{
    std::string path;
};

struct zk_sync_response : public zk_len
{
    std::string path;
};

///********************************************************
struct zk_watcher_event
{
    int type;
    int state;
    char path[0];

    void Ntoh()
    {
        type = ntohl(type);
        state = ntohl(state);
    }
};
#endif /* __ZK_PROTOCOL__H__ */
