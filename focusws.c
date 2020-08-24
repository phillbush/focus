#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "util.h"

enum Mode {Normal, Cluster, Populated};

/* Function declarations */
static ulong getwscount(void);
static ulong getcurrws(ulong wscount);
static ulong *getwsusage(ulong wscount);
static ulong getnextws(ulong *wsusage, ulong wscount, ulong currws);
static ulong getprevws(ulong *wsusage, ulong wscount, ulong currws);
static void focusws(ulong wsnum);
static void usage(void);

/* Variable declarations */
static Atom netnumberofdesktops;
static Atom netcurrentdesktop;
static Atom netwmdesktop;
static enum Mode cyclemode = Normal;

/* focusws: focus through workspace or go to a workspace */
int
main(int argc, char *argv[])
{
	enum Direction dir;
	ulong wscount, currws, ws;
	ulong *wsusage = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "cp")) != -1) {
		switch (ch) {
		case 'c':
			cyclemode = Cluster;
			break;
		case 'p':
			cyclemode = Populated;
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
	netnumberofdesktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netcurrentdesktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netwmdesktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);

	dir = getdirection(*argv);
	wscount = getwscount();
	currws = getcurrws(wscount);
	if (dir == Absolute) {
		ws = getnum(*argv);
	} else if (dir == Next || dir == Prev) {
		if (cyclemode == Cluster || cyclemode == Populated)
			wsusage = getwsusage(wscount);
		ws = (dir == Next) ? getnextws(wsusage, wscount, currws)
		                   : getprevws(wsusage, wscount, currws);
		free(wsusage);
	} else {
		printf("focusws does not support focus by direction\n");
		return 0;
	}
	if (ws >= wscount)
		errx(1, "workspace out of bounds");
	if (ws != currws)
		focusws(ws);

	killX();

	return 0;
}

/* get number of workspaces */
static ulong
getwscount(void)
{
	ulong wscount;

	wscount = getcardinalproperty(root, netnumberofdesktops);
	if (wscount == 0)
		errx(1, "could not get number of workspaces");
	return wscount;
}

/* get current workspaces */
static ulong
getcurrws(ulong wscount)
{
	ulong currws;

	currws = getcardinalproperty(root, netcurrentdesktop);
	if (currws >= wscount)
		errx(1, "could not get current workspace");
	return currws;
}

/* create array of number of windows per workspace */
static ulong *
getwsusage(ulong wscount)
{
	Window *winlist;
	ulong winws;
	ulong wincount;
	ulong *wsusage;
	ulong i;

	if ((wsusage = calloc(wscount, sizeof *wsusage)) == NULL)
		err(1, "calloc");

	wincount = getwinlist(&winlist);

	for (i = 0; i < wincount; i++) {
		winws = getcardinalproperty(winlist[i], netwmdesktop);
		if (winws < wscount) {
			wsusage[winws]++;
		}
	}
	XFree(winlist);

	return wsusage;
}

/* get the next workspace */
static ulong
getnextws(ulong *wsusage, ulong wscount, ulong currws)
{
	ulong ws, i;

	ws = currws;

	switch (cyclemode) {
	case Normal:
		ws = (ws + 1 < wscount) ? ws + 1 : 0;
		break;
	case Populated:
		for (i = ws + 1; i < wscount; i++) {
			if (wsusage[i]) {
				ws = i;
				break;
			}
		}
		if (ws <= currws) {
			for (i = 0; i < currws; i++) {
				if (wsusage[i]) {
					ws = i;
					break;
				}
			}
		}
		break;
	case Cluster:
		if (ws + 1 < wscount && wsusage[ws]) {
			ws = ws + 1;
		} else {
			if (ws > 0) {
				for (i = ws - 1; i > 0; i--) {
					if (wsusage[i]) {
						ws = i;
					} else {
						break;
					}
				}
			}
			if ((ws == 0 || i == 0) && wsusage[0])
				ws = i;
		}
		break;
	}

	return ws;
}

/* get the prev workspace */
static ulong
getprevws(ulong *wsusage, ulong wscount, ulong currws)
{
	ulong ws, i;

	ws = currws;

	switch (cyclemode) {
	case Normal:
		ws = (ws > 0) ? ws - 1 : wscount - 1;
		break;
	case Populated:
		for (i = ws; i > 0; i--) {
			if (wsusage[i]) {
				ws = i;
				break;
			}
		}
		if (i == 0 && wsusage[0]) {
			ws = i;
		} else if (ws == currws) {
			for (i = wscount - 1; i > 0; i--) {
				if (wsusage[i]) {
					ws = i;
					break;
				}
			}
		}
		break;
	case Cluster:
		if (ws > 0 && wsusage[ws - 1]) {
			ws = ws - 1;
		} else {
			for (i = ws; i < wscount; i++) {
				ws = i;
				if (!wsusage[i]) {
					break;
				}
			}
		}
		break;
	}

	return ws;
}

/* send message to wm to change workspace */
static void
focusws(ulong wsnum)
{
	XEvent ev;
	long mask = SubstructureRedirectMask | SubstructureNotifyMask;

	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = netcurrentdesktop;
	ev.xclient.window = root;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = wsnum;
	ev.xclient.data.l[1] = CurrentTime;

	if (!XSendEvent(dpy, root, False, mask, &ev))
		errx(1, "could not send message to window manager");
}

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: focusws [-cp] direction\n");
	exit(1);
}
