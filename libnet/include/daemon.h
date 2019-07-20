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

#ifndef __DAEMON__H__
#define __DAEMON__H__

#include <signal.h>
#include <string>

namespace znet
{

class CDaemon
{
public:
	// @Param DoMain 进程主函数回调
	// @Param argc 参数数量
	// @Param args 参数列表
	// @Param 收到退出信号回调函数
	// 函数托离终端
	static int DoDaemon(void (*)(int, const char**), int argc, const char** args, void (*)(int) = 0, bool bIsMonitor = true);
	// @Param pszProcCmd args[0]
	// 通知退出进程
	static void Quit(const char* pszProcCmd);
	// @Param pszProcCmd args[0]
	// 通知重启子进程
	static void RestartChildProcess(const char *pszProcCmd);
	// @Param pszProcCmd args[0]
	// 通过执行进程命令args[0]获得进程名
	static std::string GetProc(const char* pszProcCmd);
	// @Param iPid 进程的PID
	// 通过PID获得进程名
	static std::string GetProc(int iPid);

private:
	// Daemon 初始化
	static int DaemonInit();
	// 重定向标准输入输出
	static int MapPrint();
	// 获得进程PID文件名
	static std::string GetProcPidFile(const char* pszProcCmd);
	// 写进程PID
	static void WritePid(const char* p);
	// 检查进程是否执行
	static void CheckProcess(const char *p);
	// @Param pszProcCmd args[0]
	// 通过执行进程命令args[0]获得老进程PID
	static int GetOldPid(const char* pszProcName);
};

}

#endif
