#ifndef __STEPHEN_COMMON_H__
#define __STEPHEN_COMMON_H__


#ifdef DEBUG
	#include "../co_routine.h"
	#define DBG_INFO(fmt, a...) co_log_err(fmt, ##a)
#else
	#define DBG_INFO(fmt, a...) {}
#endif


typedef void* (*FuncPtrType)(void*);


int pfn_event_loop(void *);

int CPoll(int fd, bool isPollIn, int misSeconds, const char *pFuncName = nullptr, FuncPtrType pFuncAddr = nullptr);

int CreateListenFd(unsigned short port, unsigned int nListen);

int SetNonBlocking(int fd);

int SetBlocking(int fd);

#endif//__STEPHEN_COMMON_H__