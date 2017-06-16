#include "_common.h"


int socket_monitor_create(socket_monitor **m)
{
	socket_monitor *p;
	size_t i;

	assert(NULL != m && "m cannot be NULL");

	p = calloc(1, sizeof(*p));
	if (NULL == p)
		return -1;
	FD_ZERO(&p->read_set);
	FD_ZERO(&p->write_set);
	FD_ZERO(&p->read_ready_set);
	FD_ZERO(&p->write_ready_set);
	p->socklist_size = sizeof(p->socklist)/sizeof(p->socklist[0]);
	for (i = 0; i < p->socklist_size; i++)
		p->socklist[i].fd = SOCKET_INVAL;

	*m = p;
	return 0;
}


int socket_monitor_free(socket_monitor *m)
{
	if (NULL == m)
		return 0;
	free(m);
	return 0;
}


int socket_monitor_add(socket_monitor *m, socket_fd fd, int options)
{
	size_t i;

	assert(NULL != m && "m cannot be NULL");
	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");

	if (m->socklist_size <= m->socklist_len)
		return -1;
	if (!options)
		return 0;

	for (i = 0; i < m->socklist_len; i++) {
		int old_options;
		if (m->socklist[i].fd != fd)
			continue;
		old_options = m->socklist[i].options;

		if (old_options & SOCKET_MONITOR_SEND)
			FD_CLR(fd, &m->write_set);

		if (old_options & SOCKET_MONITOR_RECV)
			FD_CLR(fd, &m->read_set);

		if (options & SOCKET_MONITOR_SEND)
			FD_SET(fd, &m->write_set);

		if (options & SOCKET_MONITOR_RECV)
			FD_SET(fd, &m->read_set);

		return m->socklist[i].options = options, 0;
	}

	if (options & SOCKET_MONITOR_SEND)
		FD_SET(fd, &m->write_set);

	if (options & SOCKET_MONITOR_RECV)
		FD_SET(fd, &m->read_set);

	if (m->max_fd < fd)
		m->max_fd = fd;

	m->socklist[m->socklist_len].fd      = fd;
	m->socklist[m->socklist_len].options = options;
	m->socklist_len++;
	return 0;
}


int socket_monitor_remove(socket_monitor *m, socket_fd fd)
{
	size_t i;

	assert(NULL != m && "m cannot be NULL");
	assert(SOCKET_INVAL != fd && "fd cannot be SOCKET_INVAL");

	for (i = 0; i < m->socklist_len; i++) {
		if (m->socklist[i].fd != fd)
			continue;

		if (m->socklist[i].options & SOCKET_MONITOR_SEND)
			FD_CLR(fd, &m->write_set);

		if (m->socklist[i].options & SOCKET_MONITOR_RECV)
			FD_CLR(fd, &m->read_set);

		break;
	}
	if (i == m->socklist_len)
		return -1;
	m->socklist_len--;
	for (; i < m->socklist_len; i++) {
		m->socklist[i].fd      = m->socklist[i + 1].fd;
		m->socklist[i].options = m->socklist[i + 1].options;
	}
	m->socklist[i].fd = SOCKET_INVAL;
	m->socklist[i].options = 0;

	return 0;
}


static size_t fill_ready(socket_monitor *m,
		socket_fd *fds, size_t len)
{
	size_t ready = 0;
	size_t i;

	for (i = 0; i < m->socklist_len && ready < len; i++) {
		socket_fd fd = m->socklist[i].fd;

		if (FD_ISSET(fd, &m->read_ready_set)
		&&  FD_ISSET(fd, &m->write_ready_set)) {
			FD_CLR(fd, &m->read_ready_set);
			FD_CLR(fd, &m->write_ready_set);
			fds[ready++] = fd;
			continue;
		}

		if (FD_ISSET(fd, &m->read_ready_set)) {
			FD_CLR(fd, &m->read_ready_set);
			fds[ready++] = fd;
			continue;
		}

		if (FD_ISSET(fd, &m->write_ready_set)) {
			FD_CLR(fd, &m->write_ready_set);
			fds[ready++] = fd;
			continue;
		}
	}

	return ready;
}


int socket_monitor_wait_ms(socket_monitor *m,
		socket_fd *fds, size_t len,
		int time_ms)
{
	struct timeval  tv_alloc;
	struct timeval *tv = &tv_alloc;
	size_t ready = 0;
	int s;

	ready = fill_ready(m, fds, len);
	if (ready)
		return ready;

	if (time_ms < 0)
		tv = NULL;

	if (NULL != tv) {
		tv->tv_sec = time_ms/1000;
		tv->tv_usec = time_ms%1000*1000;
	}

	memcpy(&m->read_ready_set, &m->read_set, sizeof(m->read_set));
	memcpy(&m->write_ready_set, &m->write_set, sizeof(m->write_set));
	s = select(m->max_fd + 1,
		&m->read_ready_set,
		&m->write_ready_set,
		NULL,
		tv
	);
	if (SOCKET_ERR == s)
		return -1;
	if (!s)
		return 0;

	return fill_ready(m, fds, len);
}
