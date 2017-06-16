#include "socket.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#define preserving_error(s) do { \
		int   err_; \
		DWORD errw32_; \
		errw32_ = GetLastError(); \
		err_    = errno; \
		s; \
		SetLastError(errw32_); \
		errno = err_; \
	} while (0)
#else
	#define closesocket(s) close(s)
	#define preserving_error(s) do { \
		int   err_; \
		err_  = errno; \
		s; \
		errno = err_; \
	} while (0)
#endif


struct socket_monitor {
	socket_fd max_fd;
	fd_set    read_set;
	fd_set    read_ready_set;
	fd_set    write_set;
	fd_set    write_ready_set;
	struct {
		socket_fd fd;
		int       options;
	}      socklist[FD_SETSIZE];
	size_t socklist_len;
	size_t socklist_size;
};
