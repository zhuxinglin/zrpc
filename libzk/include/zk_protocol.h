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

static inline void set_int(std::string& d, int v)
{
    v = htonl(v);
    d.append(reinterpret_cast<char*>(&v), sizeof(v));
}

struct zk_response_info
{
    int len;
    char data[0];
};

static inline char* get_string(std::string& d, char* m, int& lm)
{
    zk_response_info* re = reinterpret_cast<zk_response_info*>(m);
    re->len = ntohl(re->len);

    if (re->len > lm)
        return nullptr;

    if (re->len < 0)
        return re->data;

    d.append(re->data, re->len);
    lm -= (re->len + sizeof(re->len));
    return (re->data + re->len);
}

static inline char* get_bytes(std::string& d, char* m, int& lm)
{
    return get_string(d, m, lm);
}

static inline char* get_int(char* m, int& len, int& res)
{
    if (len < (int)sizeof(int))
        return nullptr;

    res = ntohl(*reinterpret_cast<int*>(m));
    m += sizeof(int);
    len -= sizeof(int);
    return m;
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
        set_int(d, version);
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

    int Ntoh(char*& m, int& len)
    {
        m = get_string(scheme, m, len);
        if (!m)
            return -1;
        m = get_string(id, m, len);
        if (!m)
            return -1;
        return 0;
    }
};

struct zk_acl
{
    int perms;
    zk_id id;

    void Hton(std::string& s)
    {
        set_int(s, perms);
        id.Hton(s);
    }

    char* Ntoh(char* m, int& len)
    {
        if (!get_int(m, len, perms))
            return nullptr;

        if (id.Ntoh(m, len) < 0)
            return nullptr;

        return m;
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

        set_int(d, acl.size());
        for (auto it = acl.begin(); it != acl.end(); ++ it)
            it->Hton(d);

        set_int(d, flags);
    }
};

struct zk_create_response
{
    std::string path;

    char* Ntoh(char* msg, int& len)
    {
        return get_string(path, msg, len);
    }
};

///********************************************************
struct zk_delete_request
{
    std::string path;
    int version;

    void Hton(std::string d)
    {
        set_string(d, path);
        set_int(d, version);
    }
};

///********************************************************
struct zk_error_response
{
    int err;
    char data[0];

    void Ntoh()
    {
        err = ntohl(err);
    }
};

///********************************************************
struct zk_exists_request
{
    std::string path;
    int32_t watch;

    void Hton(std::string d)
    {
        set_string(d, path);
        set_int(d, watch);
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

    char* Ntoh(zk_stat* s, int& len)
    {
        czxid = ntohll(s->czxid);
        mzxid = ntohll(s->mzxid);
        ctime = ntohll(s->ctime);
        mtime = ntohll(s->mtime);
        version = ntohl(s->version);
        cversion = ntohl(s->cversion);
        aversion = ntohl(s->aversion);
        ephemeral_owner = ntohll(s->ephemeral_owner);
        data_length = ntohl(s->data_length);
        num_children = ntohl(s->num_children);
        pzxid = ntohll(s->pzxid);

        ++ s;
        len -= sizeof(*s);
        return reinterpret_cast<char*>(s);
    }
};

#pragma pack(4)
struct zk_exists_response
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

struct zk_get_acl_response
{
    zk_get_acl_response(std::vector<zk_acl>& a, zk_stat& s) : acl(a), stat(s){}
    std::vector<zk_acl>& acl;
    zk_stat& stat;

    char* Ntoh(char* msg, int& len)
    {
        int count;
        if (!get_int(msg, len, count))
        {
            for (int i = 0; i < count; ++ i)
            {
                zk_acl a;
                msg = a.Ntoh(msg, len);
                if (!msg)
                    return msg;
                acl.push_back(a);
            }
        }
        if (len != sizeof(stat))
            return nullptr;

        return stat.Ntoh(reinterpret_cast<zk_stat*>(msg), len);
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
        set_int(d, acl.size());
        for (auto it = acl.begin(); it != acl.end(); ++ it)
            it->Hton(d);
        set_int(d, version);
    }
};

struct zk_set_acl_response
{
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
        set_int(d, watch);
    }
};

struct zk_get_children_response
{
    zk_get_children_response(std::vector<std::string>& c):children(c){}
    std::vector<std::string>& children;

    char* Ntoh(char* msg, int& len)
    {
        int count;
        if (!get_int(msg, len, count))
        {
            for (int i = 0; i < count; ++ i)
            {
                std::string d;
                msg = get_string(d, msg, len);
                if (!msg)
                    break;
                children.push_back(d);
            }
        }
        return msg;
    }
};

struct zk_get_children2_response
{
    zk_get_children2_response(std::vector<std::string>& c, zk_stat& s):children(c),stat(s){}
    std::vector<std::string>& children;
    zk_stat& stat;

    char* Ntoh(char* msg, int& len)
    {
        int count;
        if (!get_int(msg, len, count))
        {
            for (int i = 0; i < count; ++ i)
            {
                std::string d;
                msg = get_string(d, msg, len);
                if (!msg)
                    return msg;
                children.push_back(d);
            }
        }

        if (len != sizeof(stat))
            return nullptr;
        return stat.Ntoh(reinterpret_cast<zk_stat*>(msg), len);
    }
};

///********************************************************
struct zk_get_data_request
{
    std::string path;
    int32_t watch;

    void Hton(std::string& d)
    {
        set_string(d, path);
        set_int(d, watch);
    }
};

struct zk_get_data_response
{
    zk_get_data_response(std::string& d, zk_stat& s):data(d), stat(s){}
    std::string& data;
    zk_stat& stat;

    char* Ntoh(char* msg, int& len)
    {
        msg = get_bytes(data, msg, len);
        if (msg == nullptr)
            return msg;

        if (len != sizeof(stat))
            return nullptr;

        return stat.Ntoh(reinterpret_cast<zk_stat*>(msg), len);
    }
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
        set_int(d, version);
    }
};

struct zk_set_data_response
{
    zk_set_data_response(zk_stat& s):stat(s){}
    zk_stat& stat;

    char* Ntoh(char* msg, int& len)
    {
        return stat.Ntoh(reinterpret_cast<zk_stat*>(msg), len);
    }
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
    char data[0];

    void Ntoh()
    {
        type = ntohl(type);
        done = ntohl(done);
        err = ntohl(err);
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
        relative_zxid = htonll(relative_zxid);
        s.append(reinterpret_cast<char *>(&relative_zxid), sizeof(relative_zxid));

        set_int(s, data_watches.size());
        for (auto it = data_watches.begin(); it != data_watches.end(); ++it)
            set_string(s, *it);

        set_int(s, exist_watches.size());
        for (auto it = exist_watches.begin(); it != exist_watches.end(); ++it)
            set_string(s, *it);

        set_int(s, child_watches.size());
        for (auto it = child_watches.begin(); it != child_watches.end(); ++it)
            set_string(s, *it);
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
