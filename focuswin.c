#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include "util.h"

/* Macros */
#define BETWEEN(x, a, b)    ((a) <= (x) && (x) <= (b))

static Atom netactivewindow;
static int rflag = 0;

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: focuswin [-r] direction\n");
	exit(1);
}

/* get list of geometry of clients */
static struct Client *
getclientgeoms(Window *winlist, ulong nwins)
{
	struct Client *clientlist;
	Window win, parentwin = None;
	XWindowAttributes wa;
	ulong i;
	Window dw, *dws;        /* dummy window */
	unsigned du;            /* dummy variable */

	if ((clientlist = calloc(nwins, sizeof *clientlist)) == NULL)
		err(1, "calloc");
	for (i = 0; i < nwins; i++) {
		win = winlist[i];
		if (rflag) {
			if (XQueryTree(dpy, win, &dw, &parentwin, &dws, &du) && dws) {
				XFree(dws);
				win = parentwin;
			}
		}
		if (!XGetWindowAttributes(dpy, win, &wa))
			errx(1, "could not get client geometry");
		clientlist[i].x = wa.x;
		clientlist[i].y = wa.y;
		clientlist[i].w = wa.width;
		clientlist[i].h = wa.height;
	}
	return clientlist;
}

/* return focused window */
static ulong
getfocuswin(Window *winlist, ulong nwins)
{
	Window win, focuswin = None, parentwin = None;
	unsigned char *u;
	ulong i;
	Atom da;            /* dummy variable */
	Window dw, *dws;    /* dummy variable */
	int di;             /* dummy variable */
	unsigned du;        /* dummy variable */
	unsigned long dl;   /* dummy variable */

	if (XGetWindowProperty(dpy, root, netactivewindow, 0L, 1024, False,
	                       XA_WINDOW, &da, &di, &dl, &dl, &u) == Success
	                       && u) {
		focuswin = *(Window *)u;
		XFree(u);
	}
	if (focuswin == None) {
		XGetInputFocus(dpy, &win, &di);
		if (win != None) {
			while (parentwin != root) {
				if (XQueryTree(dpy, win, &dw, &parentwin, &dws, &du) && dws)
					XFree(dws);
				focuswin = win;
				win = parentwin;
			}
		}
	}
	for (i = 0; i < nwins; i++)
		if (winlist[i] == focuswin)
			return i;
	errx(1, "could not find focused window");
}

/* return pointer to monitor containing the client with geometry *geom */
static int
getfocusmon(struct Client *geom, struct Monitor *monlist, int nmons)
{
	int i, x, y;

	x = geom->x + geom->w / 2;
	y = geom->y + geom->h / 2;
	for (i = 0; i < nmons; i++)
		if (BETWEEN(x, monlist[i].x, monlist[i].x + monlist[i].w) &&
		    BETWEEN(y, monlist[i].y, monlist[i].y + monlist[i].h))
			return i;
	errx(1, "could not find focused monitor");
}

/* return 1 if a is closer to focus than b, 0 otherwise */
static int
clientcmp(struct Client *a, struct Client *b, struct Client *focus, enum Direction dir)
{
	if (b == focus)
		return 1;
	switch (dir) {
	default:
		break;
	case Left: case Right:
		if (a->y == focus->y &&  b->y != focus->y)
			return 1;
		if (a->x > b->x + b->w && dir == Left)
			return 1;
		if (a->x + a->w < b->x && dir == Right)
			return 1;
		if (a->x == b->x && b->x == focus->x && b->w == focus->w)
			return 0;
		if (a->x == b->x)
			return 1;
		break;
	case Up: case Down:
		if (a->x == focus->x && b->x != focus->x)
			return 1;
		if (a->y > b->y + b->h && dir == Up)
			return 1;
		if (a->y + a->h < b->y && dir == Down)
			return 1;
		if (a->y == b->y && b->x == focus->x && b->w == focus->w)
			return 0;
		if (a->y == b->y)
			return 1;
		break;
	}
	return 0;
}

/* check whether client c is in monitor mon */
static int
clientisinmon(struct Client *c, struct Monitor *mon)
{
	if (mon->x <= c->x && c->x < mon->x + mon->w && mon->y <= c->y && c->y < mon->y + mon->h)
		return 1;
	return 0;
}

/* check whether client c is in the correct direction relative to focus */
static int
clientisindir(struct Client *c, struct Client *focus, enum Direction dir)
{
	switch (dir) {
	case Left:
		if (c->x < focus->x)
			return 1;
		break;
	case Right:
		if (c->x > focus->x)
			return 1;
		break;
	case Up:
		if (c->y < focus->y)
			return 1;
		break;
	case Down:
		if (c->y > focus->y)
			return 1;
		break;
	default:
		return 1;
	}
	return 0;
}

/* get previous window */
static ulong
getwinprev(struct Monitor *mon, struct Client *geom, ulong nwins, ulong currwin)
{
	ulong i;

	if (currwin > 0) {
		for (i = currwin - 1; i > 0; i--) {
			if (clientisinmon(&geom[i], mon)) {
				return i;
			}
		}
	}
	for (i = nwins - 1; i > currwin; i--) {
		if (clientisinmon(&geom[i], mon) && i != currwin) {
			return i;
		}
	}
	return currwin;
}

/* get next window */
static ulong
getwinnext(struct Monitor *mon, struct Client *geom, ulong nwins, ulong currwin)
{
	ulong i;

	for (i = currwin + 1; i < nwins; i++) {
		if (clientisinmon(&geom[i], mon)) {
			return i;
		}
	}
	for (i = 0; i < currwin; i++) {
		if (clientisinmon(&geom[i], mon) && i != currwin) {
			return i;
		}
	}
	return currwin;
}

/* get window by direction dir */
static ulong
getwindir(struct Monitor *mon, struct Client *geom, ulong nwins, ulong currwin, enum Direction dir)
{
	struct Client *curr;
	ulong i, j;

	j = currwin;
	curr = &geom[currwin];
	for (i = 0; i < nwins; i++)
		if (clientisinmon(&geom[i], mon) &&
		    clientisindir(&geom[i], curr, dir) &&
		    clientcmp(&geom[i], &geom[j], curr, dir))
			j = i;
	return j;
}

/* get n-th window in monitor mon */
static ulong
getwinabs(struct Monitor *mon, struct Client *geom, ulong nwins, ulong currwin, ulong n)
{
	ulong i, j, count;

	count = j = 0;
	for (i = 0; i < nwins && count < n + 1; i++) {
		if (clientisinmon(&geom[i], mon)) {
			j = i;
			count++;
		}
	}
	return (count == n + 1) ? j : currwin;
}

/* focus window */
static void
focuswin(Window win)
{
	XEvent ev;
	long mask = SubstructureRedirectMask | SubstructureNotifyMask;

	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = netactivewindow;
	ev.xclient.window = win;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 0;
	ev.xclient.data.l[1] = 0;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	if (!XSendEvent(dpy, root, False, mask, &ev))
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

/* focuswin: focus a window */
int
main(int argc, char *argv[])
{
	enum Direction dir;
	struct Monitor *monlist;
	struct Client *geoms;
	Window *winlist;
	ulong currwin, nwins, win;
	int currmon, nmons;
	int ch;

	while ((ch = getopt(argc, argv, "r")) != -1) {
		switch (ch) {
		case 'r':
			rflag = 1;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	initX();
	netactivewindow = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

	nwins = getwinlist(&winlist);
	if (nwins == 0) {
		free(winlist);
		return 0;
	}
	dir = getdirection(*argv);
	nmons = getmonitors(&monlist);
	geoms = getclientgeoms(winlist, nwins);
	currwin = getfocuswin(winlist, nwins);
	currmon = getfocusmon(&geoms[currwin], monlist, nmons);
	switch (dir) {
	case Prev:
		win = getwinprev(&monlist[currmon], geoms, nwins, currwin);
		break;
	case Next:
		win = getwinnext(&monlist[currmon], geoms, nwins, currwin);
		break;
	case Left: case Right: case Up: case Down:
		win = getwindir(&monlist[currmon], geoms, nwins, currwin, dir);
		break;
	case Absolute:
		win = getnum(*argv);
		win = getwinabs(&monlist[currmon], geoms, nwins, currwin, win);
		break;
	}
	free(geoms);
	free(monlist);
	if (win >= nwins)
		errx(1, "window out of bounds");
	if (win != currwin)
		focuswin(winlist[win]);
	free(winlist);

	killX();

	return 0;
}
