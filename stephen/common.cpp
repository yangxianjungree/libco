#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fcntl.h>

#include "common.h"

#include "../co_routine_inner.h"

int pfn_event_loop(void *args)
{
//	DBG_INFO("One loop here in event loop. \n");

	return 1;
}

int CPoll(int fd, bool isPollIn, int misSeconds, const char *pFuncName, FuncPtrType pFuncAddr)
{
	DBG_INFO("Before calling CPoll, curr_co:[%p], function:[%s] -> addr:[%p], timeout:[%d].\n",
			 co_self(), pFuncName, pFuncAddr, misSeconds);

	struct pollfd pf = { 0 };
	pf.fd = fd;

	if (isPollIn) {
		pf.events = (POLLIN| POLLERR | POLLHUP);
	} else {
		pf.events = (POLLOUT | POLLERR | POLLHUP);
	}

	int pollRet = poll(&pf, 1, misSeconds);
	DBG_INFO("After calling CPoll, curr_co:[%p], function:[%s] -> addr:[%p], return:[%d].\n",
			 co_self(), pFuncName, pFuncAddr, pollRet);

	return pollRet;
}

int CreateListenFd(unsigned short port, unsigned int nListen)
{
	int listenFd;
	struct sockaddr_in servAddr;

	listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listenFd) {
		perror("socket");
		exit(-1);
	}

	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(port);

	if (-1 == bind(listenFd, (sockaddr *)&servAddr, sizeof(servAddr))) {
		perror("bind");
		exit(-1);
	}

	if (listen(listenFd, nListen) == -1) {
		perror("listen");
		exit(-1);
	}

	return listenFd;
}

int SetNonBlocking(int fd)
{
	int iFlags;
	iFlags = fcntl(fd, F_GETFL, 0);
	iFlags |= O_NONBLOCK;
	iFlags |= O_NDELAY;
	int ret = fcntl(fd, F_SETFL, iFlags);
	return ret;
}

int SetBlocking(int fd)
{
	int iFlags;

	iFlags = fcntl(fd, F_GETFL, 0);
	iFlags &= ~O_NONBLOCK;
	iFlags &= ~O_NDELAY;
	int ret = fcntl(fd, F_SETFL, iFlags);
	return ret;
}