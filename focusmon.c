#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include "util.h"

enum Direction {Absolute, Left, Right, Up, Down, Prev, Next};

/* Function declarations */
static int getcurrmon(struct Monitor *monlist, int nmons);
static int getmonleft(struct Monitor *monlist, int nmons, int currmon);
static int getmonright(struct Monitor *monlist, int nmons, int currmon);
static int getmonup(struct Monitor *monlist, int nmons, int currmon);
static int getmondown(struct Monitor *monlist, int nmons, int currmon);
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

	if (argc > 1)
		usage();

	dir = Next;
	if (argc == 1) {
		if (tolower(**argv) == 'l') {
			dir = Left;
		} else if (tolower(**argv) == 'r') {
			dir = Right;
		} else if (tolower(**argv) == 'u') {
			dir = Up;
		} else if (tolower(**argv) == 'd') {
			dir = Down;
		} else if (tolower(**argv) == 'p') {
			dir = Prev;
		} else if (tolower(**argv) == 'n') {
			dir = Next;
		} else {
			dir = Absolute;
			mon = getnum(*argv);
		}
	}

	initX();

	nmons = getmonitors(&monlist);
	if (dir != Absolute) {
		currmon = getcurrmon(monlist, nmons);
		switch (dir) {
		case Absolute:
			break;
		case Left:
			mon = getmonleft(monlist, nmons, currmon);
			break;
		case Right:
			mon = getmonright(monlist, nmons, currmon);
			break;
		case Up:
			mon = getmonup(monlist, nmons, currmon);
			break;
		case Down:
			mon = getmondown(monlist, nmons, currmon);
			break;
		case Prev:
			mon = getmonprev(nmons, currmon);
			break;
		case Next:
			mon = getmonnext(nmons, currmon);
			break;
		}
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

/* get index of monitor to the left of the currently focused one */
static int
getmonleft(struct Monitor *monlist, int nmons, int currmon)
{
	int i, mon;

	mon = currmon;
	for (i = 0; i < nmons; i++) {
		if (monlist[i].x < monlist[mon].x) {
			mon = i;
			break;
		}
	}
	return mon;
}

/* get index of monitor to the right of the currently focused one */
static int
getmonright(struct Monitor *monlist, int nmons, int currmon)
{
	int i, mon;

	mon = currmon;
	for (i = 0; i < nmons; i++) {
		if (monlist[i].x > monlist[mon].x) {
			mon = i;
			break;
		}
	}
	return mon;
}

/* get index of monitor above the currently focused one */
static int
getmonup(struct Monitor *monlist, int nmons, int currmon)
{
	int i, mon;

	mon = currmon;
	for (i = 0; i < nmons; i++) {
		if (monlist[i].y < monlist[mon].y) {
			mon = i;
			break;
		}
	}
	return mon;
}

/* get index of monitor below the currently focused one */
static int
getmondown(struct Monitor *monlist, int nmons, int currmon)
{
	int i, mon;

	mon = currmon;
	for (i = 0; i < nmons; i++) {
		if (monlist[i].y > monlist[mon].y) {
			mon = i;
			break;
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
	(void)fprintf(stderr, "usage: focusmon [direction]\n");
	exit(1);
}
