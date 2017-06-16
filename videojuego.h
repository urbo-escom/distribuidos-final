#include <stdint.h>
#include <pthread.h>

#include "socket.h"
#include "queue.h"


#ifndef MAPA_XLEN
#define MAPA_XLEN (20)
#endif
#ifndef MAPA_YLEN
#define MAPA_YLEN (20)
#endif

#ifndef JUGADORES
#define JUGADORES (100)
#endif


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
	uint16_t id;
	uint8_t  tipo;
	int32_t  tiempo;
	union {
		char mapa[MAPA_YLEN][MAPA_XLEN];
		struct {
			uint8_t  id;
			uint16_t x;
			uint16_t y;
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


struct pos {
	int x;
	int y;
};


struct pelota {
	struct pos pos;
	struct pos dpos;
	struct pos ddpos;
	int        r;
};


struct jugador {
	uint8_t            id;
	struct socket_addr addr;
	int                ultimo_ping;
	uint8_t            estado;
	struct pelota      pelota;
};


struct videojuego {
	uint8_t      id;
	const char  *host;
	const char  *group;
	int          port;

	socket_fd          sock;
	struct socket_addr self_addr;
	struct socket_addr peer_addr;
	struct socket_addr group_addr;

	queue *queue_send;


	int width;
	int height;
	int tile_length;

	char mapa[MAPA_YLEN][MAPA_XLEN + 1];
	struct {
		int hue;
		int sat;
		int light;
	} bg_color;

	pthread_mutex_t lock;
	struct jugador  jugadores[JUGADORES];
	size_t          jugadores_len;
};
