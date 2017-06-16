#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include "videojuego.h"
#include "gfx_v3.h"

#define PIXEL_SPEED    (3)
#define PIXEL_DISTANCE (1)




int32_t time_now_ms(void)
{
	struct timespec spec;
	long            ms;
	time_t          s;

	clock_gettime(CLOCK_REALTIME, &spec);

	s  = spec.tv_sec;
	ms = round(spec.tv_nsec/1.0e6);

	return s*1000 + ms;
}


void update_jugadores(struct videojuego *vj)
{
	struct jugador *j;
	int i;

	pthread_mutex_lock(&vj->lock);
	for (i = 0; i < vj->jugadores_len; i++) {
		j = &vj->jugadores[i];
		j->pelota.pos.x  += j->pelota.dpos.x;
		j->pelota.dpos.x *= 0.95;
		j->pelota.pos.y  += j->pelota.dpos.y;
		j->pelota.dpos.y *= 0.95;
	}
	pthread_mutex_unlock(&vj->lock);
}


int jugador_check_collision_tile(struct videojuego *vj, int i, int j, struct jugador *jj)
{
	int x0;
	int x1;
	int y0;
	int y1;
	int a0;
	int a1;
	int b0;
	int b1;
	int s = 0;

	x0 = j*vj->tile_length;
	y0 = i*vj->tile_length;
	x1 = x0 + vj->tile_length;
	y1 = y0 + vj->tile_length;
	a0 = jj->pelota.pos.x - jj->pelota.r;
	a1 = jj->pelota.pos.x + jj->pelota.r;
	b0 = jj->pelota.pos.y - jj->pelota.r;
	b1 = jj->pelota.pos.y + jj->pelota.r;

	if (0)
		;

	else if ((x0 < a0 && a1 < x1)
	&&       (y0 < b0 && b1 < y1))
		s = -1;

	else if ((a0 < x0 && x0 < a1)
	&&       (b0 < y0 && y0 < b1))
		s = -1;

	else if ((a0 < x0 && x0 < a1)
	&&       (b0 < y1 && y1 < b1))
		s = -1;

	else if ((a0 < x1 && x1 < a1)
	&&       (b0 < y1 && y1 < b1))
		s = -1;

	else if ((a0 < x1 && x1 < a1)
	&&       (b0 < y0 && y0 < b1))
		s = -1;

	else if ((x0 < a0 && a1 < x1)
	&&  ((b0 < y0 && y0 < b1) || (b0 < y1 && y1 < b1)))
		s = -1;

	else if ((y0 < b0 && b1 < y1)
	&&  ((a0 < x0 && x0 < a1) || (a0 < x1 && x1 < a1)))
		s = -1;

	return s;
}


int jugador_check_collision(struct videojuego *vj, struct jugador *jj)
{
	int i;
	int j;
	int s;
	for (i = 0; i < MAPA_YLEN; i++) {
		for (j = 0; j < MAPA_XLEN; j++) {
			if (' ' == vj->mapa[i][j])
				continue;
			s = jugador_check_collision_tile(vj, i, j, jj);
			if (-1 == s)
				return -1;
		}
	}
	return 0;
}


void (*handler[GFX_EVENT_LAST])(struct videojuego *vj, struct gfx_event *e);

void handle_keypress(struct videojuego *vj, struct gfx_event*);

void init_handlers()
{
	handler[GFX_EVENT_KEYPRESS] = handle_keypress;
}


void handle_keypress(struct videojuego *vj, struct gfx_event *e)
{
	struct jugador *j = &vj->jugadores[0];
	struct gfx_event_key *k;
	k = &e->data.key;

	if (GFX_KEY_CURSOR != k->type)
		return;

	pthread_mutex_lock(&vj->lock);
	switch (k->value[0]) {
	case GFX_KEY_LEFT:  j->pelota.dpos.x -= PIXEL_SPEED; j->pelota.pos.x -= PIXEL_DISTANCE; break;
	case GFX_KEY_UP:    j->pelota.dpos.y -= PIXEL_SPEED; j->pelota.pos.y -= PIXEL_DISTANCE; break;
	case GFX_KEY_RIGHT: j->pelota.dpos.x += PIXEL_SPEED; j->pelota.pos.x += PIXEL_DISTANCE; break;
	case GFX_KEY_DOWN:  j->pelota.dpos.y += PIXEL_SPEED; j->pelota.pos.y += PIXEL_DISTANCE; break;
	}
	pthread_mutex_unlock(&vj->lock);
}


void* play_thread(void *param)
{
	struct videojuego *vj = param;

	vj->width  = MAPA_XLEN*vj->tile_length;
	vj->height = MAPA_YLEN*vj->tile_length;

	gfx_open(vj->width, vj->height, "Videojuego");
	gfx_bg_color_hsl(30.0, 100.0, 10.0);
	gfx_color_hsl(30.0, 90.0, 80.0);
	init_handlers();


	while (1) {
		struct queue_message  qm_alloc = {{0}};
		struct queue_message *qm = &qm_alloc;
		int x = 0;
		int y = 0;
		int i;
		int j;


		if (gfx_pending_events()) {
			struct gfx_event e = {0};
			gfx_next_event(&e);
			if (handler[e.type])
				handler[e.type](vj, &e);
		}
		gfx_clear();


		update_jugadores(vj);

		if (-1 == jugador_check_collision(vj, &vj->jugadores[0])) {
			vj->bg_color.hue   = 330.0;
			vj->bg_color.sat   =  80.0;
			vj->bg_color.light =  60.0;
			vj->jugadores[0].pelota.r     = vj->tile_length/4;
			vj->jugadores[0].pelota.pos.x = vj->tile_length/2;
			vj->jugadores[0].pelota.pos.y = vj->tile_length/2;
			vj->jugadores[0].pelota.dpos.x = 0;
			vj->jugadores[0].pelota.dpos.y = 0;
		} else {
			vj->bg_color.hue   = 210.0;
			vj->bg_color.sat   = 80.0;
			vj->bg_color.light = 20.0;
		}

		for (i = 0; i < MAPA_YLEN; i++) {
			x = 0;
			for (j = 0; j < MAPA_XLEN; j++) {
				if (' ' != vj->mapa[i][j]) {
					gfx_color_hsl(
						vj->bg_color.hue,
						vj->bg_color.sat,
						vj->bg_color.light
					);
					gfx_fill_rect(x, y, vj->tile_length, vj->tile_length);
				} else {
					gfx_color_hsl(60.0, 90.0, 80.0);
					gfx_fill_rect(x, y, vj->tile_length, vj->tile_length);
				}
				x += vj->tile_length;
			}
			y += vj->tile_length;
		}

		pthread_mutex_lock(&vj->lock);
		for (i = 0; i < vj->jugadores_len; i++) {
			fprintf(stderr, "JUG [%02d:0x%08x]\n", i, vj->jugadores[i].id);
			gfx_color_hsl(0.0, 100.0, 60.0);
			gfx_fill_rect(
				vj->jugadores[i].pelota.pos.x - vj->jugadores[i].pelota.r/2,
				vj->jugadores[i].pelota.pos.y - vj->jugadores[i].pelota.r/2,
				2*vj->jugadores[i].pelota.r,
				2*vj->jugadores[i].pelota.r
			);
		}
		pthread_mutex_unlock(&vj->lock);


		qm->mensaje.tipo = MENSAJE_POSICION;
		qm->mensaje.tiempo = time_now_ms();
		qm->mensaje.datos.jugador.id = vj->jugadores[0].id;
		qm->mensaje.datos.jugador.x  = vj->jugadores[0].pelota.pos.x;
		qm->mensaje.datos.jugador.y  = vj->jugadores[0].pelota.pos.y;
		memcpy(&qm->addr, &vj->group_addr, sizeof(qm->addr));
		queue_enqueue(vj->queue_send, qm);

		gfx_draw();
		gfx_sleep_ms(16);
	}

}
