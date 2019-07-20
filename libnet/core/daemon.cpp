/*
 * daemon.cpp
 *
 *  Created on: Dec 18, 2018
 *      Author: xinglin
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <daemon.h>
#include <assert.h>
#include <string.h>
#include <string>

using namespace znet;

#pragma GCC diagnostic ignored "-Wunused-result"

// @Param pszProcCmd args[0]
// 通过执行进程命令args[0]获得进程名
std::string CDaemon::GetProc(const char* pszProcCmd)
{
	int len = strlen(pszProcCmd);
    const char *e = pszProcCmd + (len - 1);
    while (*e && len >= 0)
    {
        if (*e == '/')
            break;
        e--;
        len--;
    }

    e++;
    return e;
}

// @Param iPid 进程的PID
// 通过PID获得进程名
std::string CDaemon::GetProc(int iPid)
{
	std::string sComm = "/proc/";
	sComm.append(std::to_string(iPid)).append("/comm");
	FILE* fp = fopen(sComm.c_str(), "rb");
	if (!fp)
		return std::string();
	char szBuf[64];
	int iLen = fread(szBuf, 1 , sizeof(szBuf), fp);
	fclose(fp);

	char* p = szBuf;
	while (iLen > 0 && *p)
	{
		if (*p == '\r' || *p == '\n')
		{
			*p = 0;
			break;
		}
		++ p;
		-- iLen;
	}

	return std::string(szBuf);
}

// 获得进程PID文件名
std::string CDaemon::GetProcPidFile(const char *p)
{
	std::string sName = ".";
	return sName.append(GetProc(p)).append(".pid");
}

// 写进程PID
void CDaemon::WritePid(const char* p)
{
	FILE *fp = fopen(GetProcPidFile(p).c_str(), "wb");
	if (!fp)
	{
		printf("error: process pid failed!\n");
		exit(0);
		return ;
	}
	fprintf(fp, "%d", getpid());
	fclose(fp);
}

// 检查进程是否执行
void CDaemon::CheckProcess(const char *p)
{
	FILE *fp = fopen(GetProcPidFile(p).c_str(), "rb");
	if (!fp)
		return ;
	char szBuf[64] = {0};
	char* s = fgets(szBuf, sizeof(szBuf), fp);
	fclose(fp);
	if (!s)
		return;

	std::string sComm = "/proc/";
	sComm.append(szBuf).append("/comm");
	fp = fopen(sComm.c_str(), "rb");
	if (!fp)
		return;
	fclose(fp);

	printf("process runing\n");
	exit(0);
}

// 重定向标准输入输出
int CDaemon::MapPrint()
{
	int iFd;
	do
	{
		iFd = open("/dev/null", O_RDWR);
		if (iFd == -1)
		{
			break;
		}

		if (dup2(iFd, STDIN_FILENO) == -1)
		{
			break;
		}

		if (dup2(iFd, STDOUT_FILENO) == -1)
		{
			break;
		}

		if (dup2(iFd, STDERR_FILENO) == -1)
		{
			break;
		}
		if (close(iFd) == -1)
		{
			iFd = -1;
			break;
		}
		return 0;
	}while(0);
	
	if (iFd != -1)
		close(iFd);
	return -1;
}

// Daemon 初始化
int CDaemon::DaemonInit()
{
	pid_t pid;
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	do
	{
		pid = fork();
		if (pid > 0)
			return pid;
		else if (pid < 0)
		{
			break;
		}

		if (setsid() == -1)
		{
			break;
		}
		umask(0);

		pid = fork();
		if (pid < 0)
			break;

		if (pid > 0)
		{
			exit(0);
			break;
		}

//		if (chdir("/") < 0)
//			break;

		if (MapPrint() == -1)
			break;
		return 0;
	}while(0);
	return -1;
}

// @Param DoMain 进程主函数回调
// @Param argc 参数数量
// @Param args 参数列表
// @Param 收到退出信号回调函数
// 函数托离终端
int CDaemon::DoDaemon(void (*Dmain)(int, const char **), int argc, const char **args, void(*Call)(int), bool bIsMonitor)
{
	assert(args != nullptr);
	assert(Dmain != nullptr);
	CheckProcess(args[0]);
	int iRet = DaemonInit();
	if (iRet < 0)
	{
		exit(0);
		return -1;
	}
	if (iRet != 0)
		return 0;

	WritePid(args[0]);
	pid_t pid = 0;
	while (1)
	{
		pid = fork();
		if (pid < 0)
			break;

		if (pid > 0)
		{
			static pid_t child;
			child = pid;
			signal(SIGHUP, [](int){
				kill(child, SIGHUP);
			});

			signal(SIGQUIT, [](int){
				kill(child, SIGHUP);
				exit(0);
			});

			if (bIsMonitor && Call)
				Call(0);

			while (bIsMonitor)
			{
				int iStatu;
				int iRet = wait(&iStatu);
				if (-1 == iRet || errno == EINTR)
				{
					continue;
				}
				sleep(2);
				break;
			}

			if (!bIsMonitor)
				break;

			if (Call)
				Call(-1);
		}
		else
		{
			if (Call)
				signal(SIGHUP, Call);
			else
				signal(SIGHUP, SIG_IGN);
			
			signal(SIGQUIT, SIG_IGN);

			Dmain(argc, args);
			break;
		}
	}
	return pid;
}

// @Param pszProcCmd args[0]
// 通过执行进程命令args[0]获得老进程PID
int CDaemon::GetOldPid(const char* pszProcName)
{
	std::string sName = GetProcPidFile(pszProcName);

	FILE* fp = fopen(sName.c_str(), "rb");
    if (!fp)
    {
        printf("process pid file '%s' not exist\n", sName.c_str());
        return 0;
    }
    int pid = 0;
    fscanf(fp, "%d", &pid);
    fclose(fp);
    if (pid == 0)
    {
        printf("process pid file '%s', process pid = 0\n", sName.c_str());
        return 0;
    }

    std::string sComm = "/proc/";
    sComm.append(std::to_string(pid)).append("/comm");
    fp = fopen(sComm.c_str(), "rb");
    if (!fp)
    {
        printf("process not run\n");
        return 0;
    }
    fclose(fp);

    return pid;
}

// @Param pszProcCmd args[0]
// 通知退出进程
void CDaemon::Quit(const char* p)
{
	int pid = GetOldPid(p);
	if (pid == 0)
		return;
	kill(pid, SIGQUIT);
	printf("send SIGQUIT %d success\n", pid);
	exit(0);
}

// @Param pszProcCmd args[0]
// 通知重启子进程
void CDaemon::RestartChildProcess(const char *pszProcCmd)
{
	int pid = GetOldPid(pszProcCmd);
	if (pid == 0)
		return;
	kill(pid, SIGHUP);
	printf("send SIGHUP %d success\n", pid);
	exit(0);
}
