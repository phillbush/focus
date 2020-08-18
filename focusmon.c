#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include "util.h"

/* Function declarations */
static int getcurrmon(struct Monitor *monlist, int nmons);
static int checkmon(struct Monitor *monlist, int a, int b, int currmon, enum Direction dir);
static int getmondir(struct Monitor *monlist, int nmons, int currmon, enum Direction dir);
static int getmonnext(int nmons, int currmon);
static int getmonprev(int nmons, int currmon);
static void focusmon(struct Monitor *monlist, int mon);
static void usage(void);

/* focusmon: focus monitor */
int
main(int argc, char *argv[])
{
	enum Direction dir;
	struct Monitor *monlist;
	int mon, nmons, currmon;

	argc--;
	argv++;

	if (argc != 1)
		usage();

	initX();

	dir = getdirection(*argv);
	nmons = getmonitors(&monlist);
	currmon = getcurrmon(monlist, nmons);
	switch (dir) {
	case Absolute:
		mon = getnum(*argv);
		break;
	case Left: case Right: case Up: case Down:
		mon = getmondir(monlist, nmons, currmon, dir);
		break;
	case Prev:
		mon = getmonprev(nmons, currmon);
		break;
	case Next:
		mon = getmonnext(nmons, currmon);
		break;
	}
	if (mon < 0 || mon >= nmons)
		errx(1, "unknown monitor: %d", mon);
	if (mon != currmon)
		focusmon(monlist, mon);
	free(monlist);

	killX();

	return 0;
}

/* get index of currently focused monitor */
static int
getcurrmon(struct Monitor *monlist, int nmons)
{
	int i;
	int mon = 0;
	int x, y;
	Window dw;          /* dummy variable */
	int di;             /* dummy variable */
	unsigned du;        /* dummy variable */

	XQueryPointer(dpy, root, &dw, &dw, &x, &y, &di, &di, &du);
	for (i = 0; i < nmons; i++) {
		if (BETWEEN(x, monlist[i].x, monlist[i].x + monlist[i].w) &&
		    BETWEEN(y, monlist[i].y, monlist[i].y + monlist[i].h)) {
			mon = i;
			break;
		}
	}
	return mon;
}

/*
 * check whether monitor a is in the given direction,
 * and check whether it is closer to current monitor than b
 */
static int
checkmon(struct Monitor *monlist, int a, int b, int currmon, enum Direction dir)
{
	switch (dir) {
	case Left:
		if (monlist[a].x < monlist[currmon].x &&
		    (b < 0 || monlist[a].x > monlist[b].x))
			return 1;
		break;
	case Right:
		if (monlist[a].x > monlist[currmon].x &&
		    (b < 0 || monlist[a].x < monlist[b].x))
			return 1;
		break;
	case Up:
		if (monlist[a].y < monlist[currmon].y &&
		    (b < 0 || monlist[a].x > monlist[b].y))
			return 1;
		break;
	case Down:
		if (monlist[a].y > monlist[currmon].y &&
		    (b < 0 || monlist[a].y < monlist[b].y))
			return 1;
		break;
	default:
		break;
	}
	return 0;
}

/* get index of monitor by direction */
static int
getmondir(struct Monitor *monlist, int nmons, int currmon, enum Direction dir)
{
	int i, mon;

	mon = -1;
	for (i = 0; i < nmons; i++) {
		if (checkmon(monlist, i, mon, currmon, dir)) {
			mon = i;
		}
	}
	return mon;
}

/* get index of monitor before the currently focused one */
static int
getmonprev(int nmons, int currmon)
{
	return (currmon > 0) ? currmon - 1 : nmons - 1;
}

/* get index of monitor after the currently focused one */
static int
getmonnext(int nmons, int currmon)
{
	return (currmon >= nmons - 1) ? 0 : currmon + 1;
}

/* click on root window in the given monitor */
static void
focusmon(struct Monitor *monlist, int mon)
{
	XEvent ev;
	int x, y;

	x = monlist[mon].x + monlist[mon].w / 2;
	y = monlist[mon].y + monlist[mon].h / 2;
	ev.xbutton.type = ButtonPress;
	ev.xbutton.serial = 0;
	ev.xbutton.send_event = True;
	ev.xbutton.same_screen = True;
	ev.xbutton.root = root;
	ev.xbutton.window = root;
	ev.xbutton.subwindow = None;
	ev.xbutton.x = x;
	ev.xbutton.y = y;
	ev.xbutton.x_root = x;
	ev.xbutton.y_root = y;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;

	XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
	if (!XSendEvent(dpy, root, False, ButtonPressMask, &ev))
		errx(1, "could not go to monitor %d", mon);
}

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: focusmon direction\n");
	exit(1);
}
