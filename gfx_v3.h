/*
 * A simple graphics library for CSE 20211
 * Originally created by Prof. Doug Thain.
 * Modified by Prof. Ramzi Bualuan
 * Further Modified by Patrick Myron and Anna McMahon and Ukranio Coronilla
 */


#if defined(__cplusplus)
extern "C" {
#endif
#ifndef GFX_H
#define GFX_H


void gfx_open(int w, int h, const char *title);
void gfx_title(const char *title);
int  gfx_screen_size(int *w, int *h);
void gfx_close();

void gfx_listen_close_event();
int  gfx_size(int *w, int *h);
void gfx_move(int x, int y);
void gfx_resize(int w, int h);


void gfx_color(int r, int g, int b);
void gfx_bg_color(int r, int g, int b);
void gfx_color_hsl(double h, double s, double l);
void gfx_bg_color_hsl(double h, double s, double l);
void gfx_hsl2rgb(int *r, int *g, int *b, double hue, double sat, double light);

void gfx_flush();
void gfx_sleep_ms(int ms);
int  gfx_elapsed_ms(void);

void gfx_clear();
void gfx_draw();

void gfx_txt(int x, int y, const char *text);

void gfx_point(int x, int y);

void gfx_line_width(int w);
void gfx_line(int x1, int y1, int x2, int y2);
void gfx_bezier2d(
	int x1, int y1,
	int cx1, int cy1,
	int x2, int y2);
void gfx_bezier3d(
	int x1, int y1,
	int cx1, int cy1,
	int cx2, int cy2,
	int x2, int y2);

void gfx_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void gfx_rect(int x, int y, int w, int h);
void gfx_circle(int cx, int cy, int r);
void gfx_ellipse(int cx, int cy, int rx, int ry);
void gfx_arc(int cx, int cy, int rx, int ry, double ang1, double ang2);

void gfx_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void gfx_fill_rect(int x, int y, int w, int h);
void gfx_fill_circle(int cx, int cy, int r);
void gfx_fill_ellipse(int cx, int cy, int rx, int ry);
void gfx_fill_arc(int cx, int cy, int rx, int ry, double ang1, double ang2);


enum gfx_event_type {
	GFX_EVENT_NONE = 0,
	GFX_EVENT_KEYDOWN,
	GFX_EVENT_KEYPRESS,
	GFX_EVENT_KEYUP,
	GFX_EVENT_MOUSEDOWN,
	GFX_EVENT_MOUSEMOVE,
	GFX_EVENT_MOUSEUP,
	GFX_EVENT_RESIZE,
	GFX_EVENT_CLOSE,
	GFX_EVENT_LAST
};

enum gfx_key_type {
	GFX_KEY_ASCII = 0,
	GFX_KEY_UTF8,
	GFX_KEY_CURSOR,
	GFX_KEY_MOD,
	GFX_KEY_UNKNOWN
};

enum gfx_key_cursor {
	GFX_KEY_LEFT,
	GFX_KEY_UP,
	GFX_KEY_RIGHT,
	GFX_KEY_DOWN,
	GFX_KEY_BEGIN,
	GFX_KEY_END,
	GFX_KEY_PAGE_UP,
	GFX_KEY_PAGE_DOWN,
	GFX_KEY_HOME
};

enum gfx_key_modifier {
	GFX_KEY_CTRL_L,
	GFX_KEY_CTRL_R,
	GFX_KEY_SHIFT_L,
	GFX_KEY_SHIFT_R,
	GFX_KEY_ALT_L,
	GFX_KEY_ALT_R,
	GFX_KEY_CAPS_LOCK,
	GFX_KEY_SHIFT_LOCK
};

enum gfx_key_modifier_type {
	GFX_MOD_CTRL  = 0x01,
	GFX_MOD_SHIFT = 0x02,
	GFX_MOD_ALT   = 0x04,
	GFX_MOD_LOCK  = 0x08
};

enum gfx_mouse_type {
	GFX_MOUSE_LEFT  = 0x01,
	GFX_MOUSE_RIGHT = 0x02
};

struct gfx_event {
	enum gfx_event_type type;
	int                 time_ms;
	int                 modifier;
	union {
		struct gfx_event_key {
			enum gfx_key_type type;
			int               keycode;
			char              value[4];
			int               len;
			int               filter;
		} key;
		struct gfx_event_mouse {
			enum gfx_mouse_type button;
			int                 x;
			int                 y;
		} mouse;
		struct gfx_event_resize {
			int x;
			int y;
		} resize;
	} data;
};


int  gfx_pending_events();
void gfx_next_event(struct gfx_event *event);


#endif
#if defined(__cplusplus)
}
#endif
