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
*
* 
 */

#include <stdio.h>
#include <memory>
#include "config.h"
#include <stack>

using namespace zrpc;

CConfig::CConfig()
{
}

CConfig::~CConfig()
{
}

int CConfig::Parse(const char *pszFileName)
{
    char* pszContent = GetContent(pszFileName);
    if (!pszContent)
        return -1;

    std::unique_ptr<char> oPtr(pszContent);

    return ParseContent(pszContent);
}

char *CConfig::GetContent(const char *pszFileName)
{
    FILE *pFile = fopen(pszFileName, "rb");
    if (!pFile)
    {
        printf("open config file '%s' fail\n", pszFileName);
        return 0;
    }
    fseek(pFile, 0L, SEEK_END);
    char *pszBuf;
    int iLen = ftell(pFile);
    fseek(pFile, 0L, SEEK_SET);
    pszBuf = new char[iLen + 1];
    fread(pszBuf, 1, iLen, pFile);
    fclose(pFile);
    return pszBuf;
}

int CConfig::ParseContent(char *pszContent)
{
    char* s = pszContent;
    int iRow = 1;
    std::stack<char> oFun;
    char* c = 0;
    std::string sFun;
    std::string sKey;
    std::string sValue;
    int iIndex = 1;
    config_info mapList;
    while (*s)
    {
        switch (*s)
        {
        case '/':
            c = s;
            ++ s;
            if (*s == '/')
            {
                c = 0;
                while('\r' != *s && '\n' != *s && *s != 0)
                    ++s;
                ++ iRow;
            }
        break;

        case ' ':
        case '\t':
            if (c && !oFun.empty())
            {
                if (!sKey.empty() && !sValue.empty())
                {
                    printf("config fail %d fail\n", iRow);
                    return -1;
                }

                if (sKey.empty())
                {
                    sKey.append(c, s - c);
                    c = 0;
                }
            }
            while(*s == ' ' || *s == '\t')
                ++s;
        break;

        case '{':
            if (!c || !oFun.empty() || !sFun.empty())
            {
                printf("config file %d front not function\n", iRow);
                return -1;
            }
            oFun.push(*s);
            sFun.append(c, s - c);
            c = 0;
            ++ s;
        break;

        case '}':
            if (oFun.empty())
            {
                printf("config file %d front not '{'\n", iRow);
                return -1;
            }
            oFun.pop();
            if (sFun.compare("global") == 0)
                m_oContent.insert(config_type::value_type(0, mapList));
            else if (sFun.compare("server") == 0)
                m_oContent.insert(config_type::value_type(iIndex ++ , mapList));
            else
            {
                printf("config file faid, error function '%s'\n", sFun.c_str());
                return -1;
            }
            sFun = "";
            mapList.clear();
            ++ s;
        break;

        case '\n':
        case '\r':
            if (!sKey.empty() || !sValue.empty())
            {
                printf("config file %d front not ';'\n", iRow);
                return -1;
            }
            ++ s;
            if ('\r' == *s)
                ++ s;
            ++ iRow;
        break;

        case ';':
            if (sKey.empty())
            {
                printf("config fail %d fail not key\n", iRow);
                return -1;
            }

            if (c)
            {
                sValue.append(c, s - c);
                c = 0;
            }
            ++ s;

            mapList.insert(config_info::value_type(sKey, sValue));
            sKey = "";
            sValue = "";
        break;

        default:
            if (!c)
                c = s;
            while (*s > 0x20 && *s <= 0x7F && *s != '{' && *s != ';' && *s != '}')
                ++s;
        break;
        }
    }

    if (!oFun.empty())
    {
        printf("config fail not '}'\n");
        return -1;
    }
    return 0;
}

CConfig::config_info *CConfig::GetConfig(int iKey)
{
    config_type::iterator it;
    it = m_oContent.find(iKey);
    if (it != m_oContent.end())
        return &(it->second);
    return 0;
}
