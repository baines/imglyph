#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

static uint64_t mask;
static Display* dpy;
static Window win;
static char output[256];

void redraw(bool mask_changed){
	GC gc = XDefaultGC(dpy, 0);

	XClearWindow(dpy, win);

	for(int i = 0; i < 8; ++i){
		for(int j = 0; j < 8; ++j){

			if(mask & (UINT64_C(1) << (i*8+j))){
				XFillRectangle(dpy, win, gc,
				               10 + j * 32,
				               10 + i * 32,
				               32, 32);
			} else {
				XDrawRectangle(dpy, win, gc,
				               10 + j * 32,
				               10 + i * 32,
				               32, 32);
			}

		}
	}

	if(mask_changed){
		snprintf(output, sizeof(output), "\t{ 0x????, UINT64_C(%#" PRIx64 ") },\n", mask);
		XSetSelectionOwner(dpy, XA_PRIMARY, win, CurrentTime);
	}
}

void update_grid(int x, int y, int butmask, bool is_drag){
	static int prev_x, prev_y;

	x = (x - 10) / 32;
	y = (y - 10) / 32;

	if(x < 0 || y < 0 || x >= 8|| y >= 8)
		return;

	if(is_drag && (x == prev_x && y == prev_y))
		return;

	uint64_t shift = (y*8+x);

	if(butmask & Button1Mask){
		mask |= (UINT64_C(1) << shift);
	} else {
		mask &= ~(UINT64_C(1) << shift);
	}

	redraw(true);

	prev_x = x;
	prev_y = y;
}

int main(void){
	dpy = XOpenDisplay(NULL);
	win = XCreateSimpleWindow(dpy, RootWindow(dpy, 0), 0, 0, 276, 276, 0, 0, WhitePixel(dpy, 0));

	XStoreName(dpy, win, "mkmask");
	XMapRaised(dpy, win);

	XSelectInput(dpy, win,
	             ButtonPressMask |
	             KeyPressMask |
	             ExposureMask |
	             Button1MotionMask |
	             Button3MotionMask);

	Atom targets = XInternAtom(dpy, "TARGETS", false);

	for(;;){
		XEvent ev;
		XNextEvent(dpy, &ev);

		if(ev.type == Expose){
			redraw(false);
		}

		else if(ev.type == KeyPress){
			KeySym sym = XLookupKeysym(&ev.xkey, 0);

			if(sym == XK_Return || sym == XK_space){
				printf("%s", output);
			} else if(sym == XK_r){
				mask = 0;
				redraw(true);
			} else if(sym == XK_f){
				mask = UINT64_MAX;
				redraw(true);
			} else if(sym == XK_Escape){
				return 0;
			}
		}

		else if(ev.type == ButtonPress){
			update_grid(ev.xbutton.x, ev.xbutton.y, 1 << (7+ev.xbutton.button), false);
		}

		else if(ev.type == MotionNotify){
			update_grid(ev.xmotion.x, ev.xmotion.y, ev.xmotion.state, true);
		}

		else if(ev.type == SelectionRequest && ev.xselectionrequest.property){
			XSelectionRequestEvent* sr = &ev.xselectionrequest;

			XSelectionEvent reply = {
				.type      = SelectionNotify,
				.requestor = sr->requestor,
				.selection = sr->selection,
				.target    = sr->target,
				.time      = sr->time,
				.property  = sr->property,
			};

			if(sr->target == targets){
				Atom a = XA_STRING;
				XChangeProperty(dpy, sr->requestor, sr->property, XA_ATOM, 32, PropModeReplace, (uint8_t*)&a, 1);
			} else if(sr->target == XA_STRING){
				XChangeProperty(dpy, sr->requestor, sr->property, XA_STRING, 8, PropModeReplace, output, strlen(output)+1);
			} else {
				reply.property = None;
			}

			XSendEvent(dpy, sr->requestor, True, 0, (XEvent*)&reply);
		}
	}

	return 0;
}
