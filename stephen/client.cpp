#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>
#include <netinet/in.h>
#include <cstring>

int main()
{
	// create socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd) {
		perror("socket");
		exit(-1);
	}

	// connect
	struct sockaddr_in addrIn = {};
	addrIn.sin_family = AF_INET;
	addrIn.sin_port = htons(8888);
	addrIn.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (-1 == connect(fd, (struct sockaddr *)&addrIn, sizeof(addrIn))) {
		perror("connect");
		exit(-1);
	}

	int i = 0;
	while (i < 40) {
		if (i % 10 == 9) {
			printf("sleep 2s.\n");
			sleep(2);
		}
		send(fd, &i, sizeof(i), MSG_DONTWAIT);
		i++;
		printf("send %d.\n", i);
	}

	close(fd);
	shutdown(fd, SHUT_RDWR);

	return 0;
}