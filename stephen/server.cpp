#include <stack>
#include <memory>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>

#include "../co_routine.h"

#include "common.h"


// ======================================================================================

struct CONSUMER_CTX_T;

typedef struct PRODUCT_CTX_T {
	stCoRoutine_t *co;
	int listenFd;
	int acceptFd;

	bool alive;

	std::shared_ptr<std::stack<CONSUMER_CTX_T *>> pConsumers;
} co_product_t;

typedef struct CONSUMER_CTX_T {
	stCoRoutine_t *co;
	int fd;
	bool isWorking;

	std::shared_ptr<PRODUCT_CTX_T> pProduct;
} co_consumer_t;


// ======================================================================================

void *FuncConsumer(void *args)
{
	DBG_INFO("Enter function: [%s], address: [%p].\n", __func__, FuncConsumer);
	co_consumer_t *pCtxConsumer = (co_consumer_t *)args;
	int fd = pCtxConsumer->fd;

	int cmd;
	int recvLen = -1;

	while (true) {
		recvLen = recv(fd, &cmd, sizeof(cmd), MSG_DONTWAIT);
		DBG_INFO("func:[%s] line:[%d] The consumer recv data length:.\n", __func__, __LINE__, recvLen);

		if (recvLen == -1) {
			// todo
			if (errno == EPIPE) {
				break;
			}
		} else if (recvLen > 0) {
			DBG_INFO("func:[%s] line:[%d] The consumer recv command: %d.\n", __func__, __LINE__, cmd);
			if (cmd == 100) {
				break;
			}
			continue;
		} else {
			// todo
			break;
		}

		CPoll(fd, true, 2000, __FUNCTION__, FuncConsumer);
	}

	pCtxConsumer->isWorking = false;
	close(fd);
	shutdown(fd, SHUT_RDWR);
	pCtxConsumer->fd = -1;

	return args;
}

void *ConsumerMain(void *args)
{
	DBG_INFO("func:[%s] line:[%d] address:[%p] Enter function firstly.\n",
			 __func__, __LINE__, ConsumerMain);
	co_enable_hook_sys();

	co_consumer_t *pCtxConsumer = (co_consumer_t *)args;
	std::shared_ptr<co_product_t> pCtxProduct = pCtxConsumer->pProduct;

	while (true) {
		if (!pCtxConsumer->isWorking) {
			pCtxProduct->pConsumers->push(pCtxConsumer);
			co_yield_ct();
			DBG_INFO("func:[%s] line:[%d] Consumer return from self's co_yield_ct.\n", __func__, __LINE__);

			continue;
		}

		FuncConsumer(args);
	}

	return nullptr;
}

void *FuncProduct(void *args)
{
	DBG_INFO("func:[%s] line:[%d] address:[%p] Enter function firstly.\n",
			 __func__, __LINE__, FuncProduct);
	co_product_t *pCtxProduct = (co_product_t *)args;

	int listenFd = pCtxProduct->listenFd;
	struct sockaddr addr_accept;
	socklen_t len;
	bzero(&addr_accept, sizeof(struct sockaddr));

	while (true) {
		if (pCtxProduct->alive) {
			SetNonBlocking(listenFd);
		} else {
			DBG_INFO("func:[%s] line:[%d] Set listen fd: %d blocking.\n", __func__, __LINE__, listenFd);
			SetBlocking(listenFd);
			pCtxProduct->alive = true;
		}

		DBG_INFO("func:[%s] line:[%d] The product accept some one to connect.\n", __func__, __LINE__);
		int acceptFd = accept(listenFd, &addr_accept, &len);
		DBG_INFO("func:[%s] line:[%d] Accept return fd: %d.\n", __func__, __LINE__, acceptFd);

		if (-1 == acceptFd) {
			if (errno == EAGAIN) {
				CPoll(listenFd, true, 200, __FUNCTION__, FuncProduct);
			} else {
				DBG_INFO("func:[%s] line:[%d] Some other error, errno: %d.\n", __func__, __LINE__, errno);
				break;
			}
		} else {
			pCtxProduct->acceptFd = acceptFd;
			break;
		}
	}

	return nullptr;
}

void *ProductMain(void *args)
{
	DBG_INFO("func:[%s] line:[%d] address:[%p] Enter function firstly.\n",
			 __func__, __LINE__, ProductMain);
	co_enable_hook_sys();

	co_product_t *pCtxProduct = (co_product_t *)args;
	while (true) {
		if (pCtxProduct->pConsumers->empty()) {
			poll(nullptr, 0, 1000);

			continue;
		}

		co_consumer_t *pctxConsumer = pCtxProduct->pConsumers->top();
		pCtxProduct->pConsumers->pop();

		FuncProduct(args);

		pctxConsumer->isWorking = true;
		pctxConsumer->fd = pCtxProduct->acceptFd;
		co_resume(pctxConsumer->co);
		DBG_INFO("func:[%s] line:[%d] The consumer on the top return from product's resume.\n",
				 __func__, __LINE__);
	}

	return nullptr;
}

typedef struct CO_TASK_CTX_T {
	stCoRoutine_t *co;
	std::shared_ptr<co_product_t> pProduct;
	int fd;
} co_task_t;

void *TaskMain(void *args)
{
	DBG_INFO("func:[%s] line:[%d] address:[%p] Enter function firstly.\n",
			 __func__, __LINE__, TaskMain);

	co_enable_hook_sys();

	co_task_t *pTask = (co_task_t *)args;

	std::shared_ptr<co_product_t> pProduct(new co_product_t);
	std::shared_ptr<std::stack<co_consumer_t*>> pConsumers(new std::stack<co_consumer_t*>);

	pTask->pProduct = pProduct;

	pProduct->pConsumers = pConsumers;
	pProduct->listenFd = pProduct->acceptFd = -1;
	pProduct->listenFd = pTask->fd;
	pProduct->alive = false;

	for (int i = 0; i < 5; ++i) {
		co_consumer_t *pConsumer = new co_consumer_t;
		pConsumer->isWorking = false;
		pConsumer->fd = -1;
		pConsumer->pProduct = pProduct;

		co_create(&pConsumer->co, NULL, ConsumerMain, (void*)pConsumer);
		co_resume(pConsumer->co);
		DBG_INFO("func:[%s] line:[%d] The consumer coroutine return from create resume.\n", __func__, __LINE__);
	}

	co_create(&pProduct->co, NULL, ProductMain, (void*)(pProduct.get()));
	co_resume(pProduct->co);
	DBG_INFO("func:[%s] line:[%d] The product coroutine return from create resume.\n", __func__, __LINE__);

	return nullptr;
}

// ======================================================================================

int main()
{
	std::shared_ptr<co_task_t> pTask(new co_task_t);
	pTask->fd = CreateListenFd(8888, 5);

	co_create(&pTask->co, nullptr, TaskMain, (void*)pTask.get());
	co_resume(pTask->co);
	DBG_INFO("func:[%s] line:[%d] The task coroutine return from resume and enter event loop.\n",
			 __func__, __LINE__);

	co_eventloop(co_get_epoll_ct(), pfn_event_loop, NULL);

	return 0;
}