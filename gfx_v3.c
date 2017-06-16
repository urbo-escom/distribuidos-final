/*
 * A simple graphics library for CSE 20211
 * Originally created by Prof. Doug Thain.
 * Modified by Prof. Ramzi Bualuan
 * Further Modified by Patrick Myron and Anna McMahon and Ukranio Coronilla
 */

#include "gfx_v3.h"

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>


static int die_line;
void die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "[%s:%d] ", "gfx.c", die_line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}
#define die(args) \
	do { \
		die_line = __LINE__; \
		die args ; \
	} while (0)


struct xwindow {
	Display              *dpy;
	Window                win;
	int                   scr;
	Visual               *vis;
	Drawable              buf;
	GC                    gc;
	Colormap              cmap;
	int                   cmap_fast;
	XColor                bg_color;
	XColor                fg_color;
	XIM                   im;
	XIC                   ic;
	XSetWindowAttributes  attr;
	Atom                  delete_msg;
	int                   offx;
	int                   offy;
	int                   w;
	int                   h;
};


struct {
	int utf8;
	int center;
	int buffer;
	int bezier2d_segments;
	int bezier3d_segments;
#ifndef NDEBUG
	int debug;
#endif
} options;


#ifndef NDEBUG

#define DEBUG_KEYDOWN   (0x01)
#define DEBUG_KEYUP     (0x02)
#define DEBUG_KEYPRESS  (0x04)
#define DEBUG_KEY       (DEBUG_KEYDOWN | DEBUG_KEYUP | DEBUG_KEYPRESS)
#define DEBUG_MOUSEDOWN (0x08)
#define DEBUG_MOUSEUP   (0x10)
#define DEBUG_MOUSEMOVE (0x20)
#define DEBUG_MOUSE     (DEBUG_MOUSEDOWN | DEBUG_MOUSEUP | DEBUG_MOUSEMOVE)
#define DEBUG_RESIZE    (0x40)
#define DEBUG_ALL       (0xff)

static int debug_line;
static void debug_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "[%s:%d] ", "gfx.c", debug_line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#define dprintf(args) \
	do { \
		if (options.debug) { \
			debug_line = __LINE__; \
			debug_printf args ; \
		} \
	} while (0)

#define dtprintf(t, args) \
	do { \
		if (options.debug & DEBUG_##t) { \
			debug_line = __LINE__; \
			debug_printf args ; \
		} \
	} while (0)

#else /* !NDEBUG */
#define dprintf(args)
#define dtprintf(t, args)
#endif


static struct xwindow xw;

static void time_init(void);
static void time_elapsed_ms(struct gfx_event *e);


static void xkey_press(XEvent*, struct gfx_event*);
static void xmapping(XEvent*, struct gfx_event*);
static void xmouse_press(XEvent*, struct gfx_event*);
static void xmouse_release(XEvent*, struct gfx_event*);
static void xmouse_move(XEvent*, struct gfx_event*);
static void xresize(XEvent*, struct gfx_event*);
static void xclose(XEvent*, struct gfx_event*);

static void (*xhandler[LASTEvent])(XEvent*, struct gfx_event*);

static void init_handlers()
{
	xhandler[KeyPress]        = xkey_press;
	xhandler[KeyRelease]      = xkey_press;
	xhandler[MappingNotify]   = xmapping;
	xhandler[ButtonPress]     = xmouse_press;
	xhandler[ButtonRelease]   = xmouse_release;
	xhandler[MotionNotify]    = xmouse_move;
	xhandler[ConfigureNotify] = xresize;
	xhandler[ClientMessage]   = xclose;
}

static int xhandler_mask = 0
		| StructureNotifyMask
		| KeyPressMask
		| KeyReleaseMask
		| ButtonPressMask
		| ButtonReleaseMask
		| PointerMotionMask
;


void parse_env(int w, int h)
{
	memset(&options, 0, sizeof(options));

	options.utf8   = (NULL == getenv("GFX_DISABLE_UTF8"));
	options.center = (NULL != getenv("GFX_CENTER_WINDOW"));
	options.buffer = (NULL == getenv("GFX_DISABLE_BUFFER"));
	options.bezier2d_segments =
		NULL == getenv("GFX_BEZIER_2D_SEGMENTS") ? 30:
		(int)strtoul(getenv("GFX_BEZIER_2D_SEGMENTS"), NULL, 0);
	options.bezier3d_segments =
		NULL == getenv("GFX_BEZIER_3D_SEGMENTS") ? 30:
		(int)strtoul(getenv("GFX_BEZIER_3D_SEGMENTS"), NULL, 0);

	if (options.bezier2d_segments <= 0 || 1000 < options.bezier2d_segments)
		options.bezier2d_segments = 30;

	if (options.bezier3d_segments <= 0 || 1000 < options.bezier3d_segments)
		options.bezier3d_segments = 30;

#ifndef NDEBUG
#define HAS(s) s*(NULL != getenv("GFX_" #s))
	options.debug |= HAS(DEBUG_KEYDOWN);
	options.debug |= HAS(DEBUG_KEYUP);
	options.debug |= HAS(DEBUG_KEYPRESS);
	options.debug |= HAS(DEBUG_KEY);
	options.debug |= HAS(DEBUG_MOUSEDOWN);
	options.debug |= HAS(DEBUG_MOUSEUP);
	options.debug |= HAS(DEBUG_MOUSEMOVE);
	options.debug |= HAS(DEBUG_MOUSE);
	options.debug |= HAS(DEBUG_RESIZE);
	options.debug |= HAS(DEBUG_ALL);
	options.debug |= DEBUG_ALL*(NULL != getenv("GFX_DEBUG"));
#undef HAS
	dprintf(("Debug 0x%02x\n", options.debug));
#endif

	if (options.center) {
		int r[2];
		dprintf(("Calculating position for centering\n"));
		if (0 == gfx_screen_size(&r[0], &r[1])) {
			xw.offx = r[0]/2 - w/2;
			xw.offy = r[1]/2 - h/2;
			dprintf(("Centering position is (%d+%d, %d+%d) "
				"from screen (%d, %d)\n",
				xw.offx, w,
				xw.offy, h,
				r[0], r[1]));
		}
	}
}


static struct timespec time_start;

static void time_init(void)
{
	char *err;
	int   errnum;
	if (-1 != clock_gettime(CLOCK_MONOTONIC, &time_start))
		return;
	err = strerror(errnum = errno);
	die(("Can't start monotonic time: %d %s\n", errnum, err));
}

static void time_elapsed_ms(struct gfx_event *e)
{
	struct timespec now;
	char *err;
	int   errnum;
	if (-1 != clock_gettime(CLOCK_MONOTONIC, &now)) {
		e->time_ms =
			(now.tv_sec - time_start.tv_sec)*1E3 +
			(now.tv_nsec - time_start.tv_nsec)/1E6;
		return;
	}
	err = strerror(errnum = errno);
	die(("Can't get monotonic time: %d %s\n", errnum, err));
}


static void init_colormap(void);
static void init_window(void);
static void init_graphics(void);
static void init_input(void);

void gfx_open(int w, int h, const char *title)
{
	dprintf(("Parsing env vars\n"));
	parse_env(w, h);

	dprintf(("Opening display\n"));
	xw.dpy = XOpenDisplay(NULL);
	if (!xw.dpy)
		die(("Can't open display\n"));
	xw.scr = DefaultScreen(xw.dpy);
	xw.vis = DefaultVisual(xw.dpy, xw.scr);
	xw.w = w;
	xw.h = h;

	time_init();
	init_handlers();
	init_colormap();
	init_window();
	init_graphics();
	init_input();
	gfx_title(title);

	/*
	 * XDefineCursor(xw.dpy, xw.win, XCreateFontCursor(xw.dpy,
	 * XC_sb_left_arrow));
	 */
	dprintf(("Mapping window\n"));
	XMapWindow(xw.dpy, xw.win);

	xw.delete_msg = None;

	for(;;) {
		XEvent e;
		XNextEvent(xw.dpy, &e);
		if (e.type == MapNotify) {
			dprintf(("MapNotify event received\n"));
			break;
		}
	}
	if (options.center) {
		dprintf(("Centering window\n"));
		XMoveWindow(xw.dpy, xw.win, xw.offx, xw.offy);
	}
}


static void init_colormap()
{
	dprintf(("Setting colormap\n"));
	xw.cmap = DefaultColormap(xw.dpy, 0);
	if (xw.vis && xw.vis->class == TrueColor)
		xw.cmap_fast = 1;
	else
		xw.cmap_fast = 0;
}


static void fill_color(XColor *color, int r, int g, int b)
{
	if (xw.cmap_fast) {
		color->pixel = 0;
		color->pixel |= (b&0xff) <<  0;
		color->pixel |= (g&0xff) <<  8;
		color->pixel |= (r&0xff) << 16;
	} else {
		color->pixel = 0;
		color->red   = r << 8;
		color->green = g << 8;
		color->blue  = b << 8;
		XAllocColor(xw.dpy, xw.cmap, color);
	}
}


static void init_window()
{
	dprintf(("Creating window [%d, %d, %d, %d]\n",
		xw.offx, xw.offy, xw.w, xw.h));
	xw.attr.background_pixel = BlackPixel(xw.dpy, xw.scr);
	xw.attr.border_pixel     = BlackPixel(xw.dpy, xw.scr);
	xw.attr.colormap         = xw.cmap;
	xw.attr.backing_store    = Always;
	xw.attr.event_mask       = xhandler_mask;
	xw.win = XCreateWindow(
		xw.dpy,
		DefaultRootWindow(xw.dpy),
		xw.offx, xw.offy, xw.w, xw.h,
		0, DefaultDepth(xw.dpy, xw.scr),
		InputOutput, xw.vis, 0
		| CWBackPixel
		| CWBorderPixel
		| CWEventMask
		| CWColormap
		| CWBackingStore
		, &xw.attr
	);
	dprintf(("Window id is %d\n", xw.win));
}


static void init_graphics()
{
	XGCValues gc_attr;

	dprintf(("Creating graphics context\n"));

	/*
	 * Disable GraphicsExpose and NoExpose events.
	 * A lot of these events will be generated by gfx_draw
	 * when double buffering, thereby slowing down the event
	 * processing loop.
	 */
	gc_attr.graphics_exposures = False;

	xw.gc = XCreateGC(xw.dpy, xw.win, GCGraphicsExposures, &gc_attr);
	xw.bg_color.pixel = BlackPixel(xw.dpy, xw.scr);
	xw.fg_color.pixel = WhitePixel(xw.dpy, xw.scr);
	XSetBackground(xw.dpy, xw.gc, xw.bg_color.pixel);
	XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);

	if (options.buffer) {
		dprintf(("Creating back buffer\n"));
		xw.buf = XCreatePixmap(xw.dpy, xw.win,
			xw.w, xw.h,
			DefaultDepth(xw.dpy, xw.scr)
		);
		XSetForeground(xw.dpy, xw.gc, xw.bg_color.pixel);
		XFillRectangle(xw.dpy, xw.buf, xw.gc, 0, 0, xw.w, xw.h);
		XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);
	} else {
		dprintf(("No back buffer\n"));
		xw.buf = xw.win;
	}
}


static void init_input()
{
	if (!options.utf8) {
		dprintf(("UTF-8 is disabled\n"));
		return;
	}
	dprintf(("Creating Input Method and Context\n"));

	if (!XSupportsLocale())
		dprintf(("X11 doesn't support current locale\n"));

	while (1) {
		xw.im = XOpenIM(xw.dpy, NULL, NULL, NULL);
		if (NULL != xw.im)
			break;

		dprintf(("IM denied 1 time\n"));
		XSetLocaleModifiers("@im=local");
		xw.im = XOpenIM(xw.dpy, NULL, NULL, NULL);
		if (NULL != xw.im)
			break;

		dprintf(("IM denied 2 times\n"));
		XSetLocaleModifiers("@im=");
		xw.im = XOpenIM(xw.dpy, NULL, NULL, NULL);
		if (NULL != xw.im)
			break;

		dprintf(("IM denied 3 times\n"));
		break;
	}

	if (NULL != xw.im) {
		xw.ic = XCreateIC(xw.im, XNInputStyle, 0
			| XIMPreeditNothing
			| XIMStatusNothing,
			XNClientWindow, xw.win,
			NULL
		);
	}

	if (NULL == xw.ic)
		dprintf(("Could not obtain input method and context\n"));
}


void gfx_title(const char *title)
{
#ifdef X_HAVE_UTF8_STRING
	XTextProperty prop;
	char  p[1024];
	char *q;

	if (!title || !title[0])
		return;

	assert(strlen(title) < sizeof(p));
	q = strcpy(p, title);

	Xutf8TextListToTextProperty(xw.dpy, &q, 1,
		XUTF8StringStyle, &prop);
	XSetWMName(xw.dpy, xw.win, &prop);
#else
	if (!title || !title[0])
		return;
	XStoreName(xw.dpy, xw.win, title);
#endif
}


int gfx_screen_size(int *w, int *h)
{
	Display *d = NULL;
	Status   ret;
	Window            root = 0;
	XWindowAttributes root_attr;

	d = XOpenDisplay(NULL);
	if (!d)
		return -1;
	root = DefaultRootWindow(d);

	ret = XGetWindowAttributes(d, root, &root_attr);
	if (0 == ret)
		return -2;

	*w = root_attr.width;
	*h = root_attr.height;

	XCloseDisplay(d);
	return 0;
}


void gfx_close()
{
	if (NULL != xw.ic) {
		dprintf(("Closing input context\n"));
		XDestroyIC(xw.ic);
	}
	if (NULL != xw.im) {
		dprintf(("Closing input method\n"));
		XCloseIM(xw.im);
	}
	dprintf(("Closing display and associated resources\n"));
	XCloseDisplay(xw.dpy);
}


void gfx_listen_close_event()
{
	dprintf(("Trying to listen to close events\n"));
	xw.delete_msg = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
	if (!XSetWMProtocols(xw.dpy, xw.win, &xw.delete_msg, 1))
		dprintf(("Cannot listen to close events\n"));
	else
		dprintf(("Listening to close events on gfx_next_event()\n"));
}


int gfx_size(int *w, int *h)
{
	Status s;
	XWindowAttributes attr;
	s = XGetWindowAttributes(xw.dpy, xw.win, &attr);
	if (0 == s)
		return -1;
	*w = attr.width;
	*h = attr.height;
	return 0;
}


void gfx_move(int x, int y)
{
	XMoveWindow(xw.dpy, xw.win, xw.offx = x, xw.offy = y);
}


void gfx_resize(int w, int h)
{
	XResizeWindow(xw.dpy, xw.win, xw.w = w, xw.h = h);
	if (!options.buffer)
		return;
	XFreePixmap(xw.dpy, xw.buf);
	xw.buf = XCreatePixmap(xw.dpy, xw.win,
		xw.w, xw.h,
		DefaultDepth(xw.dpy, xw.scr)
	);
	XSetForeground(xw.dpy, xw.gc, xw.bg_color.pixel);
	XFillRectangle(xw.dpy, xw.buf, xw.gc, 0, 0, xw.w, xw.h);
	XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);
}


void gfx_color(int r, int g, int b)
{
	fill_color(&xw.fg_color, r, g, b);
	XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);
}


void gfx_bg_color(int r, int g, int b)
{
	fill_color(&xw.bg_color, r, g, b);
	xw.attr.background_pixel = xw.bg_color.pixel;
	XChangeWindowAttributes(xw.dpy, xw.win, CWBackPixel, &xw.attr);
	XSetBackground(xw.dpy, xw.gc, xw.bg_color.pixel);
}


void gfx_color_hsl(double h, double s, double l)
{
	int r;
	int g;
	int b;
	gfx_hsl2rgb(&r, &g, &b, h, s, l);
	gfx_color(r, g, b);
}


void gfx_bg_color_hsl(double h, double s, double l)
{
	int r;
	int g;
	int b;
	gfx_hsl2rgb(&r, &g, &b, h, s, l);
	gfx_bg_color(r, g, b);
}


/* https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSL */
void gfx_hsl2rgb(int *r, int *g, int *b, double hue, double sat, double light)
{
	double rr = 0.0;
	double gg = 0.0;
	double bb = 0.0;
	double c;
	double hprime;
	double x;
	double m;

	sat   /= 100.0;
	light /= 100.0;

	if (hue < 0.0 || 360.0 < hue)
		hue -= 360.0*floor(hue/360.0);

	if (sat < 0.0) sat = 0.0;
	if (1.0 < sat) sat = 1.0;

	if (light < 0.0) light = 0.0;
	if (1.0 < light) light = 1.0;

	c = (1 - fabs(2.0*light - 1.0))*sat;
	hprime = hue/60.0;
	x = c*(1 - fabs(fmod(hprime, 2) - 1));

	if (0)
		;
	else if (0.0 <= hprime && hprime <= 1.0) rr =   c, gg =   x, bb = 0.0;
	else if (1.0 <= hprime && hprime <= 2.0) rr =   x, gg =   c, bb = 0.0;
	else if (2.0 <= hprime && hprime <= 3.0) rr = 0.0, gg =   c, bb =   x;
	else if (3.0 <= hprime && hprime <= 4.0) rr = 0.0, gg =   x, bb =   c;
	else if (4.0 <= hprime && hprime <= 5.0) rr =   x, gg = 0.0, bb =   c;
	else if (5.0 <= hprime && hprime <= 6.0) rr =   c, gg = 0.0, bb =   x;

	m = light - 0.5*c;
	*r = 255*(rr + m);
	*g = 255*(gg + m);
	*b = 255*(bb + m);
}


void gfx_flush()
{
	XFlush(xw.dpy);
}


void gfx_sleep_ms(int ms)
{
	usleep(ms*1000);
}


int gfx_elapsed_ms()
{
	struct gfx_event e;
	time_elapsed_ms(&e);
	return e.time_ms;
}


void gfx_clear()
{
	if (options.buffer) {
		XSetForeground(xw.dpy, xw.gc, xw.bg_color.pixel);
		XFillRectangle(xw.dpy, xw.buf, xw.gc, 0, 0, xw.w, xw.h);
		XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);
	} else {
		XClearWindow(xw.dpy, xw.win);
	}
}


void gfx_draw()
{
	if (options.buffer)
		XCopyArea(xw.dpy, xw.buf, xw.win, xw.gc,
			0, 0, xw.w, xw.h, 0, 0);
	gfx_flush();
}


/* https://en.wikipedia.org/wiki/UTF-8 */
void gfx_txt(int x, int y, const char *text)
{
	XChar2b *mtxt;
	size_t   len;
	size_t   i;
	size_t   j;

	len = strlen(text);
	if (!len)
		return;

	mtxt = malloc(len*sizeof(*mtxt));
	if (NULL == mtxt)
		die(("malloc failed\n"));

	for (i = j = 0; i < len && j < len; i++) {
		unsigned char *u8 = (unsigned char*)text;

		/* 0xxx yyyy -> 0000 0000 0xxx yyyy */
		if (u8[i] < 0x80) {
			mtxt[j].byte1 = 0;
			mtxt[j].byte2 = u8[i];
			j++;
			continue;
		}

		/* 110x xxxx 10yy yyyy -> 0000 0xxx xxyy yyyy */
		if (0xc0 == (u8[i] & 0xc0)) {
			if (len <= i + 1)
				break;
			mtxt[j].byte1  = (u8[i + 0] & 0x1c) >> 2;
			mtxt[j].byte2  = (u8[i + 0] & 0x03) << 6;
			mtxt[j].byte2 |= (u8[i + 1] & 0x3f) >> 0;
			j++;
			continue;
		}

		/* 1110 xxxx 10yy yyyy 10zz zzzz -> xxxx yyyy yyzz zzzz */
		if (0xe0 == (u8[i] & 0xe0)) {
			if (len <= i + 2)
				break;
			mtxt[j].byte1  = (u8[i + 0] & 0x0f) << 4;
			mtxt[j].byte1 |= (u8[i + 1] & 0x3c) >> 2;
			mtxt[j].byte2  = (u8[i + 1] & 0x03) << 6;
			mtxt[j].byte2 |= (u8[i + 2] & 0x3f) >> 0;
			j++;
			continue;
		}
	}

	XDrawString16(xw.dpy, xw.buf, xw.gc, x, y, mtxt, j);
	free(mtxt);
}


void gfx_point(int x, int y)
{
	XDrawPoint(xw.dpy, xw.buf, xw.gc, x, y);
}


void gfx_line_width(int w)
{
	XSetLineAttributes(xw.dpy, xw.gc, w <= 0 ? 1:w, 0, 0, 0);
}


void gfx_line(int x1, int y1, int x2, int y2)
{
	XDrawLine(xw.dpy, xw.buf, xw.gc, x1, y1, x2, y2);
}


/* https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Specific_cases */
static void fbezier2d(int *x, int *y,
		double t,
		int x1, int y1,
		int x2, int y2,
		int x3, int y3)
{
	double a = (1 - t)*(1 - t);
	double b = 2*(1 - t)*t;
	double c = t*t;
	*x = a*x1 + b*x2 + c*x3;
	*y = a*y1 + b*y2 + c*y3;
}

void gfx_bezier2d(
		int x1, int y1,
		int cx1, int cy1,
		int x2, int y2)
{
	int xa;
	int ya;
	int xb;
	int yb;
	int i;
	double t = 0.0;

	fbezier2d(&xa, &ya, t, x1, y1, cx1, cy1, x2, y2);
	for (i = 1; i <= options.bezier2d_segments; i++) {
		t = i/(double)options.bezier2d_segments;
		fbezier2d(&xb, &yb, t, x1, y1, cx1, cy1, x2, y2);
		gfx_line(xa, ya, xb, yb);
		xa = xb;
		ya = yb;
	}
}


/* https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Specific_cases */
static void fbezier3d(int *x, int *y,
		double t,
		int x1, int y1,
		int x2, int y2,
		int x3, int y3,
		int x4, int y4)
{
	double a = (1 - t)*(1 - t)*(1 - t);
	double b = 3*(1 - t)*(1 - t)*t;
	double c = 3*(1 - t)*t*t;
	double d = t*t*t;
	*x = a*x1 + b*x2 + c*x3 + d*x4;
	*y = a*y1 + b*y2 + c*y3 + d*y4;
}

void gfx_bezier3d(
		int x1, int y1,
		int cx1, int cy1,
		int cx2, int cy2,
		int x2, int y2)
{
	int xa;
	int ya;
	int xb;
	int yb;
	int i;
	double t = 0.0;

	fbezier3d(&xa, &ya, t, x1, y1, cx1, cy1, cx2, cy2, x2, y2);
	for (i = 1; i <= options.bezier3d_segments; i++) {
		t = i/(double)options.bezier3d_segments;
		fbezier3d(&xb, &yb, t, x1, y1, cx1, cy1, cx2, cy2, x2, y2);
		gfx_line(xa, ya, xb, yb);
		xa = xb;
		ya = yb;
	}
}


void gfx_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	gfx_line(x1, y1, x2, y2);
	gfx_line(x2, y2, x3, y3);
	gfx_line(x3, y3, x1, y1);
}


void gfx_rect(int x, int y, int w, int h)
{
	XDrawRectangle(xw.dpy, xw.buf, xw.gc, x, y, w, h);
}


void gfx_circle(int cx, int cy, int r)
{
	XDrawArc(xw.dpy, xw.buf, xw.gc, cx - r, cy - r, 2*r, 2*r,
		0*64, 360*64);
}


void gfx_ellipse(int cx, int cy, int rx, int ry)
{
	XDrawArc(xw.dpy, xw.buf, xw.gc, cx - rx, cy - ry, 2*rx, 2*ry,
		0*64, 360*64);
}


void gfx_arc(int cx, int cy, int rx, int ry, double ang1, double ang2)
{
	XDrawArc(xw.dpy, xw.buf, xw.gc, cx - rx, cy - ry, 2*rx, 2*ry,
		360*64*ang1, 360*64*ang2);
}


void gfx_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	XPoint t[3];
		t[0].x = x1;
		t[0].y = y1;
		t[1].x = x2;
		t[1].y = y2;
		t[2].x = x3;
		t[2].y = y3;
	XFillPolygon(xw.dpy, xw.buf, xw.gc, t, 3, Convex, CoordModeOrigin);
}


void gfx_fill_rect(int x, int y, int w, int h)
{
	XFillRectangle(xw.dpy, xw.buf, xw.gc, x, y, w, h);
}


void gfx_fill_circle(int cx, int cy, int r)
{
	XFillArc(xw.dpy, xw.buf, xw.gc, cx - r, cy - r, 2*r, 2*r,
		0*64, 360*64);
}


void gfx_fill_ellipse(int cx, int cy, int rx, int ry)
{
	XFillArc(xw.dpy, xw.buf, xw.gc, cx - rx, cy - ry, 2*rx, 2*ry,
		0*64, 360*64);
}


void gfx_fill_arc(int cx, int cy, int rx, int ry, double ang1, double ang2)
{
	XFillArc(xw.dpy, xw.buf, xw.gc, cx - rx, cy - ry, 2*rx, 2*ry,
		360*64*ang1, 360*64*ang2);
}


int gfx_pending_events()
{
	int p;
	XEvent e;

	gfx_flush();
	p = XPending(xw.dpy);
	if (!p)
		return p;

	XNextEvent(xw.dpy, &e);

	if (GraphicsExpose == e.type) {
		dprintf(("WARNING: GraphicsExpose event\n"));
		return 0;
	}

	if (NoExpose == e.type) {
		dprintf(("WARNING: NoExpose event\n"));
		return 0;
	}

	XPutBackEvent(xw.dpy, &e);
	return p;
}


void gfx_next_event(struct gfx_event *e)
{
	XEvent xe;
	XNextEvent(xw.dpy, &xe);
	if (xhandler[xe.type] && NULL != e)
		xhandler[xe.type](&xe, e);
}


static int last_modifier;


static void xparse_modifiers(int *modifier, int state)
{
	*modifier = 0;
	if (state & ControlMask) *modifier |= GFX_MOD_CTRL;
	if (state & ShiftMask)   *modifier |= GFX_MOD_SHIFT;
	if (state & Mod1Mask)    *modifier |= GFX_MOD_ALT;
	if (state & LockMask)    *modifier |= GFX_MOD_LOCK;
	last_modifier = *modifier;
}


static int parse_ascii(struct gfx_event *e, XEvent *xe, KeySym ks,
		char *buf, int len,
		char *buf_u8, int len_u8)
{
	struct gfx_event_key *k;
	k = &e->data.key;

	if (XK_space <= ks && ks <= XK_asciitilde) {
		k->type = GFX_KEY_ASCII;
		if (xe->xkey.state & ControlMask)
			k->value[0] = 'a' + buf[0] - 1;
		else
			k->value[0] = buf[0];
		k->len = 1;
		return 1;
	}

#define KEYS(X) \
	X(XK_BackSpace, '\b') \
	X(XK_Tab,       '\t') \
	X(XK_Linefeed,  '\n') \
	X(XK_Clear,     '\v') \
	X(XK_Return,    '\r') \
	X(XK_Escape,    0x1b) \
	X(XK_Delete,    0x7f) \

	switch (ks) {
	default: return 0;

#define X(sym, val) \
	case sym: \
		k->type = GFX_KEY_ASCII; \
		k->value[0] = val; \
		k->len = 1; \
		return 1;
	KEYS(X)
#undef X

	}

#undef KEYS
}


static int parse_utf8(struct gfx_event *e, XEvent *xe, KeySym ks,
		char *buf, int len,
		char *buf_u8, int len_u8)
{
	struct gfx_event_key *k;
	k = &e->data.key;

	if (!len_u8 || k->filter)
		return 0;

	k->len = 4 < len_u8 ? 4:len_u8;
	strncpy(k->value, buf_u8, k->len);
	return 1;
}


static int parse_cursor(struct gfx_event *e, XEvent *xe, KeySym ks,
		char *buf, int len,
		char *buf_u8, int len_u8)
{
	struct gfx_event_key *k;
	k = &e->data.key;

#define KEYS(X) \
	X(XK_Left,      GFX_KEY_LEFT) \
	X(XK_Up,        GFX_KEY_UP) \
	X(XK_Right,     GFX_KEY_RIGHT) \
	X(XK_Down,      GFX_KEY_DOWN) \
	X(XK_Begin,     GFX_KEY_BEGIN) \
	X(XK_End,       GFX_KEY_END) \
	X(XK_Page_Up,   GFX_KEY_PAGE_UP) \
	X(XK_Page_Down, GFX_KEY_PAGE_DOWN) \
	X(XK_Home,      GFX_KEY_HOME) \

	switch (ks) {
	default: return 0;

#define X(sym, val) \
	case sym: \
		k->type = GFX_KEY_CURSOR; \
		k->value[0] = val; \
		k->len = 1; \
		return 1;
	KEYS(X)
#undef X

	}

#undef KEYS
}


static int parse_mod(struct gfx_event *e, XEvent *xe, KeySym ks,
		char *buf, int len,
		char *buf_u8, int len_u8)
{
	struct gfx_event_key *k;
	k = &e->data.key;

#define KEYS(X) \
	X(XK_Control_L,        GFX_KEY_CTRL_L) \
	X(XK_Control_R,        GFX_KEY_CTRL_R) \
	X(XK_Shift_L,          GFX_KEY_SHIFT_L) \
	X(XK_Shift_R,          GFX_KEY_SHIFT_R) \
	X(XK_Alt_L,            GFX_KEY_ALT_L) \
	X(XK_Alt_R,            GFX_KEY_ALT_R) \
	X(XK_ISO_Level3_Shift, GFX_KEY_ALT_R) \
	X(XK_Caps_Lock,        GFX_KEY_CAPS_LOCK) \
	X(XK_Shift_Lock,       GFX_KEY_SHIFT_LOCK) \

	switch (ks) {
	default: return 0;

#define X(sym, val) \
	case sym: \
		k->type = GFX_KEY_MOD; \
		k->value[0] = val; \
		k->len = 1; \
		return 1;
	KEYS(X)
#undef X

	}

#undef KEYS
}


static void debug_key(struct gfx_event *e, XEvent *xe, KeySym ks,
		char *buf, int len,
		char *buf_u8, int len_u8)
{
	int i;
	char *keyname;
	char   xbuf_u8[1024] = {0};
	size_t xlen_u8       = 0;
	char   xbuf[1024]      = {0};
	size_t xlen            = 0;

	keyname = XKeysymToString(ks);

	if (!len) {
		xbuf[0] = '-';
	} else {
		unsigned char *b = (unsigned char*)buf;
		for (i = 0; i < len; i++)
			xlen += sprintf(xbuf + xlen,
				"0x%02x ", b[i]);
		xbuf[strlen(xbuf) - 1] = '\0';
	}

	if (!len_u8) {
		xbuf_u8[0] = '-';
	} else {
		unsigned char *b = (unsigned char*)buf_u8;
		for (i = 0; i < len_u8; i++)
			xlen_u8 += sprintf(xbuf_u8 + xlen_u8,
				"0x%02x ", b[i]);
		xbuf_u8[strlen(xbuf_u8) - 1] = '\0';
	}

	dprintf((
		"@%d -> %s (%d 0x%02lx %s) "
		"lookup %d (%s) lookup_utf8 %d (%s) "
		"filter %s\n",
		e->time_ms,
		GFX_EVENT_KEYDOWN  == e->type ? "KeyDown ":
		GFX_EVENT_KEYPRESS == e->type ? "KeyPress":
		GFX_EVENT_KEYUP    == e->type ? "KeyUp   ":"KeyNone",
		e->data.key.keycode,
		ks,
		NULL != keyname ? keyname:"-",
		len, xbuf,
		len_u8, xbuf_u8,
		e->data.key.filter ? "yes":"no"
	));
}


static struct gfx_event last_key;
static int              generate_keypress;


static void xkey_press(XEvent *xe, struct gfx_event *e)
{
	KeySym keysym = 0;
	Status status = 0;
	int  filter     = 0;
	char buf_u8[64] = {0};
	int  len_u8     = 0;
	char buf[64]    = {0};
	int  len        = 0;
	int  is_ascii   = 0;
	int  is_utf8    = 0;
	int  is_cursor  = 0;
	int  is_mod     = 0;

	len = XLookupString(&xe->xkey, buf, sizeof(buf), &keysym, NULL);
	if (NULL != xw.ic && KeyPress == xe->xkey.type) {
		KeySym ks;
		len_u8 = Xutf8LookupString(xw.ic, &xe->xkey,
			buf_u8, sizeof(buf_u8),
			&ks, &status
		);
	}
	filter = (True == XFilterEvent(xe, xw.win));
	assert(XBufferOverflow != status && "Buffer is too small");

	time_elapsed_ms(e);
	xparse_modifiers(&e->modifier, xe->xkey.state);
	e->data.key.keycode = xe->xkey.keycode;
	e->data.key.filter = filter;


	is_ascii  =  parse_ascii(e, xe, keysym, buf, len, buf_u8, len_u8);
	if (is_ascii)
		goto parsed;

	is_utf8   =   parse_utf8(e, xe, keysym, buf, len, buf_u8, len_u8);
	if (is_utf8)
		goto parsed;

	is_cursor = parse_cursor(e, xe, keysym, buf, len, buf_u8, len_u8);
	if (is_cursor)
		goto parsed;

	is_mod    =    parse_mod(e, xe, keysym, buf, len, buf_u8, len_u8);
	if (is_mod)
		goto parsed;

	e->data.key.type = GFX_KEY_UNKNOWN, e->data.key.len = 0;
parsed:


	if (KeyPress == xe->type && !is_mod && !filter) {
		if (generate_keypress) {
			generate_keypress = 0;
			e->type = GFX_EVENT_KEYPRESS;
			memcpy(&last_key, e, sizeof(last_key));
		} else {
			generate_keypress = 1;
			e->type = GFX_EVENT_KEYDOWN;
			XPutBackEvent(xw.dpy, xe);
		}


	} else if (KeyPress == xe->type) {
		e->type = GFX_EVENT_KEYDOWN;


	} else if (last_key.data.key.keycode
			&& XEventsQueued(xw.dpy, QueuedAfterReading)) {
		XEvent xe2;
		XNextEvent(xw.dpy, &xe2);

		if (xe2.type != KeyPress)
			goto putback;

		if (xe2.xkey.time != xe->xkey.time)
			goto putback;

		if (xe2.xkey.keycode != xe->xkey.keycode)
			goto putback;

		if (255 < xe2.xkey.keycode)
			goto putback;

		if (xe2.xkey.keycode != last_key.data.key.keycode)
			goto putback;

		memcpy(e, &last_key, sizeof(*e));
		time_elapsed_ms(e);
		goto end;

	putback:
		XPutBackEvent(xw.dpy, &xe2);
		e->type = GFX_EVENT_KEYUP;
		memset(&last_key, 0, sizeof(last_key));
	end:
		;


	} else {
		e->type = GFX_EVENT_KEYUP;
		memset(&last_key, 0, sizeof(last_key));
	}


#ifndef NDEBUG
	if ((DEBUG_KEYDOWN & options.debug) && GFX_EVENT_KEYDOWN == e->type)
		debug_key(e, xe, keysym, buf, len, buf_u8, len_u8);

	if ((DEBUG_KEYPRESS & options.debug) && GFX_EVENT_KEYPRESS == e->type)
		debug_key(e, xe, keysym, buf, len, buf_u8, len_u8);

	if ((DEBUG_KEYUP & options.debug) && GFX_EVENT_KEYUP == e->type)
		debug_key(e, xe, keysym, buf, len, buf_u8, len_u8);
#endif
}


static void xmapping(XEvent *xe, struct gfx_event *e)
{
	dprintf(("Refreshing keyboard mapping\n"));
	XRefreshKeyboardMapping(&xe->xmapping);
	memset(&last_key, 0, sizeof(last_key));
}


static void xmouse_press(XEvent *xe, struct gfx_event *e)
{
	e->type = GFX_EVENT_MOUSEDOWN;
	time_elapsed_ms(e);
	xparse_modifiers(&e->modifier, xe->xbutton.state);
	switch (xe->xbutton.button) {
	case Button1: e->data.mouse.button = GFX_MOUSE_LEFT;  break;
	default:      e->data.mouse.button = GFX_MOUSE_RIGHT; break;
	}
	e->data.mouse.x = xe->xbutton.x;
	e->data.mouse.y = xe->xbutton.y;

	dtprintf(MOUSEDOWN, ("@%d -> MouseDown %s (%d, %d)\n",
		e->time_ms,
		GFX_MOUSE_LEFT == e->data.mouse.button ? "Left":"Right",
		e->data.mouse.x,
		e->data.mouse.y));
}


static void xmouse_release(XEvent *xe, struct gfx_event *e)
{
	e->type = GFX_EVENT_MOUSEUP;
	time_elapsed_ms(e);
	xparse_modifiers(&e->modifier, xe->xbutton.state);
	switch (xe->xbutton.button) {
	case Button1: e->data.mouse.button = GFX_MOUSE_LEFT;  break;
	default:      e->data.mouse.button = GFX_MOUSE_RIGHT; break;
	}
	e->data.mouse.x = xe->xbutton.x;
	e->data.mouse.y = xe->xbutton.y;

	dtprintf(MOUSEUP, ("@%d -> MouseUp %s (%d, %d)\n",
		e->time_ms,
		GFX_MOUSE_LEFT == e->data.mouse.button ? "Left":"Right",
		e->data.mouse.x,
		e->data.mouse.y));
}


static void xmouse_move(XEvent *xe, struct gfx_event *e)
{
	e->type = GFX_EVENT_MOUSEMOVE;
	time_elapsed_ms(e);
	xparse_modifiers(&e->modifier, xe->xbutton.state);
	e->data.mouse.button = GFX_MOUSE_LEFT;
	e->data.mouse.x = xe->xmotion.x;
	e->data.mouse.y = xe->xmotion.y;

	dtprintf(MOUSEMOVE, ("@%d -> MouseMove %s (%d, %d)\n",
		e->time_ms,
		GFX_MOUSE_LEFT == e->data.mouse.button ? "Left":"Right",
		e->data.mouse.x,
		e->data.mouse.y));
}


static void xresize(XEvent *xe, struct gfx_event *e)
{
	e->type = GFX_EVENT_RESIZE;
	time_elapsed_ms(e);
	e->modifier = last_modifier;
	e->data.resize.x = xe->xconfigure.width;
	e->data.resize.y = xe->xconfigure.height;

	if (xw.w == xe->xconfigure.width && xw.h == xe->xconfigure.height)
		return;

	dtprintf(RESIZE, ("Resize [%d, %d] -> [%d, %d]\n",
		xw.w, xw.h,
		xe->xconfigure.width,
		xe->xconfigure.height));

	xw.w = xe->xconfigure.width;
	xw.h = xe->xconfigure.height;

	if (!options.buffer)
		return;
	XFreePixmap(xw.dpy, xw.buf);
	xw.buf = XCreatePixmap(xw.dpy, xw.win,
		xw.w, xw.h,
		DefaultDepth(xw.dpy, xw.scr)
	);
	XSetForeground(xw.dpy, xw.gc, xw.bg_color.pixel);
	XFillRectangle(xw.dpy, xw.buf, xw.gc, 0, 0, xw.w, xw.h);
	XSetForeground(xw.dpy, xw.gc, xw.fg_color.pixel);
}


static void xclose(XEvent *xe, struct gfx_event *e)
{
	e->type = GFX_EVENT_NONE;
	time_elapsed_ms(e);
	e->modifier = last_modifier;

	if (None == xw.delete_msg) {
		dprintf(("Listening to close events is not enabled\n"));
		return;
	}

	if (xw.delete_msg != xe->xclient.data.l[0]) {
		dprintf(("Event is not a delete window message\n"));
		return;
	}

	dprintf(("Close event received, please handle this event\n"));
	e->type = GFX_EVENT_CLOSE;
}
