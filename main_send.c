#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "videojuego.h"


void* send_thread(void *param)
{
	struct videojuego *vj = param;

	while (1) {
		struct queue_message  qm_alloc = {{0}};
		struct queue_message *qm = &qm_alloc;
		char phost[46] = {0};
		char shost[46] = {0};
		int  pport = 0;
		int  sport = 0;
		int s;


		s = queue_dequeue(vj->queue_send, qm);
		if (-1 == s)
			return NULL;


		socket_addr_get_ipv4(&vj->group_addr, phost, sizeof(phost));
		socket_addr_get_ipv4(&vj->self_addr, shost, sizeof(shost));
		socket_addr_get_port(&vj->group_addr, &pport);
		socket_addr_get_port(&vj->self_addr, &sport);
		fprintf(stderr,
			"SEND [%s:%d] -> [%s:%d] sent "
			"(0x%08x@%d)\n",
			shost, sport, phost, pport,
			qm->mensaje.tipo, qm->mensaje.tiempo);
	}

	return NULL;
}
