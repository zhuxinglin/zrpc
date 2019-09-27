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

#ifndef __XML_CONF_H__
#define __XML_CONF_H__

#include <map>
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
namespace xmlconf
{

class XmlConf
{
public:
    static XmlConf* CreateObj();
    virtual int Query(const std::string& sAddr, std::vector<std::string>& vQueryKey, std::map<std::string, std::string>& mapData, bool bIsSync) = 0;
};

class XmlConfHelper
{
public:
    XmlConfHelper(std::string sAddr = "", bool bIsSync = false):m_sAddr(sAddr), m_bIsSync(bIsSync)
    {}

public:
    void SetKey(const std::string sKey)
    {m_vQueryKey.push_back(sKey);}

    int Query()
    {
        XmlConf* pXml = XmlConf::CreateObj();
        if (!pXml)
            return -1;

        int iRet = pXml->Query(m_sAddr, m_vQueryKey, m_mapData, m_bIsSync);
        delete pXml;
        if (iRet < 0)
            return -1;
        return 0;
    }

    template<typename T>
    T GetValueI(const char* pszKey, T v = 0)
    {
        auto it = m_mapData.find(pszKey);
        if (it == m_mapData.end())
            return v;

        v = static_cast<T>(strtoll(it->second.c_str(), nullptr, 10));
        return v;
    }

    float GetValueF(const char* pszKey, float v = 0.0f)
    {
        auto it = m_mapData.find(pszKey);
        if (it == m_mapData.end())
            return v;
        v = static_cast<float>(strtod(it->second.c_str(), nullptr));
        return v;
    }

    double GetValueF(const char* pszKey, double v = 0.0f)
    {
        auto it = m_mapData.find(pszKey);
        if (it == m_mapData.end())
            return v;

        v = strtod(it->second.c_str(), nullptr);
        return v;
    }

    std::string GetValue(const char* pszKey, const char* v = "")
    {
        auto it = m_mapData.find(pszKey);
        if (it == m_mapData.end())
            return v;
        return it->second;
    }

private:
    std::string m_sAddr;
    bool m_bIsSync;
    std::vector<std::string> m_vQueryKey;
    std::map<std::string, std::string> m_mapData;
};

}
#pragma GCC diagnostic pop

#endif
