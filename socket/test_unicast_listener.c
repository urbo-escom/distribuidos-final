#include "socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef NUM
#define NUM 10
#endif


void run(struct socket_monitor *m)
{
	struct socket_addr remote;
	struct socket_addr self;
	char remote_host[46];
	int  remote_port;
	char host[46];
	int  port;
	char buf[4096] = {0};
	socket_fd fd[NUM];
	int       fdlen = 0;
	int i;


	fdlen = socket_monitor_wait_ms(m, fd, NUM, 1000);
	if (-1 == fdlen) {
		perror("socket_monitor_wait_ms");
		return;
	}
	if (0 == fdlen) {
		fprintf(stderr, "timeout\n");
		return;
	}

	for (i = 0; i < fdlen; i++) {
		int s;
		s = socket_recvfrom(fd[i], buf, sizeof(buf), &remote);
		if (s < 0)
			return;
		socket_sendto(fd[i], buf, s, &remote);

		socket_addr_get_ipv4(&self, host, sizeof(host));
		socket_addr_get_port(&self, &port);
		printf("[%s:%d] <-\n", host, port);

		socket_addr_get_ipv4(&remote, remote_host, sizeof(remote_host));
		socket_addr_get_port(&remote, &remote_port);
		printf("[%s:%d]: '%*s'\n", remote_host, remote_port, s, buf);
	}
}


int main(int argc, char **argv)
{
	struct socket_monitor *m;
	socket_fd fd[NUM];
	size_t    fdlen = 0;
	int i;


	if (argc < 2) {
		fprintf(stderr, "usage: %s PORT...\n", argv[0]);
		return EXIT_FAILURE;
	}


	socket_init();
	socket_monitor_create(&m);

	for (i = 1; i < argc && fdlen < NUM; i++) {
		struct socket_addr self;
		char phost[46];
		int  pport;
		int  port;
		int  s;

		port = atoi(argv[i]);
		fd[fdlen] = socket_udp4_bind(&self, NULL, port);
		if (SOCKET_INVAL == fd[fdlen]) {
			perror("socket_udp4_bind");
			continue;
		}

		s = socket_monitor_add(m, fd[fdlen], SOCKET_MONITOR_RECV);
		if (-1 == s) {
			perror("socket_monitor_add");
			socket_close(fd[fdlen]);
			continue;
		}

		fdlen++;

		socket_addr_get_ipv4(&self, phost, sizeof(phost));
		socket_addr_get_port(&self, &pport);
		fprintf(stderr, "# Listening on [%s:%d]\n", phost, pport);
	}


	while (1)
		run(m);


	return EXIT_SUCCESS;
}
