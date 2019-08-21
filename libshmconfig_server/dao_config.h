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

struct ShmConfigTable
{
    uint64_t id;
    std::string key;
    std::string value;
    uint32_t status;
    uint32_t del_flag;
    uint64_t create_time;
    uint64_t update_time;
    std::string author;
    std::string description;

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, id);
        _SERIALIZE(ar, 0, key);
        _SERIALIZE(ar, 0, value);
        _SERIALIZE(ar, 0, status);
        _SERIALIZE(ar, 0, del_flag);
        _SERIALIZE(ar, 0, create_time);
        _SERIALIZE(ar, 0, update_time);
        _SERIALIZE(ar, 0, author);
        _SERIALIZE(ar, 0, description);
    }
};

struct ConfigResp
{
    int code{0};
    std::string msg;
    std::vector<ShmConfigTable> data;
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

    int Add(const ShmConfigTable* pConf);
    int Mod(ShmConfigTable* pConf);
    int Del(const std::string& sKey, const std::string& sAuthor);
    int Query(const std::string& sKey, uint32_t dwPageNo, uint32_t dwPageSize, std::vector<ShmConfigTable>& vConf);
    int ConfigCount(uint32_t& dwSumCount);

private:
    struct ManageMysqlPool;
    ManageMysqlPool* getMysqlCliObj();
    uint64_t getTimeMs();

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
