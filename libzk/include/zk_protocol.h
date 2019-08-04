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
#include <vector>

#define WATCHER_EVENT_XID -1 
#define PING_XID -2
#define AUTH_XID -4
#define SET_WATCHES_XID -8

namespace zkproto
{

static inline int64_t htonll(int64_t v)
{
    if (htonl(1) == 1)
        return v;

    char *s = reinterpret_cast<char *>(&v);
    for (int i = 0; i < 4; ++i)
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

    char *s = reinterpret_cast<char *>(&v);
    for (int i = 0; i < 4; ++i)
    {
        int tmp = s[8 - i - 1];
        s[8 - i - 1] = s[i];
        s[i] = tmp;
    }
    return v;
}

static inline void set_string(std::string &d, const std::string &s)
{
    int len = -1;
    if (s.empty())
    {
        len = htonl(len);
        d.append(reinterpret_cast<char *>(&len), sizeof(len));
    }
    else
    {
        len = htonl(s.length());
        d.append(reinterpret_cast<char *>(&len), sizeof(len));
        d.append(s);
    }
}

static inline void set_bytes(std::string &d, const std::string &s)
{
    set_string(d, s);
}

struct zk_len
{
    uint32_t len;
};

struct zk_auth_packet
{
    int type;
    std::string scheme;
    std::string auth;

    void Hton(std::string &s)
    {
        type = htonl(type);
        s.append(reinterpret_cast<char *>(&type), sizeof(type));
        set_string(s, scheme);
        set_bytes(s, auth);
    }
};

struct zk_check_version_request
{
    std::string path;
    int version;

    void Hton(std::string& d)
    {
        set_string(d, path);
        version = htonl(version);
        d.append(reinterpret_cast<char *>(&version), sizeof(version));
    }
};

///********************************************************
#pragma pack(4)
struct zk_connect_request : public zk_len
{
    int protocol_version;
    int64_t last_zxid_seen;
    int timeout;
    int64_t session_id;
    int32_t passwd_len;
    char passwd[16];

    void Hton()
    {
        len = htonl(sizeof(zk_connect_request) - sizeof(zk_len));
        protocol_version = htonl(protocol_version);
        last_zxid_seen = htonll(last_zxid_seen);
        timeout = htonl(timeout);
        session_id = htonll(session_id);
        passwd_len = htonl(passwd_len);
    }
};

struct zk_connect_response
{
    int protocol_version;
    int timeout;
    int64_t session_id;
    int32_t passwd_len;
    char passwd[16];

    void Ntoh()
    {
        protocol_version = ntohl(protocol_version);
        timeout = ntohl(timeout);
        session_id = ntohll(session_id);
        passwd_len = ntohl(passwd_len);
    }
};
#pragma pack()

///********************************************************
struct zk_id
{
    std::string scheme;
    std::string id;

    void Hton(std::string& s)
    {
        set_string(s, scheme);
        set_string(s, id);
    }
};

struct zk_acl
{
    int perms;
    zk_id id;

    void Hton(std::string& s)
    {
        perms = htonl(perms);
        s.append(reinterpret_cast<char*>(&perms), sizeof(perms));
        id.Hton(s);
    }
};

struct zk_create_request
{
    std::string path;
    std::string data;
    std::vector<zk_acl> acl;
    int flags;

    void Hton(std::string& d)
    {
        set_string(d, path);
        set_bytes(d, data);
        for (auto it = acl.begin(); it != acl.end(); ++ it)
            it->Hton(d);

        flags = htonl(flags);
        d.append(reinterpret_cast<char*>(&flags), sizeof(flags));
    }
};

struct zk_create_response : public zk_len
{
    std::string path;
};

///********************************************************
struct zk_delete_request
{
    std::string path;
    int version;

    void Hton(std::string d)
    {
        set_string(d, path);
        version = htonl(version);
        d.append(reinterpret_cast<char*>(&version), sizeof(version));
    }
};

///********************************************************
struct zk_error_response : public zk_len
{
    int err;
};

///********************************************************
struct zk_exists_request
{
    std::string path;
    int32_t watch;

    void Hton(std::string d)
    {
        set_string(d, path);
        watch = htonl(watch);
        d.append(reinterpret_cast<char*>(&watch), sizeof(watch));
    }
};

struct zk_stat
{
    int64_t czxid;
    int64_t mzxid;
    int64_t ctime;
    int64_t mtime;
    int version;
    int cversion;
    int aversion;
    int64_t ephemeral_owner;
    int data_length;
    int num_children;
    int64_t pzxid;
};

#pragma pack(4)
struct zk_exists_response : public zk_len
{
    zk_stat stat;
};
#pragma pack()
///********************************************************
struct zk_get_acl_request
{
    std::string path;
    void Hton(std::string& d)
    {
        set_string(d, path);
    }
};

struct zk_get_acl_response : public zk_len
{
    zk_acl acl;
    zk_stat stat;
};

///********************************************************
struct zk_get_children_request
{
    std::string path;
    int32_t watch;

    void Hton(std::string& d)
    {
        set_string(d, path);
        watch = htonl(watch);
        d.append(reinterpret_cast<char*>(&watch), sizeof(watch));
    }
};

struct zk_get_children_response : public zk_len
{
    std::list<std::string> children;
};

struct zk_get_children2_response : public zk_len
{
    std::list<std::string> children;
    zk_stat stat;
};

///********************************************************
struct zk_get_data_request
{
    std::string path;
    int32_t watch;

    void Hton(std::string& d)
    {
        set_string(d, path);
        watch = htonl(watch);
        d.append(reinterpret_cast<char*>(&watch), sizeof(watch));
    }
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
struct zk_multi_header
{
    int32_t type;
    int32_t done;
    int32_t err;

    void Hton()
    {
        type = htonl(type);
        done = htonl(done);
        err = htonl(err);
    }
};

///********************************************************
struct zk_reply_header
{
    int32_t xid;
    int64_t zxid;
    int32_t err;
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
    int xid;
    int type;

    zk_request_header(int x, int t) : xid(x), type(t)
    {
        len = sizeof(zk_request_header) - sizeof(zk_len);
    }

    void Hton()
    {
        len = htonl(len);
        xid = htonl(xid);
        type = htonl(type);
    }
};
///********************************************************
struct zk_set_acl_request
{
    std::string path;
    std::vector<zk_acl> acl;
    int version;

    void Hton(std::string& d)
    {
        set_string(d, path);
        for (auto it = acl.begin(); it != acl.end(); ++ it)
            it->Hton(d);
        version = htonl(version);
        d.append(reinterpret_cast<char*>(&version), sizeof(version));
    }
};

struct zk_set_acl_response : public zk_len
{
    zk_stat stat;
};

///********************************************************
struct zk_set_data_request
{
    std::string path;
    std::string data;
    int version;

    void Hton(std::string& d)
    {
        set_string(d, path);
        set_bytes(d, data);
        version = htonl(version);
        d.append(reinterpret_cast<char*>(&version), sizeof(version));
    }
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
struct zk_set_watches
{
    int64_t relative_zxid;
    std::list<std::string> data_watches;
    std::list<std::string> exist_watches;
    std::list<std::string> child_watches;

    void Hton(std::string &s)
    {
        int i = htonl(-1);
        relative_zxid = htonll(relative_zxid);
        s.append(reinterpret_cast<char *>(&relative_zxid), sizeof(relative_zxid));
        if (data_watches.empty())
            s.append(reinterpret_cast<char *>(i), sizeof(i));
        else
        {
            for (auto it = data_watches.begin(); it != data_watches.end(); ++it)
            {
                size_t k = htonl((*it).length());
                s.append(reinterpret_cast<char *>(k), sizeof(k));
                s.append(*it);
            }
        }

        if (exist_watches.empty())
            s.append(reinterpret_cast<char *>(i), sizeof(i));
        else
        {
            for (auto it = exist_watches.begin(); it != exist_watches.end(); ++it)
            {
                size_t k = htonl((*it).length());
                s.append(reinterpret_cast<char *>(k), sizeof(k));
                s.append(*it);
            }
        }

        if (child_watches.empty())
            s.append(reinterpret_cast<char *>(i), sizeof(i));
        else
        {
            for (auto it = child_watches.begin(); it != child_watches.end(); ++it)
            {
                size_t k = htonl((*it).length());
                s.append(reinterpret_cast<char *>(k), sizeof(k));
                s.append(*it);
            }
        }
    }
};
///********************************************************
struct zk_sync_request
{
    std::string path;

    void Hton(std::string& d)
    {
        set_string(d, path);
    }
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

}


#endif /* __ZK_PROTOCOL__H__ */
