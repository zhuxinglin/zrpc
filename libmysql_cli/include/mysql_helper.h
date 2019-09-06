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

#ifndef __MYSQL_HELPER__H__
#define __MYSQL_HELPER__H__

#include <stdint.h>
#include "mysql_cli.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include "serializable.h"

namespace mysqlcli
{

class MySqlResult : public serialize::ISerialize
{
public:
    MySqlResult():m_pRes(nullptr), m_qwRows(0), m_pRowRes(nullptr),m_dwIndex(0){}
    ~MySqlResult(){if(m_pRes)mysql_free_result(m_pRes);}

public:
    void SetResult(MYSQL_RES* pRes, uint64_t qwRows)
    {
        m_pRes = pRes;
        m_qwRows = qwRows;
        if (qwRows > 0)
            m_pRowRes = mysql_fetch_row(pRes);
    }

public:
#define MYSQL_TO_TYPE(type) \
    void operator |= (type& o)  \
    {   \
        if (m_pRowRes)  \
        {   \
            if (m_pRowRes[m_dwIndex])   \
                o = static_cast<type>(strtoll(m_pRowRes[m_dwIndex], nullptr, 10));  \
            ++ m_dwIndex;   \
        }   \
    }

    MYSQL_TO_TYPE(int)
    MYSQL_TO_TYPE(uint32_t)
    MYSQL_TO_TYPE(int64_t)
    MYSQL_TO_TYPE(uint64_t)
#undef MYSQL_TO_TYPE

    void operator |= (std::string& o)
    {
        if (m_pRowRes)
        {
            if (m_pRowRes[m_dwIndex])
                o = m_pRowRes[m_dwIndex];
            ++ m_dwIndex;
        }
    }

    template<typename T>
    void operator |= (T& oObj)
    {
        oObj.Serialize(*this);
    }

#define MYSQL_TO_ARRAY(type)    \
    void operator |= (type& v)  \
    {   \
        for (uint64_t i = 0; i < m_qwRows; ++ i)    \
        {   \
            OBJ o;  \
            *this |= o; \
            v.push_back(o); \
            m_pRowRes = mysql_fetch_row(m_pRes);  \
            m_dwIndex = 0;      \
        }   \
    }

    template<typename OBJ>
    MYSQL_TO_ARRAY(std::list<OBJ>)
    template<typename OBJ>
    MYSQL_TO_ARRAY(std::vector<OBJ>)
#undef MYSQL_TO_ARRAY

    template<typename OBJ>
    void operator |= (std::set<OBJ>& v)
    {
        for (uint64_t i = 0; i < m_qwRows; ++ i) 
        {
            OBJ o;
            *this |= o;
            v.insert(o);
            m_pRowRes = mysql_fetch_row(m_pRes);
            m_dwIndex = 0;
        }
    }

    template<typename T>
    void GetRes(T& oBj)
    {
        *this |= oBj;
    }

private:
    MYSQL_RES* m_pRes;
    uint64_t m_qwRows;
    MYSQL_ROW m_pRowRes;
    uint32_t m_dwIndex;
};

class MySqlHelper
{
public:
    MySqlHelper(MysqlCli* pCli):m_pCli(pCli){};
    ~MySqlHelper(){};

public:
    int64_t Query();
    template <typename T>
    int64_t Query(T& oObj)
    {
        MySqlResult oRes;
        int64_t iRet = m_pCli->Query(m_sSql, &oRes);
        Clear();
        if (iRet < 0)
            return iRet;

        oRes.GetRes(oObj);
        return iRet;
    }

    template <class T>
    void SetValueI(T v)
    {m_vValue.push_back(std::to_string(v));}

    void SetUint128(__uint128_t& v);
    void SetInt128(__int128_t& v);
    void SetString(const std::string& v);
    void SetString(const char* v);
    const char* GetErr()const {return m_pCli->GetErr();}

    int Begin();
    int Commit();
    int Rollback();

    std::string& GenerateSql(std::string& sSql);

    operator bool()
    {
        return m_pCli == nullptr;
    }

private:
    void Clear();

private:
    MysqlCli* m_pCli;
    std::vector<std::string> m_vValue;
    std::string m_sSql;
};

}

#endif
