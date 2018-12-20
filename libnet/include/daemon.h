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

class CDaemon
{
public:
	static int DoDaemon(void (*)(int, const char**), int argc, const char** args, void (*)(int) = 0, bool bIsMonitor = true);

private:
	static int DaemonInit();
	static int MapPrint();
};

#endif
