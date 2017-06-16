/*
 * cross-platform socket library (IPv4-only)
 */
#ifndef SOCKET_H
#define SOCKET_H


#ifdef _WIN32
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501 /* Windows XP */
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	typedef SOCKET socket_fd;
	#define SOCKET_INVAL INVALID_SOCKET
	#define SOCKET_ERR   SOCKET_ERROR
#else
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <unistd.h>
	typedef int socket_fd;
	#define SOCKET_INVAL (-1)
	#define SOCKET_ERR   (-1)
#endif

#include <stddef.h>


struct socket_addr {
	struct sockaddr_storage addr;
	socklen_t               addrlen;
};


/* Must be called before any other function */
int socket_init();

/* Should be called at the end of the program */
int socket_finalize();


/*
 * addr functions
 */

int socket_addr_set_ipv4(struct socket_addr *addr, const char *ip);
int socket_addr_set_port(struct socket_addr *addr, int port);
int socket_addr_get_ipv4(const struct socket_addr *addr, char *ip, size_t n);
int socket_addr_get_port(const struct socket_addr *addr, int *port);

struct socket_addr* socket_addr_cpy(
		struct socket_addr *dst,
		const struct socket_addr *src);
int          socket_addr_cmp(
		const struct socket_addr *a,
		const struct socket_addr *b);


/*
 * socket functions
 */

socket_fd socket_udp4(void);
int       socket_close(socket_fd fd);

int socket_bind(   socket_fd fd, const struct socket_addr *addr);
int socket_connect(socket_fd fd, const struct socket_addr *addr);
int socket_getpeeraddr(socket_fd fd, struct socket_addr *addr);
int socket_getselfaddr(socket_fd fd, struct socket_addr *addr);

socket_fd socket_udp4_bind(struct socket_addr *addr,
		const char *ip, int port);
socket_fd socket_udp4_connect(struct socket_addr *addr,
		const char *ip, int port);


/*
 * socket send/recv
 */

int socket_send(  socket_fd fd, const void *buf, size_t len);
int socket_sendto(socket_fd fd, const void *buf, size_t len,
		const struct socket_addr *addr);

int socket_recv(    socket_fd fd, void *buf, size_t len);
int socket_recvfrom(socket_fd fd, void *buf, size_t len,
		struct socket_addr *addr);


/*
 * socket options
 */
int socket_setnonblocking(socket_fd fd);
int socket_setbroadcast(socket_fd fd);
int socket_settimetolive(socket_fd fd, int ttl);
int socket_group_join( socket_fd fd, const struct socket_addr *addr);
int socket_group_leave(socket_fd fd, const struct socket_addr *addr);
int socket_recv_timeout_ms(socket_fd fd, int msec);
int socket_send_timeout_ms(socket_fd fd, int msec);


/*
 * socket monitor
 */

typedef struct socket_monitor socket_monitor;

#define SOCKET_MONITOR_SEND (0x01)
#define SOCKET_MONITOR_RECV (0x02)

int socket_monitor_create(socket_monitor **m);
int socket_monitor_free(socket_monitor *m);
int socket_monitor_add(   socket_monitor *m,       socket_fd fd, int options);
int socket_monitor_remove(socket_monitor *m, const socket_fd fd);
int socket_monitor_wait_ms(socket_monitor *m,
		socket_fd *fd, size_t fd_size,
		int time_ms);


#endif /* !SOCKET_H */
