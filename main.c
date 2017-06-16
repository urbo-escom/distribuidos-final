#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "videojuego.h"


const char *progname = "laberynth";


void print_options(struct videojuego *vj)
{
	fprintf(stderr, "HOST  = \"%s\"\n", vj->host ? vj->host:"0.0.0.0");
	fprintf(stderr, "GROUP = \"%s\"\n", vj->group);
	fprintf(stderr, "PORT  = %d\n", vj->port);
}


void print_help(struct videojuego *vj)
{
	fprintf(stderr, "USAGE: %s [OPTIONS]... KAZAA_DIR TRASH_DIR\n",
		progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tPrint this help\n\n");

	fprintf(stderr, "\t--host IP\n");
	fprintf(stderr, "\t\tSet IP as HOST (for binding)\n\n");

	fprintf(stderr, "\t--group IP\n");
	fprintf(stderr, "\t\tSet IP as GROUP (for multicast)\n\n");

	fprintf(stderr, "\t--port NUM\n");
	fprintf(stderr, "\t\tSet NUM as PORT\n\n");

	print_options(vj);
}


int parse_args(struct videojuego *vj, int argc, char **argv)
{
	int i;
	int can_exit = 0;


	for (i = 0; i < argc; i++) {
		if (0 == strcmp("--help", argv[i])) {
			can_exit = 1;
			continue;
		}

		if (0 == strcmp("--host", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(vj);
				fprintf(stderr, "Missing --host arg\n");
				exit(EXIT_FAILURE);
			}
			vj->host = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--group", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(vj);
				fprintf(stderr, "Missing --group arg\n");
				exit(EXIT_FAILURE);
			}
			vj->group = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--port", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(vj);
				fprintf(stderr, "Missing --port arg\n");
				exit(EXIT_FAILURE);
			}
			vj->port = strtol(argv[i + 1], NULL, 0);
			i++;
			continue;
		}

	}

	if (can_exit) {
		print_help(vj);
		exit(EXIT_FAILURE);
	}

	return 0;
}


int main(int argc, char **argv)
{
	struct videojuego *vj = NULL;
	pthread_t thread_recv;
	pthread_t thread_send;
	pthread_t thread_play;
	int s;


#ifdef _WIN32
	if (NULL != getenv("MSYSTEM") || NULL != getenv("CYGWIN")) {
		setvbuf(stdout, 0, _IONBF, 0);
		setvbuf(stderr, 0, _IONBF, 0);
	}
#endif
	srand(time(NULL));


	vj = calloc(1, sizeof(*vj));
	if (NULL == vj) {
		perror("calloc");
		return EXIT_FAILURE;
	}
	{
		vj->group    = "224.0.0.1";
		vj->port     = 7000;
	}


	progname = argv[0];
	if (NULL != strrchr(progname, '\\'))
		progname = strrchr(progname, '\\') + 1;
	if (NULL != strrchr(progname, '/'))
		progname = strrchr(progname, '/') + 1;
	parse_args(vj, argc - 1, argv + 1);


	socket_init();

	vj->sock = socket_udp4_bind(&vj->self_addr, vj->host, vj->port);
	if (SOCKET_INVAL == vj->sock) {
		print_options(vj);
		fprintf(stderr, "Could not bind to [%s:%d]\n",
			vj->host ? vj->host:"0.0.0.0", vj->port);
		exit(EXIT_FAILURE);
	}
	socket_recv_timeout_ms(vj->sock, 200);
	socket_settimetolive(vj->sock, 10);

	socket_addr_set_ipv4(&vj->group_addr, vj->group);
	socket_addr_set_port(&vj->group_addr, vj->port);
	s = socket_group_join(vj->sock, &vj->group_addr);
	if (-1 == s) {
		print_options(vj);
		fprintf(stderr, "Could not join to [%s:%d]\n",
			vj->group ? vj->group:"0.0.0.0", vj->port);
		exit(EXIT_FAILURE);
	}


	vj->queue_send = queue_create(sizeof(struct queue_message));


	print_options(vj);


	pthread_create(&thread_recv, NULL, recv_thread, vj);
	pthread_create(&thread_send, NULL, send_thread, vj);
	pthread_create(&thread_play, NULL, play_thread, vj);
	pthread_join(thread_recv, NULL);
	pthread_join(thread_send, NULL);
	pthread_join(thread_play, NULL);


	return EXIT_SUCCESS;
}