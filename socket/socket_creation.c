#include "_common.h"


socket_fd socket_udp4()
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}


int socket_close(socket_fd fd)
{
	if (SOCKET_ERR == closesocket(fd))
		return -1;
	return 0;
}


int socket_bind(socket_fd fd, const struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != addr && "addr cannot be NULL");

	s = bind(fd, (struct sockaddr*)&addr->addr, addr->addrlen);
	if (SOCKET_ERR == s)
		return -1;

	return 0;
}


int socket_connect(socket_fd fd, const struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != addr && "addr cannot be NULL");

	s = connect(fd, (struct sockaddr*)&addr->addr, addr->addrlen);
	if (SOCKET_ERR == s)
		return -1;

	return 0;
}


int socket_getpeeraddr(socket_fd fd, struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != addr && "addr cannot be NULL");

	addr->addrlen = sizeof(addr->addr);
	s = getpeername(fd, (struct sockaddr*)&addr->addr, &addr->addrlen);
	if (SOCKET_ERR == s)
		return -1;

	return 0;
}


int socket_getselfaddr(socket_fd fd, struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != addr && "addr cannot be NULL");

	addr->addrlen = sizeof(addr->addr);
	s = getsockname(fd, (struct sockaddr*)&addr->addr, &addr->addrlen);
	if (SOCKET_ERR == s)
		return -1;

	return 0;
}


socket_fd socket_udp4_bind(struct socket_addr *addr,
		const char *ip, int port)
{
	struct addrinfo hints = {0};
	struct addrinfo *rp = NULL;
	struct addrinfo *r = NULL;
	socket_fd sock;
	char serv[16];
	int s;

	assert((0 <= port && port <= 65535) && "port must be 0-65536");

	sprintf(serv, "%d", port);

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags    = AI_PASSIVE;


	s = getaddrinfo(ip, serv, &hints, &r);
	if (0 != s)
		return SOCKET_INVAL;
	for (rp = r; NULL != rp; rp = rp->ai_next) {
		sock = socket(
			rp->ai_family,
			rp->ai_socktype,
			rp->ai_protocol
		);
		if (SOCKET_INVAL == sock)
			continue;

		s = bind(sock, rp->ai_addr, rp->ai_addrlen);
		if (SOCKET_ERR == s) {
			closesocket(sock), sock = SOCKET_INVAL;
			continue;
		}

		if (NULL != addr) {
			memcpy(&addr->addr, rp->ai_addr, rp->ai_addrlen);
			addr->addrlen = rp->ai_addrlen;
		}
		freeaddrinfo(r);
		return sock;
	}
	freeaddrinfo(r);
	return SOCKET_INVAL;
}
