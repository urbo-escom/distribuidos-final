#include "_common.h"


int socket_addr_set_ipv4(struct socket_addr *addr, const char *ip)
{
	struct addrinfo hints = {0};
	struct addrinfo *r = NULL;
	int s;

	assert(NULL != addr && "addr cannot be NULL");
	if (NULL == ip) {
		struct sockaddr_in *a4;
		addr->addr.ss_family = AF_INET;
		a4 = (struct sockaddr_in*)&addr->addr;
		memset(&a4->sin_addr, 0, sizeof(a4->sin_addr));
		addr->addrlen = sizeof(addr->addr);
		return 0;
	}

	hints.ai_family = AF_INET;
	hints.ai_flags  = AI_NUMERICHOST;
	s = getaddrinfo(ip, NULL, &hints, &r);
	if (0 != s)
		return -1;
	memset(&addr->addr, 0, sizeof(addr->addr));
	memcpy(&addr->addr, r->ai_addr, r->ai_addrlen);
	addr->addrlen = r->ai_addrlen;
	freeaddrinfo(r);
	return 0;
}


int socket_addr_set_port(struct socket_addr *addr, int port)
{
	struct sockaddr_in  *addr4 = NULL;
	struct sockaddr_in6 *addr6 = NULL;

	assert(NULL != addr && "addr cannot be NULL");

	switch (addr->addr.ss_family) {
	default:
		return -1;

	case AF_INET:
		addr4 = (struct sockaddr_in*)addr;
		addr4->sin_port = htons(port);
		return 0;

	case AF_INET6:
		addr6 = (struct sockaddr_in6*)addr;
		addr6->sin6_port = htons(port);
		return 0;
	}
	return -1;
}


int socket_addr_get_ipv4(const struct socket_addr *addr, char *ip, size_t n)
{
	int s;

	assert(NULL != addr && "addr cannot be NULL");
	assert(NULL != ip && "ip cannot be NULL");
	if (0 == n)
		return 0;
	if (1 == n)
		return *ip = '\0', 0;

	s = getnameinfo((struct sockaddr*)&addr->addr, addr->addrlen,
		ip, n,
		NULL, 0, 0
		| NI_NUMERICHOST
		| NI_NUMERICSERV
	);
	if (0 == s)
		return 0;
	sprintf(ip, "-");
	return -1;
}


int socket_addr_get_port(const struct socket_addr *addr, int *port)
{
	struct sockaddr_in  *addr4 = NULL;
	struct sockaddr_in6 *addr6 = NULL;

	assert(NULL != addr && "addr cannot be NULL");
	if (NULL == port)
		return 0;

	switch (addr->addr.ss_family) {
	default:
		return -1;

	case AF_INET:
		addr4 = (struct sockaddr_in*)addr;
		*port = ntohs(addr4->sin_port);
		break;

	case AF_INET6:
		addr6 = (struct sockaddr_in6*)addr;
		*port = ntohs(addr6->sin6_port);
		break;
	}
	return -1;
}


struct socket_addr* socket_addr_cpy(
		struct socket_addr *dst,
		const struct socket_addr *src)
{
	assert(NULL != dst && "dst addr cannot be NULL");
	assert(NULL != src && "src addr cannot be NULL");
	return memcpy(dst, src, sizeof(*dst));
}


int socket_addr_cmp(const struct socket_addr *a, const struct socket_addr *b)
{
	assert(NULL != a && "a addr cannot be NULL");
	assert(NULL != b && "b addr cannot be NULL");

	if (a->addr.ss_family == b->addr.ss_family
	&&  a->addrlen == b->addrlen)
		return memcmp(a, b, a->addrlen);

	if (a->addr.ss_family == b->addr.ss_family)
		return a->addrlen - b->addrlen;

	if (AF_INET6 == a->addr.ss_family)
		return 1;
	if (AF_INET6 == b->addr.ss_family)
		return -1;
	if (AF_INET == a->addr.ss_family)
		return 1;
	return memcmp(a, b, a->addrlen < b->addrlen ? a->addrlen:b->addrlen);
}
