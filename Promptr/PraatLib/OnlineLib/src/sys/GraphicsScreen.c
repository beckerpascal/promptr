/* GraphicsScreen.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2002/05/28 GPL
 * pb 2004/10/18 ReleaseDC
 * pb 2007/08/01 reintroduced yIsZeroAtTheTop
 * sdk 2008/03/24 cairo
 * sdk 2008/05/09 cairo
 */

#include "GraphicsP.h"
#include "Printer.h"

#if xwin
	static int inited;
	static Display *display;
	static int xscreen;
	static Window rootWindow;
	static Visual *visual;
	static unsigned int depth;
	static Colormap colourMap;
	static int theBitsPerPixel;
	static int thePad;
#elif win
#elif mac
	#include "macport_on.h"
	static RGBColor theBlackColour = { 0, 0, 0 };
#endif

static void destroy (I) {
	iam (GraphicsScreen);
	#if cairo
		if (my cr) cairo_destroy (my cr);
	#elif xwin
		XFreeGC (my display, my gc);
	#elif win
		if (my dc) {
			SelectPen (my dc, GetStockPen (BLACK_PEN));
			SelectBrush (my dc, GetStockBrush (NULL_BRUSH));
		}
		if (my pen) DeleteObject (my pen);
		if (my brush) DeleteObject (my brush);
		/*
		 * No ReleaseDC here, because we have not created it ourselves,
		 * not even with GetDC.
		 */
	#elif mac
		/* Nothing. */
	#endif
	inherited (GraphicsScreen) destroy (me);
}

void Graphics_flushWs (I) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			// Ik weet niet of dit is wat het zou moeten zijn ;)
			// gdk_window_process_updates (my window, TRUE);
			gdk_flush ();
			// TODO: een aanroep die de eventuele grafische buffer ledigt,
			// zodat de gebruiker de grafica ziet ook al blijft Praat in hetzelfde event zitten
		#elif xwin
			XFlush (my display);
		#elif win
			/*GdiFlush ();*/
		#elif mac
			if (my drawingArea) GuiWindow_drain (my drawingArea);
		#endif
	}
}

void Graphics_clearWs (I) {
	iam (Graphics);

	/* Suggestie: voor screen plaatsen, dan kan gtk gewoon 'updaten' */
	if (my record) { Melder_free (my record); my irecord = 0; my nrecord = 0; }
	
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) {
				Graphics_updateWs (me);
				return;
			} else {
				GdkRectangle rect;

				if (my x1DC < my x2DC) {
					rect.x = my x1DC;
					rect.width = my x2DC - my x1DC;
				} else {
					rect.x = my x2DC;
					rect.width = my x1DC - my x2DC;
				}

				if (my y1DC < my y2DC) {
					rect.y = my y1DC;
					rect.height = my y2DC - my y1DC;
				} else {
					rect.y = my y2DC;
					rect.height = my y1DC - my y2DC;
				}

				cairo_set_source_rgb (my cr, 1.0, 1.0, 1.0);
				// TODO: cairo_rectangle (my gc, 0, 0, GTK_WIDGET(I)->allocation.width, GTK_WIDGET(I)->allocation.height); ?
				cairo_rectangle (my cr, rect.x, rect.y, rect.width, rect.height);
				cairo_fill (my cr);
			}
		#elif xwin
			XClearArea (my display, my window, 0, 0, 0, 0, False);
		#elif win
			RECT rect;
			rect. left = rect. top = 0;
			rect. right = my x2DC - my x1DC;
			rect. bottom = my y2DC - my y1DC;
			FillRect (my dc, & rect, GetStockBrush (WHITE_BRUSH));
			/*if (my window) SendMessage (my window, WM_ERASEBKGND, (WPARAM) my dc, 0);*/
		#elif mac
			Rect r;
			RGBColor white = { 65535, 65535, 65535 }, black = { 0, 0, 0 };
			if (my drawingArea) GuiMac_clipOn (my drawingArea);
			SetRect (& r, my x1DC, my y1DC, my x2DC, my y2DC);
			SetPort (my macPort);
			RGBForeColor (& white);
			PaintRect (& r);
			RGBForeColor (& black);
			if (my drawingArea) GuiMac_clipOff ();
		#endif
	}
}

void Graphics_updateWs (I) {
	/*
	 * A function that invalidates the graphics.
	 * This function is typically called by the owner of the drawing area
	 * whenever the data to be displayed in the drawing area has changed;
	 * the idea is to generate an expose event to which the drawing area will
	 * respond by redrawing its contents from the (changed) data.
	 */
	iam (Graphics);
	if (me && my screen) {
		iam (GraphicsScreen);
		#if gtk
			GdkWindow *window = gtk_widget_get_parent_window (my drawingArea);
			GdkRectangle rect;

			if (my x1DC < my x2DC) {
				rect.x = my x1DC;
				rect.width = my x2DC - my x1DC;
			} else {
				rect.x = my x2DC;
				rect.width = my x1DC - my x2DC;
			}

			if (my y1DC < my y2DC) {
				rect.y = my y1DC;
				rect.height = my y2DC - my y1DC;
			} else {
				rect.y = my y2DC;
				rect.height = my y1DC - my y2DC;
			}

			gdk_window_invalidate_rect (window, & rect, true);
			gdk_window_process_updates (window, true);
		#elif xwin
			XClearArea (my display, my window, 0, 0, 0, 0, True);
		#elif win
			//clear (me); // lll
			if (my window) InvalidateRect (my window, NULL, TRUE);
		#elif mac
			Rect r;
			if (my drawingArea) GuiMac_clipOn (my drawingArea);   // to prevent invalidating invisible parts of the canvas
			SetRect (& r, my x1DC, my y1DC, my x2DC, my y2DC);
			SetPort (my macPort);
			/*EraseRect (& r);*/
			InvalWindowRect (GetWindowFromPort ((CGrafPtr) my macPort), & r);
			if (my drawingArea) GuiMac_clipOff ();
		#endif
	}
}

class_methods (GraphicsScreen, Graphics)
	class_method (destroy)
class_methods_end

static int GraphicsScreen_init (GraphicsScreen me, void *voidDisplay, unsigned long voidWindow, int resolution) {

	/* Fill in new members. */

	#if cairo
		_Graphics_text_init (me);
		my resolution = 100;
		my cr = NULL;
	#elif xwin
		if (! inited) {
			int i;
			display = (Display *) voidDisplay;
			xscreen = DefaultScreen (display);
			rootWindow = RootWindow (display, xscreen);
			visual = DefaultVisual (display, xscreen);
			depth = DefaultDepth (display, xscreen);
			colourMap = DefaultColormap (display, xscreen);
			resolution = 100; // floor (25.4 * (double) DisplayWidth (display, xscreen) / DisplayWidthMM (display, xscreen) + 0.5);
			/*if (resolution >= 90) resolution = 100; else resolution = 75;*/
			/*Melder_casual ("nformats %d, depth %d, pad %d", ((_XPrivDisplay) display) -> nformats, depth, BitmapPad (display));*/
			for (i = 0; i < ((_XPrivDisplay) display) -> nformats; i ++) {
				ScreenFormat *format = & ((_XPrivDisplay) display) -> pixmap_format [i];
				/*Melder_casual ("depth %d, bpp %d, pad %d", format -> depth, format -> bits_per_pixel,
					format -> scanline_pad);*/
				if (format -> depth == depth) {
					theBitsPerPixel = format -> bits_per_pixel;
					thePad = format -> scanline_pad;
				}
			}
			inited = 1;
		}
		my display = display;
		my xscreen = xscreen;
		my rootWindow = rootWindow;
		my visual = visual;
		my depth = depth;
		my colourMap = colourMap;
		my resolution = resolution;
		my bitsPerPixel = theBitsPerPixel;
		my pad = thePad;
		my text. window = my window = (Window) voidWindow;
		_Graphics_colour_init (me);
		XSetLineAttributes (my display, my gc, 0, LineSolid, CapButt, JoinBevel);
	#elif win
		if (my printer) {
			my dc = (HDC) voidWindow;
		} else if (voidDisplay) {
			my dc = (HDC) voidDisplay;
			my metafile = TRUE;
		} else {
			my window = (HWND) voidWindow;
			my dc = GetDC (my window);   // window must have a constant display context; see XtInitialize ()
		}
		Melder_assert (my dc != NULL);
		my resolution = resolution;
		SetBkMode (my dc, TRANSPARENT);   // not the default!
		/*
		 * Create pens and brushes.
		 */
		my pen = CreatePen (PS_SOLID, 0, RGB (0, 0, 0));
		my brush = CreateSolidBrush (RGB (0, 0, 0));
		SelectBrush (my dc, GetStockBrush (NULL_BRUSH));
		SetTextAlign (my dc, TA_LEFT | TA_BASELINE | TA_NOUPDATECP);   // baseline is not the default!
		_Graphics_text_init (me);
	#elif mac
		(void) voidDisplay;
		my macPort = (GrafPtr) voidWindow;
		GetQDGlobalsBlack (& my macPattern);
		my macColour = theBlackColour;
		my resolution = resolution;
		my depth = my resolution > 150 ? 1 : 8;   /* BUG: replace by true depth (1=black/white) */
		if (my useQuartz) {
			(void) my macGraphicsContext;
		}
		_Graphics_text_init (me);
	#endif
	return 1;
}

Graphics Graphics_create_screen (void *display, unsigned long window, int resolution) {
	GraphicsScreen me = new (GraphicsScreen);
	my screen = true;
	my yIsZeroAtTheTop = true;
	if (! Graphics_init (me)) return 0;
	Graphics_setWsViewport ((Graphics) me, 0, 100, 0, 100);
	#ifdef macintosh
		GraphicsScreen_init (me, display, (unsigned long) GetWindowPort ((WindowRef) window), resolution);
	#else
		GraphicsScreen_init (me, display, window, resolution);
	#endif
	return (Graphics) me;
}

#ifdef macintosh
Graphics Graphics_create_port (void *display, unsigned long port, int resolution) {
	GraphicsScreen me = new (GraphicsScreen);
	my screen = true;
	my yIsZeroAtTheTop = true;
	if (! Graphics_init (me)) return 0;
	Graphics_setWsViewport ((Graphics) me, 0, 100, 0, 100);
	GraphicsScreen_init (me, display, (unsigned long) port, resolution);
	return (Graphics) me;
}
#endif

Graphics Graphics_create_screenPrinter (void *display, unsigned long window) {
	GraphicsScreen me = new (GraphicsScreen);
	my screen = true;
	my yIsZeroAtTheTop = true;
	my printer = true;
	if (! Graphics_init (me)) return 0;
	my paperWidth = (double) thePrinter. paperWidth / thePrinter. resolution;
	my paperHeight = (double) thePrinter. paperHeight / thePrinter. resolution;
	my x1DC = my x1DCmin = thePrinter. resolution / 2;
	my x2DC = my x2DCmax = (my paperWidth - 0.5) * thePrinter. resolution;
	my y1DC = my y1DCmin = thePrinter. resolution / 2;
	my y2DC = my y2DCmax = (my paperHeight - 0.5) * thePrinter. resolution;
	#if win
		/*
		 * Map page coordinates to paper coordinates.
		 */
		my x1DC -= GetDeviceCaps ((HDC) window, PHYSICALOFFSETX);
		my x2DC -= GetDeviceCaps ((HDC) window, PHYSICALOFFSETX);
		my y1DC -= GetDeviceCaps ((HDC) window, PHYSICALOFFSETY);
		my y2DC -= GetDeviceCaps ((HDC) window, PHYSICALOFFSETY);
	#endif
	Graphics_setWsWindow ((Graphics) me, 0, my paperWidth - 1.0, 13.0 - my paperHeight, 12.0);
	GraphicsScreen_init (me, display, window, thePrinter. resolution);
	return (Graphics) me;
}

#if mac
/* Drawing areas support resize callbacks.
 * However, Mac drawing areas also support move callbacks.
 * This is the only way for a graphics driver to get notified of a move,
 * which would bring about a change in device coordinates.
 * On Xwin and Win, we are not interested in moves, because we draw in widget coordinates.
 */
static void cb_move (GUI_ARGS) {
	iam (GraphicsScreen);
	Dimension width, height, marginWidth, marginHeight;
	XtVaGetValues (w, XmNwidth, & width, XmNheight, & height,
		XmNmarginWidth, & marginWidth, XmNmarginHeight, & marginHeight, NULL);

	/* The four values returned are probably equal to the previous ones.
	 * However, the following call forces a new computation of the device coordinates
	 * by widgetToWindowCoordinates ().
	 */

	Graphics_setWsViewport ((Graphics) me, marginWidth /* Left x value in widget coordinates */,
		width - marginWidth, marginHeight, height - marginHeight);
	Graphics_updateWs ((Graphics) me);
}
static bool _GraphicsMacintosh_tryToInitializeQuartz (void) {
	return _GraphicsMac_tryToInitializeAtsuiFonts ();
}
#endif

Graphics Graphics_create_xmdrawingarea (void *w) {   /* w = XmDrawingArea widget */
	GraphicsScreen me = new (GraphicsScreen);
	#if gtk
		GtkRequisition realsize;
	#elif motif
		Dimension width, height, marginWidth, marginHeight;
	#endif

	my drawingArea = w;   /* Now !!!!!!!!!! */
	my screen = true;
	my yIsZeroAtTheTop = true;
	#ifdef macintosh
		my useQuartz = _GraphicsMacintosh_tryToInitializeQuartz ();
	#endif
	if (! Graphics_init (me)) return 0;
	#ifdef macintosh
		GraphicsScreen_init (me, XtDisplay (w), (unsigned long) GetWindowPort ((WindowRef) XtWindow (w)), Gui_getResolution (w));
	#else
		#if gtk
			GraphicsScreen_init (me, GTK_WIDGET (w), (unsigned long) GTK_WIDGET (w), Gui_getResolution(w));
		#elif motif
			GraphicsScreen_init (me, XtDisplay (w), (unsigned long) XtWindow (w), Gui_getResolution (w));
		#endif
	#endif

	#if gtk
		gtk_widget_size_request (my drawingArea, &realsize);
		// HIER WAS IK
		//	g_debug("--> %d %d", realsize.width, realsize.height);
		Graphics_setWsViewport ((Graphics) me, 0, realsize.width, 0, realsize.height);
	#elif motif
		XtVaGetValues (w, XmNwidth, & width, XmNheight, & height,
		XmNmarginWidth, & marginWidth, XmNmarginHeight, & marginHeight, NULL);
		Graphics_setWsViewport ((Graphics) me,
			marginWidth, width - marginWidth, marginHeight, height - marginHeight);
	#endif
	#ifdef macintosh
		XtAddCallback (w, XmNmoveCallback, cb_move, (XtPointer) me);
	#endif
	return (Graphics) me;
}

#if cairo
	void *Graphics_x_getCR (I) {
		iam (GraphicsScreen);
		return my cr;
	}
	void Graphics_x_setCR (I, void *cr) {
		iam (GraphicsScreen);
		my cr = cr;
	}
#endif

#if xwin
	void *Graphics_x_getGC (I) {
		iam (GraphicsScreen);
		return my gc;
	}
#endif

/* End of file GraphicsScreen.c */
