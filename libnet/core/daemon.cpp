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

static std::string GetProcPidFile(const char *p)
{
	int len = strlen(p);
	const char *e = p + (len - 1);
	while (*e && len >= 0)
	{
		if (*e == '/')
			break;
		e--;
		len--;
	}
	e++;
	std::string sName = ".";
	return sName.append(e);
}

static void WritePid(const char* p)
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

static void CheckProcess(const char *p)
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

int CDaemon::DaemonInit()
{
	pid_t pid;
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

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

		// if (MapPrint() == -1)
		// 	break;
		return 0;
	}while(0);
	return -1;
}

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
			signal(SIGHUP, Call);
			signal(SIGQUIT, SIG_IGN);

			Dmain(argc, args);
			break;
		}
	}
	return pid;
}
