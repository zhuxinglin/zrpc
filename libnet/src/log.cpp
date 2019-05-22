//////////////////////////////////////////////////////////////////////////
// file name : log.cpp
// author : 
// create time : 
// 
// 
// 
// 
// 
//////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "log.h"

using namespace znet;

CLog *CLog::m_pSelf = 0;

CLog::CLog()
{
	m_sLogDir = "../log/";
	m_sAddr = "";
	m_dwWriteMode = CLogConfig::LOG_FILE;
	m_dwLevel = CLogConfig::LOG_INFO_LEVEL;
	m_iLogMaxSize = 20 * 1024 * 1024;
	m_pFile = 0;
	m_pSock = 0;
	m_dwSync = 0;
	m_iCurLogSize = 0;
	m_dwVer = 4;
	m_bIsInit = false;
	GetProcName();
}

CLog::~CLog()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	if (m_pSock)
	{
		delete m_pSock;
		m_pSock = 0;
	}
}

CLog *CLog::GetObj()
{
	if (!m_pSelf)
		m_pSelf = new CLog;
	return m_pSelf;
}

void CLog::SetObj(CLog *pLog)
{
	m_pSelf = pLog;
}

void CLog::DelObj()
{
	if (m_pSelf)
		delete m_pSelf;
	m_pSelf = 0;
}

bool CLog::Create(CLogConfig *pConfig)
{
	if (m_bIsInit)
		return true;

	if (pConfig->dwWriteMode <= 0x1F)
		m_dwWriteMode = pConfig->dwWriteMode;

	if (pConfig->dwLevel <= CLogConfig::LOG_ALL_LEVEL)
		m_dwLevel = pConfig->dwLevel;

	if (pConfig->iLogMaxSize >= 10 * 1024 * 1024)
		m_iLogMaxSize = pConfig->iLogMaxSize;

	if (pConfig->m_iVer == 6)
		m_dwVer = pConfig->m_iVer;

	if (!pConfig->sLogDir.empty())
	{
		m_sLogDir = pConfig->sLogDir;
		const char* s = m_sLogDir.c_str() + m_sLogDir.size();
		-- s;
		if (*s != '/')
			m_sLogDir.append("/");
	}

	if (m_dwWriteMode & CLogConfig::LOG_FILE)
	{
		if (!OpenLogFile(false, true))
			return false;
	}
	if (m_dwWriteMode & CLogConfig::LOG_TCP)
	{
		if (OpenNet(pConfig->sNetAddr.c_str(), CLogConfig::LOG_TCP, true) < 0)
			return false;
	}
	else if (m_dwWriteMode & CLogConfig::LOG_UINX)
	{
		if (OpenNet(pConfig->sNetAddr.c_str(), CLogConfig::LOG_UINX, true) < 0)
			return false;
	}
	else if (m_dwWriteMode & CLogConfig::LOG_UDP)
	{
		if (OpenNet(pConfig->sNetAddr.c_str(), CLogConfig::LOG_UDP, true) < 0)
			return false;
	}
	Start(0, 45);
	m_bIsInit = true;
	return true;
}

int CLog::Write(CLogMessage<> *pMessage, uint32_t dwCurLevel)
{
	if (m_dwWriteMode & CLogConfig::LOG_DEF)
	{
		switch (dwCurLevel)
		{
		case CLogConfig::LOG_FATAL:
			// 粉红
			fprintf(stdout, "\033[35m%s\033[m\n", pMessage->oStream.str().c_str());
		break;

		case CLogConfig::LOG_ERROR:
			// 红色打印
			fprintf(stdout, "\033[31m%s\033[m\n", pMessage->oStream.str().c_str());
		break;

		case CLogConfig::LOG_WARN:
			// 蓝色
			fprintf(stdout, "\033[34m%s\033[m\n", pMessage->oStream.str().c_str());
		break;

		case CLogConfig::LOG_INFO:
			// 绿色
			fprintf(stdout, "\033[32m%s\033[m\n", pMessage->oStream.str().c_str());
		break;

		case CLogConfig::LOG_DEBUG:
			// 青色
			fprintf(stdout, "\033[36m%s\033[m\n", pMessage->oStream.str().c_str());
		break;
		}
	}

	if (m_dwWriteMode == CLogConfig::LOG_DEF)
		delete pMessage;
	else
	{
		{
			CSpinLock oLock(&m_dwSync);
			m_oMsgQueue.push(pMessage);
		}
		m_oSem.Post();
	}

	return 0;
}

void CLog::Run(uint32_t)
{
	while(m_bExit)
	{
		m_oSem.Wait();
		while (!m_oMsgQueue.empty())
		{
			CLogMessage<> *pMessage = m_oMsgQueue.front();
			do
			{
				CSpinLock oLock(&m_dwSync);
				m_oMsgQueue.pop();
			}while(0);

			pMessage->oStream << "\n";
			if (m_dwWriteMode & CLogConfig::LOG_FILE)
			{
				if (m_pFile)
				{
					fwrite(pMessage->oStream.str().c_str(), 1, pMessage->oStream.str().size(), m_pFile);
					fflush(m_pFile);
					m_iCurLogSize += pMessage->oStream.str().size();
				}

				if (m_iCurLogSize >= m_iLogMaxSize || !m_pFile)
					OpenLogFile(true);
			}

			if (m_dwWriteMode == CLogConfig::LOG_FILE)
			{
				delete pMessage;
				continue;
			}

			if (m_dwWriteMode & CLogConfig::LOG_UDP)
			{
				CUnreliableFd *pSock = (CUnreliableFd *)m_pSock;
				if (pSock)
				{
					int iLen = pSock->Write(pMessage->oStream.str().c_str(), pMessage->oStream.str().size(), (sockaddr*)m_sUdpAddr.c_str(), m_sUdpAddr.size());
					if (iLen < 0)
						OpenNet(m_sAddr.c_str(), CLogConfig::LOG_UDP);
				}
			}
			else if (m_dwWriteMode & CLogConfig::LOG_TCP || m_dwWriteMode & CLogConfig::LOG_UINX)
			{
				CReliableFd *pSock = (CReliableFd *)m_pSock;
				if (pSock)
				{
					int iLen = pSock->Write(pMessage->oStream.str().c_str(), pMessage->oStream.str().size());
					if (iLen < 0)
					{
						if (m_dwWriteMode & CLogConfig::LOG_TCP)
							OpenNet(m_sAddr.c_str(), CLogConfig::LOG_TCP);
						else
							OpenNet(m_sAddr.c_str(), CLogConfig::LOG_UINX);
					}
				}
			}

			delete pMessage;
		}
	}
}

std::string CLog::GetSwapLogName()
{
	time_t t;
	time(&t);
	struct tm l;
	localtime_r(&t, &l);
	char szBuf[64];
	snprintf(szBuf, sizeof(szBuf), "%04d%02d%02d%02d%02d%02d", l.tm_year + 1900, l.tm_mon + 1, l.tm_mday, l.tm_hour, l.tm_min, l.tm_sec);
	return std::string(szBuf);
}


void CLog::GetProcName()
{
	char szBuf[256] = {0};
	readlink("/proc/self/exe", szBuf, sizeof(szBuf));
	int iLen = strlen(szBuf);
	char* s = szBuf + iLen;
	while(*s != '/') -- s;
	++ s;
	m_sExeName = s;
}

FILE *CLog::OpenLogFile(bool bIsSwap, bool bIsCheck)
{
	std::string sFileName = m_sLogDir + m_sExeName;
	if (bIsCheck)
	{
		struct stat oFileInfo;
		oFileInfo.st_size = 0;
		std::string sName = sFileName;
		sName.append(".log");
		stat(sName.c_str(), &oFileInfo);
		if (oFileInfo.st_size > m_iLogMaxSize)
			bIsSwap = true;
		m_iCurLogSize = oFileInfo.st_size;
	}

	std::string sCurName = sFileName;
	sCurName.append(".log");

	if (bIsSwap)
	{
		std::string sNewName = sFileName;
		sNewName.append(GetSwapLogName()).append(".log");
		rename(sCurName.c_str(), sNewName.c_str());
		m_iCurLogSize = 0;
	}

	if (m_pFile)
		fclose(m_pFile);

	m_pFile = fopen(sCurName.c_str(), "a");
	if (!m_pFile)
	{
		printf("error: open log file failed, '%s'\n", sCurName.c_str());
		return 0;
	}
	return m_pFile;
}

int CLog::OpenNet(const char *pszAddr, int iMode, bool bIsReconnent)
{
	if (bIsReconnent)
		m_sAddr = pszAddr;

	if (iMode == CLogConfig::LOG_UINX)
	{
		CUnixCli *pCli = (CUnixCli *)GetFd(iMode);
		if (pCli->Create(pszAddr, 0, 0) < 0)
		{
			printf("error: create unix socket failed, %s\n", pCli->GetErr().c_str());
			return -1;
		}
	}
	else
	{
		const char *s = pszAddr;
		while (*s != ':')
			++s;
		std::string sAddr;
		sAddr.append(pszAddr, s - pszAddr);
		uint32_t dwPort = atoi(++s);
		if (sAddr.empty() || dwPort == 0)
		{
			printf("error: error ip:%s and port:%d '%s'\n", sAddr.c_str(), dwPort, pszAddr);
			return -1;
		}

		if (iMode == CLogConfig::LOG_TCP)
		{
			CTcpCli *pCli = (CTcpCli *)GetFd(iMode);
			if (pCli->Create(sAddr.c_str(), dwPort, 0, 0, m_dwVer) < 0)
			{
				printf("error: create tcp socket failed, %s\n", pCli->GetErr().c_str());
				return -1;
			}
		}
		else
		{
			CUdpCli *pCli = (CUdpCli *)GetFd(iMode);
			char szBuf[40];
			uint32_t dwLen;
			if (pCli->Create(sAddr.c_str(), dwPort, szBuf, &dwLen, m_dwVer) < 0)
			{
				printf("error: create tcp socket failed, %s\n", pCli->GetErr().c_str());
				return -1;
			}

			m_sUdpAddr.assign(szBuf, dwLen);
		}
	}

	return 0;
}

CFileFd *CLog::GetFd(int iMode)
{
	if (iMode == CLogConfig::LOG_UINX)
	{
		if (!m_pSock)
		{
			CUnixCli *pCli = new CUnixCli();
			m_pSock = pCli;
			pCli->SetSync();
		}
		else
			m_pSock->Close();
	}
	else if (iMode == CLogConfig::LOG_TCP)
	{
		if (!m_pSock)
		{
			CTcpCli *pCli = new CTcpCli();
			m_pSock = pCli;
			pCli->SetSync();
		}
		else
			m_pSock->Close();
	}
	else 
	{
		if (!m_pSock)
		{
			CUdpCli *pCli = new CUdpCli();
			m_pSock = pCli;
		}
		else
			m_pSock->Close();
	}
	return m_pSock;
}
