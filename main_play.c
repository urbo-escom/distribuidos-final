#include <stdio.h>
#include <stdlib.h>

#include "videojuego.h"
#include "gfx_v3.h"

#define PIXEL_MOVE (3)


void jugador_update(struct videojuego *vj, struct jugador *j)
{
	(void)*vj;
	j->pelota.pos.x  += j->pelota.dpos.x;
	j->pelota.dpos.x *= 0.85;
	j->pelota.pos.y  += j->pelota.dpos.y;
	j->pelota.dpos.y *= 0.85;
}


int jugador_check_collision_tile(struct videojuego *vj, int i, int j, struct jugador *jj)
{
	int x0;
	int x1;
	int y0;
	int y1;
		x0 = j*vj->tile_length;
		y0 = i*vj->tile_length;
		x1 = x0 + vj->tile_length;
		y1 = y0 + vj->tile_length;

	fprintf(stderr, "[%d, %d] - [%d, %d, %d, %d]\n",
		jj->pelota.pos.x,
		jj->pelota.pos.y,
		x0, x1, y0, y1);

	if ((x0 <= jj->pelota.pos.x && jj->pelota.pos.x <= x1)
	&&  (y0 <= jj->pelota.pos.y && jj->pelota.pos.y <= y1))
		return -1;

	return 0;
}


int jugador_check_collision(struct videojuego *vj, struct jugador *jj)
{
	int i;
	int j;
	for (i = 0; i < MAPA_YLEN; i++) {
		for (j = 0; j < MAPA_XLEN; j++) {
			if (' ' == vj->mapa[i][j])
				continue;
			jugador_check_collision_tile(vj, i, j, jj);
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

	switch (k->value[0]) {
	case GFX_KEY_LEFT:  j->pelota.dpos.x -= PIXEL_MOVE; j->pelota.pos.x -= PIXEL_MOVE; break;
	case GFX_KEY_UP:    j->pelota.dpos.y -= PIXEL_MOVE; j->pelota.pos.y -= PIXEL_MOVE; break;
	case GFX_KEY_RIGHT: j->pelota.dpos.x += PIXEL_MOVE; j->pelota.pos.x += PIXEL_MOVE; break;
	case GFX_KEY_DOWN:  j->pelota.dpos.y += PIXEL_MOVE; j->pelota.pos.y += PIXEL_MOVE; break;
	}
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


		jugador_update(vj, &vj->jugadores[0]);
		if (-1 == jugador_check_collision(vj, &vj->jugadores[0])) {
			vj->bg_color.hue   = 330.0;
			vj->bg_color.sat   =  80.0;
			vj->bg_color.light =  60.0;
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
		gfx_color_hsl(0.0, 100.0, 60.0);
		gfx_fill_circle(
			vj->jugadores[0].pelota.pos.x,
			vj->jugadores[0].pelota.pos.y,
			vj->jugadores[0].pelota.r
		);

		gfx_draw();
		gfx_sleep_ms(16);
	}

}
