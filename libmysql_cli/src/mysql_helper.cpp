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


#include "../include/mysql_helper.h"
#include <stdlib.h>
#include <string.h>

using namespace mysqlcli;

void itoa128(__int128_t v, char* s)
{
    if (!s)
        return;

    char temp;
    int i = 0;

    while(v)
    {
        s[i++] = v % 10 + '0';
        v /= 10;
    }

    if (v < 0)
        ++ i;

    s[i --] = '\0';

    int j = 0;
    while(j < i)
    {
        temp = s[j];
        s[j] = s[i];
        s[i] = temp;
        j++;
        i--;
    }

    if (v < 0)
        s[0] = '-';
}

void uitoa128(__uint128_t v, char* s)
{
    if (!s)
        return;

    char temp;
    int i = 0;

    while(v)
    {
        s[i++] = v % 10 + '0';
        v /= 10;
    }
    s[i --] = '\0';

    int j = 0;
    while(j < i)
    {
        temp = s[j];
        s[j] = s[i];
        s[i] = temp;
        j++;
        i--;
    }
}

void MySqlHelper::Clear()
{
    m_sSql = "";
    m_vValue.clear();
}

void MySqlHelper::SetUint128(__uint128_t& v)
{
    char s[40];
    itoa128(v, s);
    SetString(s);
}

void MySqlHelper::SetInt128(__int128_t& v)
{
    char s[40];
    uitoa128(v, s);
    SetString(s);
}

void MySqlHelper::SetString(const std::string& v)
{
    if (!m_pCli)
        return;

    m_vValue.push_back(m_pCli->GetEscapeString(v.c_str(), v.length()));
}

void MySqlHelper::SetString(const char* v)
{
    if (!m_pCli)
        return;

    m_vValue.push_back(m_pCli->GetEscapeString(v, strlen(v)));
}

int64_t MySqlHelper::Query()
{
    MySqlResult oRes;
    int64_t iRet = m_pCli->Query(m_sSql, &oRes);
    Clear();
    return iRet;
}

int MySqlHelper::BeginCommit()
{
    return m_pCli->BeginCommit();
}

int MySqlHelper::EndCommit()
{
    return m_pCli->EndCommit();
}

int MySqlHelper::Commit()
{
    return m_pCli->Commit();
}

int MySqlHelper::Rollback()
{
    return m_pCli->Rollback();
}

std::string& MySqlHelper::GenerateSql(std::string& sSql)
{
    if (m_vValue.empty())
    {
        m_sSql = sSql;
        return sSql;
    }
    const char* s = sSql.c_str();
    auto it = m_vValue.begin();
    const char* e = s;
    size_t l = sSql.length();
    while (l > 0)
    {
        if (*e == '?')
        {
            m_sSql.append(s, e - s);
            s = e + 1;
            if (it != m_vValue.end())
                m_sSql.append(*it);
            ++ it;
        }
        ++ e;
        -- l;
    }
    m_sSql.append(s, e - s);
    return m_sSql;
}
