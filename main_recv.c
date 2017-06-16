#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "videojuego.h"


static void jugador_agregar(struct videojuego *vj, struct queue_message *qm)
{
	int i;
	pthread_mutex_lock(&vj->lock);

	if (JUGADORES <= vj->jugadores_len)
		return;

	for (i = 0; i < vj->jugadores_len; i++) {
		if (vj->jugadores[i].id != qm->mensaje.datos.jugador.id)
			continue;
		vj->jugadores[i].pelota.pos.x = qm->mensaje.datos.jugador.x;
		vj->jugadores[i].pelota.pos.y = qm->mensaje.datos.jugador.y;

		pthread_mutex_unlock(&vj->lock);
		return;
	}
	vj->jugadores[i].id           = qm->mensaje.datos.jugador.id;
	vj->jugadores[i].pelota.r     = vj->jugadores[0].pelota.r;
	vj->jugadores[i].pelota.pos.x = qm->mensaje.datos.jugador.x;
	vj->jugadores[i].pelota.pos.y = qm->mensaje.datos.jugador.y;
	vj->jugadores_len++;
	fprintf(stderr, "Player %d added\n", vj->jugadores[i].id);

	pthread_mutex_unlock(&vj->lock);
	return;
}


static void process_message(struct videojuego *vj, struct queue_message *qm)
{
	switch (qm->mensaje.tipo) {
	case MENSAJE_PING:
		/* send pong */
		break;

	case MENSAJE_PONG:
		/* update last ping */
		break;

	case MENSAJE_CONNECT:
		/* add player (in connecting) */
		break;

	case MENSAJE_READY:
		/* if player has connected, send all info */
		break;

	case MENSAJE_MAPAS:
		break;

	case MENSAJE_MONEDAS:
		break;

	case MENSAJE_POSICION:
		jugador_agregar(vj, qm);
		break;

		/* queue_enqueue(vj->queue_fs, op); */
	}

}


void* recv_thread(void *param)
{
	struct videojuego *vj = param;

	while (1) {
		struct queue_message  qm_alloc = {{0}};
		struct queue_message *qm = &qm_alloc;
		struct mensaje  m_alloc = {0};
		struct mensaje *m = &m_alloc;
		char phost[46] = {0};
		char shost[46] = {0};
		int  pport = 0;
		int  sport = 0;
		int s;

		s = socket_recvfrom(vj->sock, m, sizeof(*m), &vj->peer_addr);
		if (-1 == s && EAGAIN == errno)
			continue;
		if (s <= 0)
			continue;
		if (m->id == vj->id)
			continue;


		socket_addr_get_ipv4(&vj->peer_addr, phost, sizeof(phost));
		socket_addr_get_ipv4(&vj->self_addr, shost, sizeof(shost));
		socket_addr_get_port(&vj->peer_addr, &pport);
		socket_addr_get_port(&vj->self_addr, &sport);
		fprintf(stderr,
			"RECV [%s:%d] <- [%s:%d] recv %d bytes "
			"(0x%08x@%d)\n",
			shost, sport, phost, pport, s,
			m->tipo, m->tiempo);

		memcpy(&qm->mensaje, m, sizeof(*m));
		memcpy(&qm->addr, &vj->peer_addr, sizeof(vj->peer_addr));
		process_message(vj, qm);
	}

	return NULL;
}
