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
    MySqlResult():m_pRes(nullptr), m_qwRows(0), m_pRowRes(nullptr),m_dwIndex(0),m_pMySql(nullptr), m_dwFields(0){}
    ~MySqlResult()
    {
        do
        {
            if (!m_pRes)
                m_pRes = mysql_store_result(m_pMySql);

            if(m_pRes)
                mysql_free_result(m_pRes);

            m_pRes = nullptr;
        } while (!mysql_next_result(m_pMySql));
    }

public:
    int64_t SetResult(MYSQL_RES* pRes, MYSQL* pMySql)
    {
        m_pRes = pRes;
        m_qwRows = mysql_num_rows(pRes);
        m_dwFields = mysql_num_fields(pRes);
        m_pMySql = pMySql;
        if (m_qwRows > 0)
            m_pRowRes = mysql_fetch_row(pRes);
        return static_cast<int64_t>(m_qwRows);
    }

public:
#define MYSQL_TO_TYPE(type) \
    void operator |= (type& o)  \
    {   \
        if (m_dwIndex >= m_dwFields)    \
            return;                     \
        if (m_pRowRes)  \
        {   \
            if (m_pRowRes[m_dwIndex])   \
                o = static_cast<type>(strtoll(m_pRowRes[m_dwIndex], nullptr, 10));  \
            ++ m_dwIndex;   \
        }   \
    }

    MYSQL_TO_TYPE(int16_t)
    MYSQL_TO_TYPE(uint16_t)
    MYSQL_TO_TYPE(int32_t)
    MYSQL_TO_TYPE(uint32_t)
    MYSQL_TO_TYPE(int64_t)
    MYSQL_TO_TYPE(uint64_t)
#undef MYSQL_TO_TYPE

    void operator |= (std::string& o)
    {
        if (m_dwIndex >= m_dwFields)
            return;

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
        if (NextRes())
            return;
        oObj.Serialize(*this);
    }

#define MYSQL_TO_ARRAY(type)    \
    void operator |= (type& v)  \
    {   \
        if (NextRes())  \
            return;     \
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
        if (NextRes())
            return;
        for (uint64_t i = 0; i < m_qwRows; ++ i) 
        {
            OBJ o;
            *this |= o;
            v.insert(o);
            m_pRowRes = mysql_fetch_row(m_pRes);
            m_dwIndex = 0;
        }
    }

    template<typename K, typename V>
    void operator |= (std::map<K, V>& m)
    {
        if (NextRes())
            return;

        for (uint64_t i = 0; i < m_qwRows; ++ i) 
        {
            K k;
            *this |= k;
            V v;
            *this |= v;
            m.insert(std::make_pair(k, v));
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
    bool NextRes()
    {
        if (m_dwIndex >= m_dwFields)
        {
            if (mysql_next_result(m_pMySql))
                return true;

            if(m_pRes)
                mysql_free_result(m_pRes);

            m_pRes = mysql_store_result(m_pMySql);
            m_dwFields = mysql_num_fields(m_pRes);
            m_qwRows = mysql_num_rows(m_pRes);
            if (m_qwRows > 0)
                m_pRowRes = mysql_fetch_row(m_pRes);

            m_dwIndex = 0;
        }
        return false;
    }

private:
    MYSQL_RES* m_pRes;
    uint64_t m_qwRows;
    MYSQL_ROW m_pRowRes;
    uint32_t m_dwIndex;
    MYSQL* m_pMySql;
    uint32_t m_dwFields;
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

        oRes.GetRes(oObj);
        Clear();
        if (iRet < 0)
            return iRet;

        return iRet;
    }

    template <class T>
    void SetValueI(T v)
    {m_vValue.push_back(std::to_string(v));}

    void SetUint128(__uint128_t& v);
    void SetInt128(__int128_t& v);
    void SetString(const std::string& v);
    void SetString(const char* v);
    const char* GetErr()const 
    {
        if (!m_pCli)
            return "get mysql client ptr null";
        return m_pCli->GetErr();
    }

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
