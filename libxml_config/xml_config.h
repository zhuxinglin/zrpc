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

#ifndef __XML_CONFIG_H__
#define __XML_CONFIG_H__

#include "include/xml_conf.h"
#include "socket_fd.h"
#include "serializable.h"

namespace xmlconf
{

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

struct ConfigResp
{
    std::vector<XmlConfigResp> data;

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, data);
    }
};

class XmlConfig : public XmlConf
{
public:
    XmlConfig();
    ~XmlConfig();

private:
    virtual int Query(const std::string& sAddr, std::vector<std::string>& vQueryKey, std::map<std::string, std::string>& mapData, bool bIsSync = false);
    int onMsg(const std::string& sAddr, uint16_t wPort, const std::string& sUri, std::string& sResp, bool bIsSync);
    std::string getUri(std::vector<std::string>& vQueryKey);
};

}

#endif
