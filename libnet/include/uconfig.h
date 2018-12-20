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

#ifndef __UCONFIG_H__
#define __UCONFIG_H__

#ifdef __USLEEP__
#define _usleep_() usleep(0)
#else
#define _usleep_()
#endif

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#endif
