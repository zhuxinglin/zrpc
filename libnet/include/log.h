//////////////////////////////////////////////////////////////////////////
// file name : log.h
// author : 
// create time : 
// 
// 
// 
// 
// 
//////////////////////////////////////////////////////////////////////////

#ifndef __LOG__H__
#define __LOG__H__

#include <stdint.h>
#include <stdio.h>
#include <thread.h>
#include <socket_fd.h>
#include <sstream>
#include <time.h>
#include <queue>
#include <unistd.h>
#include "coroutine.h"
#include <sys/syscall.h>

namespace znet
{

typedef struct _LogConfig
{
	_LogConfig() : sLogDir("../log/"),
				   dwWriteMode(LOG_DEF),
				   dwLevel(LOG_INFO_LEVEL),
				   iLogMaxSize(20 * 1024 * 1024),
				   m_iVer(4)
	{}

	std::string sLogDir;
	std::string sNetAddr;	// ip:port

	enum
	{
		LOG_DEF = 0x01,
		LOG_FILE = 0x02,
		LOG_TCP = 0x04,
		LOG_UINX = 0x08,
		LOG_UDP = 0x10,
	};

	uint32_t dwWriteMode;

	enum
	{
		LOG_OFF = 0x000,
		LOG_FATAL_LEVEL = 0x001,
		LOG_FATAL = 0x001,
		LOG_ERROR_LEVEL = 0x003,
		LOG_ERROR = 0x002,
		LOG_WARN_LEVEL = 0x007,
		LOG_WARN = 0x004,
		LOG_INFO_LEVEL = 0x00F,
		LOG_INFO = 0x008,
		LOG_DEBUG_LEVEL = 0x01F,
		LOG_DEBUG = 0x010,
		LOG_ALL_LEVEL = 0x01F,
		LOG_ALL = LOG_DEBUG,

	};
	uint32_t dwLevel;
	int iLogMaxSize;
	int m_iVer;
} CLogConfig;

template <class T = typename std::stringstream>
class CLogMessage;

class CLog : public CThread
{
private:
	CLog();
	~CLog();

public:
	static CLog* GetObj();
	static void SetObj(CLog* pLog);
	static void DelObj();

public:
	bool Create(CLogConfig* pConfig);
	bool IsInit() { return m_bIsInit; }
	int Write(CLogMessage<> *pMessage, uint32_t dwCurLevel);
	void SetLevel(uint32_t dwLevel){m_dwLevel = dwLevel;}
	uint32_t GetLevel() {return m_dwLevel;}
	void SetMode(uint32_t dwWriteMode){m_dwWriteMode = dwWriteMode;};
	void SetSwapFileSize(int iSize){ if (iSize > 1024 * 1024)m_iLogMaxSize = iSize;}

private:
	virtual void Run(uint32_t);
	void GetProcName();
	FILE* OpenLogFile(bool bIsSwap, bool bIsCheck = false);
	std::string GetSwapLogName();
	int OpenNet(const char* pszAddr, int iMode, bool bIsReconnent = false);
	CFileFd* GetFd(int iMode);
	void GenDir();

private:
	bool m_bIsInit;
	std::string m_sLogDir;
	std::string m_sAddr;
	std::string m_sUdpAddr;
	uint16_t m_dwVer;
	uint32_t m_dwWriteMode;
	uint32_t m_dwLevel;
	int m_iLogMaxSize;
	int m_iCurLogSize;
	FILE* m_pFile;
	CFileFd *m_pSock;
	typedef std::queue<CLogMessage<> *> MessageQueue;
	MessageQueue m_oMsgQueue;
	volatile uint32_t m_dwSync;
	std::string m_sExeName;
	CSem m_oSem;
	static CLog* m_pSelf;
};

template <class T>
class CLogMessage
{
public:
	CLogMessage(const char* pszFile, const char* pszFunciont, int iLine, const char* pszLevel)
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		struct tm t;
		localtime_r(&ts.tv_sec, &t);
		char szBuf[64];
		snprintf(szBuf, sizeof(szBuf), "[%04d-%02d-%02d %02d:%02d:%02d.%09lu] [",
				 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, ts.tv_nsec);
		uint64_t qwCid = 0;
		if (CCoroutine::GetObj()->GetTaskBase())
			qwCid = CCoroutine::GetObj()->GetTaskBase()->m_qwCid;
		oStream << szBuf << pszFile << " " << pszFunciont << ":" << iLine;
		oStream << "] [pid:" << getpid() << " " << "tid:" << syscall(SYS_gettid) << " cid:" << qwCid << "] ";
		oStream << pszLevel << " ";
	};
	~CLogMessage(){};

	typedef T value_type;
	value_type oStream;
};

template <class T = CLogMessage<> >
class CLogManage
{
public:
	explicit CLogManage(T* p, uint32_t dwLevel):m_ptr(p),m_dwLevel(dwLevel){}
	~CLogManage() { if (m_ptr)CLog::GetObj()->Write(m_ptr, m_dwLevel); }

	inline typename T::value_type &operator()()
	{
		return m_ptr->oStream;
	}

private:
	T* m_ptr;
	uint32_t m_dwLevel;
};

class CLogVoid
{
public:
	CLogVoid(){}
	void operator & (std::ostream& ){}
};

}

#define LOG_WRITE(x, y) (znet::CLogManage<>(x, y))
#define LOG_LEVE(x) (znet::CLog::GetObj()->GetLevel() & x)

#define LOG_MESSAGE(x, y) !!!LOG_LEVE(x) ? (void) 0 : znet::CLogVoid() & LOG_WRITE(new (std::nothrow) znet::CLogMessage<>(__FILE__, __FUNCTION__, __LINE__, #y), x)()

#define LOGF	LOG_MESSAGE(znet::CLogConfig::LOG_FATAL, FATAL:)
#define LOGE	LOG_MESSAGE(znet::CLogConfig::LOG_ERROR, ERROR:)
#define LOGW	LOG_MESSAGE(znet::CLogConfig::LOG_WARN, WARN:)
#define LOGI	LOG_MESSAGE(znet::CLogConfig::LOG_INFO, INFO:)
#define LOGD	LOG_MESSAGE(znet::CLogConfig::LOG_DEBUG, DEBUG:)


#endif
