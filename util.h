#ifndef _UTIL_H_
#define _UTIL_H_

#include <X11/Xlib.h>

/* macros */
#define BETWEEN(x, a, b)    ((a) <= (x) && (x) <= (b))

typedef unsigned long ulong;

enum Direction {Absolute, Left, Right, Up, Down, Prev, Next};

struct Monitor {
	int x, y, w, h;
};

struct Client {
	Window win;
	int x, y, w, h;
};

void initX(void);
void killX(void);
enum Direction getdirection(const char *s);
ulong getnum(const char *s);
ulong getcardinalproperty(Window win, Atom prop);
ulong getwinlist(Window **winlist);
int getmonitors(struct Monitor **monlist);

extern Display *dpy;
extern Window root;
extern int screen;

#endif /* _UTIL_H_ */
