/*
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
*
*/

#ifndef __DAO_CONFIG__H__
#define __DAO_CONFIG__H__

#include "mysql_helper.h"
#include <atomic>

constexpr int DB_CONNECT_COUNT    = 2;

namespace dao
{

struct XmlConfigTable
{
    uint64_t id;
    std::string key1;
    std::string key2;
    std::string key3;
    std::string key4;
    std::string value;
    uint32_t status{0};
    uint32_t del_flag{0};
    uint64_t create_time{0};
    uint64_t update_time{0};
    std::string author;
    std::string description;

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, id);
        _SERIALIZE(ar, 0, key1);
        _SERIALIZE(ar, 0, key2);
        _SERIALIZE(ar, 0, key3);
        _SERIALIZE(ar, 0, key4);
        _SERIALIZE(ar, 0, value);
        _SERIALIZE(ar, 0, status);
        _SERIALIZE(ar, 0, del_flag);
        _SERIALIZE(ar, 0, create_time);
        _SERIALIZE(ar, 0, update_time);
        _SERIALIZE(ar, 0, author);
        _SERIALIZE(ar, 0, description);
    }
};

struct XmlConfigInitInfo
{
    std::string key1;
    std::string key2;
    std::string key3;
    std::string key4;
    std::string value;

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, key1);
        _SERIALIZE(ar, 0, key2);
        _SERIALIZE(ar, 0, key3);
        _SERIALIZE(ar, 0, key4);
        _SERIALIZE(ar, 0, value);
    }
};

struct XmlConfigResp
{
    std::string key;
    std::string value;
    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, key);
        _SERIALIZE(ar, 0, value);
    }
};

template<typename T>
struct ConfigResp
{
    int code{0};
    std::string msg;
    std::vector<T> data;
    struct Page
    {
        uint32_t page_no{0};
        uint32_t page_size{0};
        uint32_t total_num{0};
        uint32_t total_page{0};

        template<typename AR>
        void Serialize(AR& ar)
        {
            _SERIALIZE(ar, 0, page_no);
            _SERIALIZE(ar, 0, page_size);
            _SERIALIZE(ar, 0, total_num);
            _SERIALIZE(ar, 0, total_page);
        }
    };
    Page page;

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, code);
        _SERIALIZE(ar, 0, msg);
        _SERIALIZE(ar, 0, page);
        _SERIALIZE(ar, 0, data);
    }
};

class DaoConfig
{
public:
    DaoConfig();
    ~DaoConfig();

public:
    int Init(const char* pszDbAddr);

    int Add(const XmlConfigTable* pConf);
    int Mod(XmlConfigTable* pConf);
    int Del(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, const std::string& sAuthor);
    int Query(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, uint32_t dwPageNo, uint32_t dwPageSize, std::vector<XmlConfigTable>& vConf);
    int Query(const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, std::vector<XmlConfigInitInfo>& vConf);
    int ConfigCount(uint32_t& dwSumCount, const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4);

private:
    struct ManageMysqlPool;
    ManageMysqlPool* getMysqlCliObj();
    uint64_t getTimeMs();
    void setKey(std::string& sSql, const std::string& sKey1, const std::string& sKey2, const std::string& sKey3, const std::string& sKey4, mysqlcli::MySqlHelper& oHelper);

private:
    struct ManageMysqlPool
    {
        mysqlcli::MysqlCli* pSqlCli{nullptr};
        std::atomic_char16_t wRef{0};
    };

    ManageMysqlPool m_oMysql[DB_CONNECT_COUNT];
};


}

#endif
