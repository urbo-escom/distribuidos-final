#include <stdint.h>
#include <pthread.h>

#include "socket.h"
#include "queue.h"


enum mensaje_tipo {
	MENSAJE_PING     = 0,
	MENSAJE_PONG     = 1,
	MENSAJE_CONNECT  = 2,
	MENSAJE_READY    = 3,
	MENSAJE_MAPAS    = 5,
	MENSAJE_MONEDAS  = 6,
	MENSAJE_POSICION = 7
};


struct mensaje {
	uint8_t tipo;
	int32_t tiempo;
	union {
		char mapa[32][32];
		struct {
			uint8_t x;
			uint8_t y;
		} monedas[100];
		struct {
			uint8_t id;
			uint8_t x;
			uint8_t y;
		} jugador;
	} datos;
};


struct queue_message {
	struct mensaje     mensaje;
	struct socket_addr addr;
};


enum conexion_estado {
	CONEXION_SIN_CONECTAR = 0,
	CONEXION_CONECTANDO   = 1,
	CONEXION_MAPAS        = 2,
	CONEXION_MONEDAS      = 3,
	CONEXION_JUGANDO      = 4
};


extern void* recv_thread(void*);
extern void* send_thread(void*);
extern void* play_thread(void*);


struct videojuego {
	const char  *host;
	const char  *group;
	int          port;

	socket_fd          sock;
	struct socket_addr self_addr;
	struct socket_addr peer_addr;
	struct socket_addr group_addr;

	queue *queue_send;

	pthread_mutex_t lock;

	struct {
		struct socket_addr addr;
		int16_t            ultimo_ping;
		uint8_t            estado;
		uint8_t id;
		uint16_t x;
		uint16_t y;
	}      jugadores[16];
	size_t jugadores_len;

	struct {
		uint8_t x;
		uint8_t y;
	}      monedas[100];
	size_t monedas_len;
};
