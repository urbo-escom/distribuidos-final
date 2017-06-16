#include "_common.h"


int socket_recv(socket_fd fd, void *buf, size_t buflen)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != buf && "buf cannot be NULL");

	s = recv(fd, buf, buflen, 0);
#ifdef _WIN32
	if (SOCKET_ERR == s && WSAETIMEDOUT == GetLastError())
		SetLastError(0), errno = EAGAIN;
#endif
	if (SOCKET_ERR == s)
		return -1;

	return s;
}


int socket_send(socket_fd fd, const void *buf, size_t buflen)
{
	int s;


	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != buf && "buf cannot be NULL");

	s = send(fd, buf, buflen, 0);
#ifdef _WIN32
	if (SOCKET_ERR == s && WSAETIMEDOUT == GetLastError())
		SetLastError(0), errno = EAGAIN;
#endif
	if (SOCKET_ERR == s)
		return -1;

	return s;
}


int socket_recvfrom(socket_fd fd, void *buf, size_t buflen,
		struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != buf && "buf cannot be NULL");

	if (NULL != addr)
		addr->addrlen = sizeof(addr->addr);

	s = recvfrom(fd, buf, buflen, 0,
		NULL == addr ? NULL:(struct sockaddr*)&addr->addr,
		NULL == addr ? NULL:&addr->addrlen);
#ifdef _WIN32
	if (SOCKET_ERR == s && WSAETIMEDOUT == GetLastError())
		SetLastError(0), errno = EAGAIN;
#endif
	if (SOCKET_ERR == s)
		return -1;

	return s;
}


int socket_sendto(socket_fd fd, const void *buf, size_t buflen,
		const struct socket_addr *addr)
{
	int s;

	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");
	assert(NULL != buf && "buf cannot be NULL");
	assert(NULL != addr && "addr cannot be NULL");

	s = sendto(fd, buf, buflen, 0,
		(struct sockaddr*)&addr->addr,
		addr->addrlen);
#ifdef _WIN32
	if (SOCKET_ERR == s && WSAETIMEDOUT == GetLastError())
		SetLastError(0), errno = EAGAIN;
#endif
	if (SOCKET_ERR == s)
		return -1;

	return s;
}
