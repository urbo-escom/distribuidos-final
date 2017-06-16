#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "gfx.h"


#define ANCHURA 600
#define ALTURA  600

#define TILE_X (25)
#define TILE_Y (25)

const char *mapa[] = {
	"xxxxxxxxxxxxxxxxxxxx",
	"                   x",
	"xxxxxxxxxxxxxxxxxx x",
	"xxxxxxxxxxxxxxxxxxxx",
	"x xxxxxxxxxxxxxxxxxx",
	"x                  x",
	"xxxxxxxxxxxxxxxxxx x",
	"x                  x",
	"x xxxxxxxxxxxxxxxxxx",
	"x                  x",
	"xxxxxxxxxxxxxxxxxx x",
	"x                  x",
	"x xxxxxxxxxxxxxxxxxx",
	"x                  x",
	"xxxxxxxxxxxxxxxxxx x",
	"x                  x",
	"x xxxxxxxxxxxxxxxxxx",
	"x                  x",
	"xxxxxxxxxxxxxxxxxxxx",
	NULL,
};


typedef struct {
	float x;
	float y;
	float vx;
	float vy;

	int r;
	int color[3];
} Pelota;

typedef struct {
    float x;
    float y;
    
    float vx;
    float vy;
    
    int r;
    int color[3];
}Agente;

void muevePelota(Pelota *pPelota, Agente* pJugador);
void checaColisiones(Pelota *pPelota, Agente* pAgente);
void mueveAgente(Agente *pJugador, bool tecla[6]);
void validaMovimientoAgente(Agente *pAgente);
void calculaDireccionPelota(Pelota *Ball, Agente *Player);

bool tecla[6];


void* play_thread(void *param)
{
	struct videojuego *vj = param;

	Pelota pelota = {ANCHURA/2, 10,0,0,10, {255,255,255}};
	Agente jugador = {ANCHURA/4,ALTURA,0,0,25, {87,1,2}};
	Pelota *pPelota;
	Agente *pJugador;
    int i;

	(void)vj;
    pPelota=&pelota;
    pJugador=&jugador;

    for(i=0;i<6;i++){
        tecla[i]=false;
    }
    gfx_open(ANCHURA, ALTURA, "Ejemplo Micro Juego GFX");
    while(1) {
	int x = 0;
	int y = 0;
	int i;
	int j;
        if(gfx_event_waiting2()) {

            gfx_keyPress(tecla);
            
            mueveAgente(pJugador,tecla);
            validaMovimientoAgente(pJugador);            
        }

        muevePelota(pPelota, pJugador);
        gfx_color(pelota.color[0],pelota.color[1],pelota.color[2]);
        gfx_fill_arc(pelota.x - pelota.r, pelota.y - pelota.r, 2*pelota.r, 2*pelota.r, 0, 360*64);

        gfx_color(jugador.color[0],jugador.color[1],jugador.color[2]);
        gfx_fill_arc(jugador.x - jugador.r, jugador.y - jugador.r, 2*jugador.r, 2*jugador.r, 0, 180*64);

	for (i = 0; NULL != mapa[i]; i++) {
		x = 0;
		for (j = 0; '\0' != mapa[i][j]; j++) {
			if (' ' != mapa[i][j]) {
        			gfx_color(10, 10, 10);
				gfx_fill_rectangle(x, y, TILE_X, TILE_Y);
			} else {
        			gfx_color(255, 255, 255);
				gfx_fill_rectangle(x, y, TILE_X, TILE_Y);
			}
			x += TILE_X;
		}
		y += TILE_Y;
	}
       
		gfx_flush();
        usleep(18000);
    }
}

void muevePelota(Pelota *pPelota, Agente* pAgente){
    checaColisiones(pPelota, pAgente);
    (pPelota->vy)+=.08;
    (pPelota->x) = (pPelota->x) + (pPelota->vx);
    (pPelota->y) = (pPelota->y) + (pPelota->vy);
    gfx_clear();
}

void checaColisiones(Pelota *pPelota, Agente* pAgente){
    float distContacto, agenteBolaDist;
    
    if((pPelota->x < pPelota->r) || (pPelota->x > (ANCHURA - pPelota->r)))
        pPelota->vx = -pPelota->vx;
    if(pPelota->y < pPelota->r)
        pPelota->vy = -pPelota->vy;
    if(pPelota->y >= (ALTURA - pPelota->r))
        pPelota->vy = -pPelota->vy;
    
    agenteBolaDist=sqrtf(powf(pAgente->y - pPelota->y,2)+powf(pAgente->x - pPelota->x,2));
    distContacto = pAgente->r + pPelota->r;
    if(agenteBolaDist < distContacto)
        calculaDireccionPelota(pPelota,pAgente);
}


void mueveAgente(Agente *pJugador, bool tecla[6]){

    if ( tecla[3] == 1 ){
        pJugador->x-=10;
    }
    if ( tecla[5] == 1 ){
        pJugador->x+=10;
    }
    gfx_flush();
}

void validaMovimientoAgente(Agente *pAgente){
    
    if((pAgente->x)>(ANCHURA-pAgente->r)){
        pAgente->x=(ANCHURA-pAgente->r);
    }
    if((pAgente->x)<(pAgente->r)){
        pAgente->x=(pAgente->r);
    }
    
}


void calculaDireccionPelota(Pelota *Bola, Agente *Jugador){ 
    float velocidadAbsoluta = 10;
    float pendienteEntreCentros;
    float difCentrosX,difCentrosY;
    float anguloEntreCentros;

    difCentrosX = Bola->x - Jugador->x;
    difCentrosY = Bola->y - Jugador->y;
    pendienteEntreCentros = difCentrosY / difCentrosX ;
    anguloEntreCentros = atanf(pendienteEntreCentros);
    if(difCentrosX < 0 && difCentrosY < 0)
        anguloEntreCentros += acosf(-1.0);
            
    Bola->vx = velocidadAbsoluta * cosf(anguloEntreCentros);
    Bola->vy = velocidadAbsoluta * sinf(anguloEntreCentros);
}
