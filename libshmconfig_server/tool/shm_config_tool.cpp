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

#include "mysql_helper.h"
#include "socket_fd.h"
#include <stdio.h>
#include <unistd.h>
#include "serializable.h"

void show()
{
    printf("-V version\n-h help\n-H host\n-P port\n-i debug info\n");
    printf("-A add shm config(-A -k -v -s -a -d)\n");
    printf("-D delete shm config(-D -k -a)\n");
    printf("-M modification shm config(-M -k -v -s -a -d[optional])\n");
    printf("-Q query shm config(-Q -k[optional], -n[optional] -c[optional])\n");
    printf("-k shm config key\n");
    printf("-v shm config value\n");
    printf("-s shm config status(0 hand make, 1 edit, 2 publish)\n");
    printf("-a shm config author\n");
    printf("-d shm config description\n");
    printf("-n query page no\n");
    printf("-c query page size\n");
    printf("=====================================================================\n");
    printf("-V 版本\n-h 帮助\n-H 服务地址\n-P 服务端口\n-i 调试信息\n");
    printf("-A 添加内存配置,命令格式如:(-A -k -v -s -a -d)\n");
    printf("-D 删除内存配置,命令格式如:(-D -k -a)\n");
    printf("-M 修改内存配置,命令格式如:(-M -k -v -s -a -d[可选])\n");
    printf("-Q 查询内存配置,命令格式如:(-Q -k[可选], -n[可选] -c[可选])\n");
    printf("-k 内存配置主键\n");
    printf("-v 内存配置值\n");
    printf("-s 内存配置操作状态(0 手搞, 1 编辑, 2 发布)\n");
    printf("-a 内存配置操作人\n");
    printf("-d 内存配置描述信息\n");
    printf("-n 查询的页\n");
    printf("-c 查询页大小\n");
    exit(0);
}

struct ConfigInfo
{
    std::string key;
    std::string value;
    std::string author;
    std::string description;
    uint32_t status{0};

    uint32_t dwPageNo{0};
    uint32_t dwPageSize{0};
    int debug{0};

    template<typename AR>
    void Serialize(AR& ar)
    {
        _SERIALIZE(ar, 0, key);
        _SERIALIZE(ar, 0, value);
        _SERIALIZE(ar, 0, author);
        _SERIALIZE(ar, 0, description);
        _SERIALIZE(ar, 0, status);
    }
};

static void OnMsg(std::string& sAddr, uint16_t wPort, const std::string& sUri, int type, const std::string& sReq, std::string& sResp, int debug)
{
    znet::CTcpCli oCli;
    oCli.SetSync();
    if (oCli.Create(sAddr.c_str(), wPort, 30000, nullptr) < 0)
    {
        printf("%s\n", oCli.GetErr().c_str());
        return ;
    }

    std::string sHttpHeader;
    if (type == 1)
        sHttpHeader = "POST ";
    else
        sHttpHeader = "GET ";
    sHttpHeader.append(sUri).append(" HTTP/1.1\r\n");
    sHttpHeader.append("Host: ").append(sAddr).append(":").append(std::to_string(wPort)).append("\r\n");
    sHttpHeader.append("Connection: keep-alive\r\n");
    sHttpHeader.append("User-Agent: shm-config\r\n");
    sHttpHeader.append("Accept-Language: zh-CN,zh;q=0.8\r\n");
    if (type == 1)
    {
        sHttpHeader.append("Content-Type: application/x-www-form-urlencoded\r\n");
        sHttpHeader.append("Content-Length: ").append(std::to_string(sReq.length())).append("\r\n");
    }
    sHttpHeader.append("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\r\n");

    if (type == 1)
        sHttpHeader.append(sReq);

    if (debug == 1)
        printf("http request:\n%s\n", sHttpHeader.c_str());

    if (oCli.Write(sHttpHeader.c_str(), sHttpHeader.length()) < 0)
        return;

    char szBuf[8192];
    int iLen = 8192;
    char* pszBuf = szBuf;
    int iOffset = 0;
    while (1)
    {
        int iRet = oCli.Read(pszBuf, iLen);
        if (iRet == -1)
            return ;

        pszBuf += iRet;
        iLen -= iRet;
        iOffset += iRet;

        if (strstr(szBuf, "\r\n\r\n"))
            break;
    }

    const char* p = strstr(szBuf, "Content-Length: ");
    p += sizeof("Content-Length: ") - 1;
    int iBodyLen = atoi(p);

    p = strstr(p, "\r\n\r\n");
    p += sizeof("\r\n\r\n") - 1;
    sResp.append(p, iOffset - (p - szBuf));
    if (debug == 1)
        printf("http response:\n%s", szBuf);
    if (sResp.length() == static_cast<std::size_t>(iBodyLen))
        return;

    iBodyLen -= sResp.length();

    iLen = 8192;
    while (iBodyLen > 0)
    {
        int iRet = oCli.Read(szBuf, iLen);
        if (iRet < 0)
            return;

        if (debug == 1)
            printf("%s", szBuf);
        sResp.append(szBuf, iRet);
        iBodyLen -= iRet;
    }
    if (debug == 1)
        printf("\n");
}

static void Add(std::string& sAddr, uint16_t wPort, ConfigInfo* pConf)
{
    if (pConf->key.empty() || pConf->key.length() > 64)
    {
        printf("error : add key error, empty ot length > 64, %s\n", pConf->key.c_str());
        return ;
    }

    if (pConf->value.length() > 4096)
    {
        printf("error : add value length > 4096\n");
        return ;
    }

    if (pConf->author.empty())
    {
        printf("error : add Author empty\n");
        return ;
    }

    std::string sMsg;
    serialize::CJson oJson;
    oJson << *pConf;

    std::string sUri = "/shm/config/add";
    OnMsg(sAddr, wPort, sUri, 1, oJson.GetJson(), sMsg, pConf->debug);
    printf("add reslut:\n%s\n", sMsg.c_str());
}

static void Del(std::string& sAddr, uint16_t wPort, ConfigInfo* pConf)
{
    if (pConf->key.empty() || pConf->key.length() > 64)
    {
        printf("error : add key error, empty ot length > 64, %s\n", pConf->key.c_str());
        return ;
    }

    if (pConf->author.empty())
    {
        printf("error : add Author empty\n");
        return ;
    }

    std::string sUri = "/shm/config/del?key=";
    sUri.append(pConf->key).append("&author=");
    sUri.append(pConf->author);
    std::string sMsg;

    OnMsg(sAddr, wPort, sUri, 0, std::string(), sMsg, pConf->debug);
    printf("delete reslut:\n%s\n", sMsg.c_str());
}

static void Mod(std::string& sAddr, uint16_t wPort, ConfigInfo* pConf)
{
    if (pConf->key.empty() || pConf->key.length() > 64)
    {
        printf("error : add key error, empty ot length > 64, %s\n", pConf->key.c_str());
        return ;
    }

    if (pConf->value.length() > 4096)
    {
        printf("error : add value length > 4096\n");
        return ;
    }

    if (pConf->author.empty())
    {
        printf("error : add Author empty\n");
        return ;
    }

    std::string sMsg;
    serialize::CJson oJson;
    oJson << *pConf;
    std::string sUri = "/shm/config/mod";
    OnMsg(sAddr, wPort, sUri, 1, oJson.GetJson(), sMsg, pConf->debug);
    printf("modification reslut:\n%s\n", sMsg.c_str());
}

static void Query(std::string& sAddr, uint16_t wPort, ConfigInfo* pConf)
{
    std::string sUri = "/shm/config/querylist?";
    if (pConf->key.empty())
    {
        sUri.append("page_no=").append(std::to_string(pConf->dwPageNo));
        sUri.append("&page_size=").append(std::to_string(pConf->dwPageSize));
    }
    else
    {
        sUri.append("key=").append(pConf->key);
    }
    std::string sMsg;

    OnMsg(sAddr, wPort, sUri, 0, std::string(), sMsg, pConf->debug);
    printf("query reslut:\n%s\n", sMsg.c_str());
}

int main(int argc, char * const argv[])
{
    if (argc < 2)
    {
        show();
        return 0;
    }
    int opt;
    int type = 0;
    ConfigInfo oInfo;
    std::string sAddr;
    uint16_t wPort = 0;
    while ((opt = getopt(argc, argv, "VhH:P:k:v:s:a:d:ADMQn:c:i")) != -1)
    {
        switch (opt)
        {
        case 'V':
            printf("version : 1.0.0\n");
            exit(0);
            break;

        case 'h':
            show();
            break;

        case 'A':
            type = 'A';
            break;

        case 'D':
            type = 'D';
            break;

        case 'M':
            type = 'M';
            break;

        case 'Q':
            type = 'Q';
            break;

        case 'k':
            oInfo.key = optarg;
            break;

        case 'v':
            oInfo.value = optarg;
            break;

        case 's':
            oInfo.status = atoi(optarg);
            break;

        case 'a':
            oInfo.author = optarg;
            break;

        case 'd':
            oInfo.description = optarg;
            break;

        case 'n':
            oInfo.dwPageNo = atoi(optarg);
            break;

        case 'c':
            oInfo.dwPageSize = atoi(optarg);
            break;

        case 'H':
            sAddr = optarg;
            break;

        case 'P':
            wPort = atoi(optarg);
            break;

        case 'i':
            oInfo.debug = 1;
            break;

        default:
            show();
            break;
        }
    }

    if (sAddr.empty() || wPort == 0)
    {
        printf("error : host address or potr is empty\n");
        return 0;
    }

    switch (type)
    {
    case 'A':
        Add(sAddr, wPort, &oInfo);
        break;

    case 'D':
        Del(sAddr, wPort, &oInfo);
        break;

    case 'M':
        Mod(sAddr, wPort, &oInfo);
        break;

    case 'Q':
        Query(sAddr, wPort, &oInfo);
        break;

    default:
        printf("error : -h help");
        break;
    }
    return 0;
}
