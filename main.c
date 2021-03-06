#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "videojuego.h"


const char *progname = "laberynth";

extern int32_t time_now_ms(void);


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
		vj->id       = (uint16_t)rand();
		vj->group    = "224.0.0.1";
		vj->port     = 7000;
		vj->tile_length = 25;
	}


	strcpy(vj->mapa[ 0], "  xxxxxxxxxxxxxxxxx ");
	strcpy(vj->mapa[ 1], "x xxxx        x   x ");
	strcpy(vj->mapa[ 2], "x   xx xxxxxx x x x ");
	strcpy(vj->mapa[ 3], "x           x x x x ");
	strcpy(vj->mapa[ 4], "x xxxxxxxxxxx x x x ");
	strcpy(vj->mapa[ 5], "x x           x x x ");
	strcpy(vj->mapa[ 6], "x x xxxxxxxxxxx x x ");
	strcpy(vj->mapa[ 7], "x x      x      x x ");
	strcpy(vj->mapa[ 8], "x xxxxxx x xxxxxx x ");
	strcpy(vj->mapa[ 9], "x          x      x ");
	strcpy(vj->mapa[10], "xxxxxxxxxxxx xxxxxx ");
	strcpy(vj->mapa[11], "x        x   x      ");
	strcpy(vj->mapa[12], "  xxxxxx x xxxxxx x ");
	strcpy(vj->mapa[13], "x xx   x x      x x ");
	strcpy(vj->mapa[14], "x xx x x xx x x x x ");
	strcpy(vj->mapa[15], "x xx x x  x x x   x ");
	strcpy(vj->mapa[16], "x xx x xx   x xxxxx ");
	strcpy(vj->mapa[17], "x xx x  xxx xxxxxxx ");
	strcpy(vj->mapa[18], "x xx xx    x        ");
	strcpy(vj->mapa[19], "xxx  xxxxxxxxxxxxxxx");


	vj->jugadores[0].id = (srand(time(NULL)), rand());
	vj->jugadores[0].choques = 0;
	vj->jugadores[0].puntos = 0;
	vj->jugadores[0].pelota.r     = vj->tile_length/4;
	vj->jugadores[0].pelota.pos.x = vj->tile_length/2;
	vj->jugadores[0].pelota.pos.y = vj->tile_length/2;
	vj->jugadores[0].pelota.dpos.x = 0;
	vj->jugadores[0].pelota.dpos.y = 0;
	vj->jugadores[0].ultimo_movimiento = time_now_ms();
	vj->jugadores_len++;


	vj->bg_color.hue   = 210.0;
	vj->bg_color.sat   = 80.0;
	vj->bg_color.light = 20.0;


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

	pthread_mutex_init(&vj->lock, NULL);

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
