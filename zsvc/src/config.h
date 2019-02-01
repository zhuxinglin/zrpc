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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <map>
#include <string>

namespace zrpc
{

class CConfig
{
public:
    CConfig();
    ~CConfig();

public:
    typedef std::map<std::string, std::string> config_info;
    typedef std::map<int, config_info > config_type;
    
public:
    int Parse(const char* pszFileName);
    config_info* GetConfig(int iKey);

private:
    char* GetContent(const char* pszFileName);
    int ParseContent(char* pszContent);

private:
    config_type m_oContent;

};

}

#endif
