#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include "util.h"

Display *dpy;
Window root;
int screen;

/* initialize X */
void
initX(void)
{
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "could not open display");
	screen = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
}

/* terminate X */
void
killX(void)
{
	XCloseDisplay(dpy);
}

/* get direction to focus */
enum Direction
getdirection(const char *s)
{
	if (tolower(*s) == 'l')
		return Left;
	else if (tolower(*s) == 'r')
		return Right;
	else if (tolower(*s) == 'u')
		return Up;
	else if (tolower(*s) == 'd')
		return Down;
	else if (tolower(*s) == 'p')
		return Prev;
	else if (tolower(*s) == 'n')
		return Next;
	return Absolute;
}

/* get number from s */
ulong
getnum(const char *s)
{
	long n;
	char *endp;

	n = strtol(s, &endp, 10);
	if (errno == ERANGE || n < 0 || endp == s || *endp != '\0')
		errx(1, "improper argument: %s", s);
	return (ulong)n;
}

/* get cardinal property */
ulong
getcardinalproperty(Window win, Atom prop)
{
	unsigned char *p = NULL;
	ulong dl;           /* dummy variable */
	int di;             /* dummy variable */
	Atom da;            /* dummy variable */
	ulong retval;

	if (XGetWindowProperty(dpy, win, prop, 0L, 1L, False, XA_CARDINAL,
	                       &da, &di, &dl, &dl, &p) == Success && p) {
		retval = *(ulong *)p;
		XFree(p);
	} else {
		errx(1, "XGetWindowProperty");
	}

	return retval;
}

/* get list of windows, return number of windows */
ulong
getwinlist(Window **winlist)
{
	unsigned char *list;
	ulong len;
	Atom netclientlist;
	ulong dl;           /* dummy variable */
	unsigned int du;    /* dummy variable */
	int di;             /* dummy variable */
	Window dw;          /* dummy variable */
	Atom da;            /* dummy variable */

	netclientlist = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	if (XGetWindowProperty(dpy, root, netclientlist, 0L, 1024, False,
	                       XA_WINDOW, &da, &di, &len, &dl, &list) ==
	                       Success && list) {
		*winlist = (Window *)list;
	} else if (XQueryTree(dpy, root, &dw, &dw, winlist, &du)) {
		len = (ulong)du;
	} else {
		errx(1, "could not get list of windows");
	}

	return len;
}

/* get list of monitors and their geometries, return number of monitors */
int
getmonitors(struct Monitor **monlist)
{
	XineramaScreenInfo *info = NULL;
	int nmons, i;

	if ((info = XineramaQueryScreens(dpy, &nmons)) != NULL && nmons > 0) {
		if ((*monlist = calloc(nmons, sizeof **monlist)) == NULL)
			err(1, "calloc");
		for (i = 0; i < nmons; i++) {
			(*monlist)[i].x = info[i].x_org;
			(*monlist)[i].y = info[i].y_org;
			(*monlist)[i].w = info[i].width;
			(*monlist)[i].h = info[i].height;
		}
	} else {
		nmons = 1;
		if ((*monlist = malloc(sizeof **monlist)) == NULL)
			err(1, "malloc");
		(*monlist)[0].x = 0;
		(*monlist)[0].y = 0;
		(*monlist)[0].w = DisplayWidth(dpy, screen);
		(*monlist)[0].h = DisplayHeight(dpy, screen);
	}
	XFree(info);
	return nmons;
}
