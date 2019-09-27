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

#include "xml_config.h"
#include "shm_config.h"
#include "socket_fd.h"
#include <memory>

constexpr const char* pszUri = "/xml/config/query?key=[";

using namespace xmlconf;

XmlConfig::XmlConfig()
{
}

XmlConfig::~XmlConfig()
{
}

int XmlConfig::Query(const std::string& sAddr, std::vector<std::string>& vQueryKey, std::map<std::string, std::string>& mapData)
{
    std::string sAddrInfo;
    if (sAddr.empty())
        sAddrInfo = SHM_CONF->GetValue("xml.config.server.addr");
    else
        sAddrInfo = sAddr;

    if (sAddrInfo.empty())
        return -1;

    std::string sIp;
    uint16_t wPort = 9008;
    std::size_t dwPos = sAddrInfo.find(":");
    if (dwPos == std::string::npos)
        sIp = sAddrInfo;
    else
    {
        sIp = sAddrInfo.substr(0, dwPos);
        sAddrInfo = sAddrInfo.substr(++ dwPos);
        wPort = atoi(sAddrInfo.c_str());
    }

    std::string sUri = getUri(vQueryKey);
    std::string sJson;
    int iRet = onMsg(sIp, wPort, sUri, sJson);
    if (iRet < 0)
        return -1;

    ConfigResp oResp;
    serialize::CJsonToObjet oJson(sJson);
    if (oJson)
        return -1;

    oJson >> oResp;

    for (auto it = oResp.data.begin(); it != oResp.data.end(); ++ it)
        mapData.insert(std::map<std::string, std::string>::value_type(it->key, it->value));

    return 0;
}

int XmlConfig::onMsg(const std::string& sAddr, uint16_t wPort, const std::string& sUri, std::string& sResp)
{
    znet::CTcpCli oCli;
    oCli.SetSync();
    if (oCli.Create(sAddr.c_str(), wPort, 30000, nullptr) < 0)
        return -1;

    std::string sHttpHeader;
    sHttpHeader = "GET ";
    sHttpHeader.append(sUri).append(" HTTP/1.1\r\n");
    sHttpHeader.append("Host: ").append(sAddr).append(":").append(std::to_string(wPort)).append("\r\n");
    sHttpHeader.append("Connection: keep-alive\r\n");
    sHttpHeader.append("User-Agent: xml-config\r\n");
    sHttpHeader.append("Accept-Language: zh-CN,zh;q=0.8\r\n");
    sHttpHeader.append("Content-Type: application/x-www-form-urlencoded\r\n");
    sHttpHeader.append("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\r\n");


    if (oCli.Write(sHttpHeader.c_str(), sHttpHeader.length()) < 0)
        return -1;

    std::unique_ptr<char> pBuf(new char[9182]);
    int iLen = 8192;
    char* pszBuf = pBuf.get();
    int iOffset = 0;
    while (1)
    {
        int iRet = oCli.Read(pszBuf, iLen);
        if (iRet == -1)
            return -1;

        pszBuf += iRet;
        iLen -= iRet;
        iOffset += iRet;

        if (strstr(pBuf.get(), "\r\n\r\n"))
            break;
    }

    const char* p = strstr(pBuf.get(), "Content-Length: ");
    p += sizeof("Content-Length: ") - 1;
    int iBodyLen = atoi(p);

    p = strstr(p, "\r\n\r\n");
    p += sizeof("\r\n\r\n") - 1;
    sResp.append(p, iOffset - (p - pBuf.get()));

    if (sResp.length() == static_cast<std::size_t>(iBodyLen))
        return 0;

    iBodyLen -= sResp.length();

    iLen = 8192;
    while (iBodyLen > 0)
    {
        int iRet = oCli.Read(pBuf.get(), iLen);
        if (iRet < 0)
            return -1;

        sResp.append(pBuf.get(), iRet);
        iBodyLen -= iRet;
    }
    return 0;
}

std::string XmlConfig::getUri(std::vector<std::string>& vQueryKey)
{
    std::string sUri = pszUri;
    size_t i = 0;
    for (; i < vQueryKey.size(); ++ i)
    {
        if (i == 0)
            sUri.append("\"").append(vQueryKey[i]).append("\"");
        else
            sUri.append(",\"").append(vQueryKey[i]).append("\"");
    }
    sUri.append("]");
    return sUri;
}

XmlConf* XmlConf::CreateObj()
{
    return new XmlConfig();
}
