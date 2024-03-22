/*
 * xhextris Copyright 1990 David Markley, dm3e@+andrew.cmu.edu, dam@cs.cmu.edu
 *
 * Permission to use, copy, modify, and distribute, this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders be used in
 * advertising or publicity pertaining to distribution of the software with
 * specific, written prior permission, and that no fee is charged for further
 * distribution of this software, or any modifications thereof.  The copyright
 * holder make no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA, PROFITS, QPA OR GPA, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* This file contains the X I/O handling routines for hextris.
 */

#include <stdio.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <pwd.h>

#ifdef LOG
#include <system.h>
#endif

#include "header.h"

/* Macros to make 4.2 BSD select compatible with 4.3 BSD select */
#ifndef FD_SET
#define fd_set int
#define FD_SET(fd,fdset) (*(fdset) |= (1<<(fd)))
#define FD_CLR(fd,fdset) (*(fdset) &= ~(1<<(fd)))
#define FD_ISSET(fd, fdset) (*(fdset) & (1<<(fd)))
#define FD_ZERO(fdset) (*(fdset) = 0)
#endif

/* I dislike global variables, but this made it much simpler */
Display *display;
int screen;
Window win;
GC gc, hexgc;
XFontStruct *font_info, *hexfont_info;
Pixmap black, white;
#ifdef AFS
extern int PlayerUID;
#endif

/* This is the big, ugly main X procedure...
 */
void main(argc, argv)
int argc;
char **argv;
{
    char buffer[20];
    int width, height, i, bufsize=20, inverse=0, pleasure=0, window_size = 0;
    XSizeHints size_hints;
    XEvent report;
    KeySym key;
    XComposeStatus compose;
    struct timeval tp, ltp;
    struct timezone tzp;
    double intvl = 0, newintvl;
    fd_set fdst;
    struct passwd  *pwent;
/* The following variables are required by hextris */
    int score = 0, rows = 0, game_over = 1, game_view = 1, oldscore = 0;
    high_score_t high_scores[MAXHIGHSCORES];
    position_t grid[MAXROW][MAXCOLUMN];
    piece_t npiece, piece;
    char *name, *log_name;
    
#ifdef AFS
    Authenticate();
    bePlayer();
    pwent = getpwuid(PlayerUID);
    if (pwent == (struct passwd *) NULL) {
      if ((log_name = (char *)getenv("USER")) == NULL)
	log_name = "anon";
    } else
      log_name = pwent->pw_name;
#else
    pwent = getpwuid(getuid());
    if (pwent == (struct passwd *) NULL) {
      if ((log_name = (char *)getenv("USER")) == NULL)
	log_name = "anon";
    } else
      log_name = pwent->pw_name;
#endif
    for (i = 1; i < argc; i++) {
	if (! strcmp(argv[i],"-rv")) {
	    inverse = 1;
	    continue;
	}
	if (! strcmp(argv[i],"-p"))
	  pleasure = 1;
    }
    if ((name = (char *)getenv("XHEXNAME")) == NULL)
      name = log_name;
    printf("\nWelcome, %s...\n",name);
    gettimeofday(&tp, &tzp);
    srandom((int)(tp.tv_usec));
#ifdef LOG
    strcpy(log_message,log_name);
    strcat(log_message,"\t");
    strcat(log_message,SYS_NAME);
    strcat(log_message,"\t1.00");
#endif
    set_up_display(inverse);
    set_up_window(&width,&height,&size_hints,argv,argc);
    XSync(display,True);
    while(1) {
	if (pleasure) {
	    score = 0;
	    intvl = 400000;
	} else
	  intvl = 100000+(200000-((rows > 40) ? 20 : (rows/2))*10000);
	oldscore = score;
	if (! game_over)
	  if ((game_over = update_drop(grid,&npiece,&piece,&score,&rows))) {
	      read_high_scores(high_scores);
	      if (is_high_score(name, log_name, score, rows, high_scores))
		write_high_scores(high_scores,log_name);
	      read_high_scores(high_scores);
	  }
	if (score != oldscore) {
	    XSync(display,True);
	    intvl = 400000;
	}
	gettimeofday(&ltp, NULL);
	while (1) {
	    gettimeofday(&tp, NULL);
	    newintvl = intvl - (((tp.tv_sec - ltp.tv_sec)*1000000)+
	      (tp.tv_usec - ltp.tv_usec));
	    if (newintvl <= 0)
	      break;
	    tp.tv_sec = 0;
	    tp.tv_usec = newintvl;
	    FD_ZERO(&fdst);
	    FD_SET(ConnectionNumber(display),&fdst);
	    select(ConnectionNumber(display)+1,&fdst,0,0,&tp);
	    while (XPending(display)) {
		XNextEvent(display, &report);
		switch (report.type) {
		case Expose:
		    while (XCheckTypedEvent(display, Expose, &report));
		    if (window_size)
		      TooSmall();
		    redraw_game(grid,&npiece,&piece,&score,&rows,game_view,
				high_scores);
		    break;
		case ConfigureNotify:
		    while (XCheckTypedEvent(display,ConfigureNotify,&report));
		    width = report.xconfigure.width;
		    height = report.xconfigure.height;
		    if ((width < size_hints.min_width) ||
			(height < size_hints.min_height))
		      window_size = 1;
		    else
		      window_size = 0;
		    redraw_game(grid,&npiece,&piece,&score,&rows,game_view,
				high_scores);
		    break;
		case EnterNotify:
		    break;
		case LeaveNotify:
		    break;
		case ButtonPress:
		    XBell(display,50);
		    break;
		case KeyPress:
		    XLookupString(&report, buffer, bufsize, &key, &compose);
		    oldscore = score;
		    do_choice(buffer[0],grid,&npiece,&piece,&score,&rows,
			      &game_over, &game_view, high_scores);
		    if ((score != oldscore) || (! score)) {
			XSync(display,True);
			intvl = 400000;
			gettimeofday(&ltp, NULL);
		    }
		    break;
		default:
		    break;
		}
	    }
	}
    }
}

/* This sets up the basic connections to the X server, the fonts, and
 * which colors are to be foreground and background.
 */
set_up_display(inverse)
int inverse;
{
    Pixmap temp;

    if ( (display = XOpenDisplay(NULL)) == NULL) {
	    fprintf(stderr, "xhextris: cannot connect to X server.\n");
	    exit(-1);
	}
    screen = DefaultScreen(display);
    black = BlackPixel(display, screen);
    white = WhitePixel(display, screen);
    if (inverse) {
	temp = black;
	black = white;
	white = temp;
    }
    set_font_path(HEXFONTDIR);
    set_font_path(FONTDIR);
}

/* This sets up the font path to contain the directories that have the
 * fonts this program needs.
 */
set_font_path(fontdir)
char *fontdir;
{
    int i, font_length;
    char **font_path = XGetFontPath(display, &font_length);

    for (i = 0; (i < font_length) && strcmp(font_path[i],fontdir); i++);

    if (i >= font_length) {
	char **new_font_path;

	if (new_font_path = (char **) malloc((font_length+1)*sizeof(char *))) {
	    for(i = 0; i < font_length; i++)
	      new_font_path[i] = font_path[i];
	    new_font_path[i] = fontdir;
	    XSetFontPath(display, new_font_path, font_length + 1);
	    free(new_font_path);
	}
    }
    if (font_path)
      XFreeFontPath(font_path);
}

/* This sets up the window position, size, fonts, and gcs.
 */
set_up_window(width,height,size_hints,argv,argc)
int *width, *height;
XSizeHints *size_hints;
char *argv[];
int argc;
{
    *width = MAXCOLUMN*40;
    *height = (MAXROW+4)*26;
    win = XCreateSimpleWindow(display,RootWindow(display, screen),50,50,
			      *width, *height, 4, black, white);
    size_hints->flags = PPosition | PSize | PMinSize;
    size_hints->x = 50;
    size_hints->y = 50;
    size_hints->width = *width;
    size_hints->height = *height;
    size_hints->min_width = 300;
    size_hints->min_height = 700;
    XSetStandardProperties(display, win, WINDOWNAME, ICONNAME, NULL, argv,
			   argc, size_hints);
    XSelectInput(display, win, ExposureMask | KeyPressMask | ButtonPressMask |
		 StructureNotifyMask | EnterWindowMask | LeaveWindowMask);
    load_font(&font_info, FONTNAME);
    get_GC(win, &gc, font_info);
    load_font(&hexfont_info, HEXFONTNAME);
    get_GC(win, &hexgc, hexfont_info);
    XMapWindow(display, win);
    XFlush(display);
}

/* This sets up a gc
 */
get_GC(win, tgc, tfont_info)
Window win;
GC *tgc;
XFontStruct *tfont_info;
{
    unsigned long valuemask = 0;
    XGCValues values;
    unsigned int line_width = 2;
    int line_style = LineSolid;
    int cap_style = CapRound;
    int join_style = JoinRound;
    int dash_offset = 0;
    static char dash_list[] = { 12, 24 };
    int list_length = 2;

    *tgc = XCreateGC(display, win, valuemask, &values);
    XSetFont(display, *tgc, tfont_info->fid);
    XSetForeground(display, *tgc, black);
    XSetLineAttributes(display, *tgc, line_width, line_style, cap_style,
		   join_style);
    XSetDashes(display, *tgc, dash_offset, dash_list, list_length);
}

/* This loads a font
 */
load_font(tfont_info, font_name)
XFontStruct **tfont_info;
char *font_name;
{
    if ((*tfont_info = XLoadQueryFont(display, font_name)) == NULL) {
	(void)fprintf(stderr, "xhextris: Cannot open %s font.\n",font_name);
	exit(-1);
    }
}

/* This yells if the window is too small.
 */
TooSmall()
{
    char *string1 = "Too Small";
    int y_offset, x_offset;

    y_offset = font_info->max_bounds.ascent + 2;
    x_offset = 2;
    XDrawString(display, win, gc, x_offset, y_offset, string1,
		strlen(string1));
}

/* This is required by hextris!
 *
 * This clears the window.
 */
clear_display()
{
    XClearWindow(display,win);
}

/* This is required by hextris!
 *
 * This displays the current score and rows completed.
 */
display_scores(score,rows)
int *score, *rows;
{
    int y_offset, x_offset;
    char scores[40];

    XSetFillStyle(display, gc, FillSolid);
    sprintf(scores,"Score: %6d", *score);
    y_offset = 250;
    x_offset = (MAXCOLUMN + 5) * 20;
    XClearArea(display,win,x_offset,y_offset-20,MAXCOLUMN*20, 50, False);
    XDrawString(display, win, gc, x_offset, y_offset, scores,strlen(scores));
    sprintf(scores,"Rows: %3d", *rows);
    y_offset += 20;
    XDrawString(display, win, gc, x_offset, y_offset, scores,strlen(scores));
    XFlush(display);
}

/* This is required by hextris!
 *
 * This displays the help information.
 */
display_help()
{
    int y_offset, x_offset, i;
    static char *message[] = { "The keys to press are:",
				 "J,j,4 - move left.",
				 "L,l,6 - move right.",
				 "K,k,5 - rotate ccw.",
				 "I,i,8 - rotate cw.",
				 "space,0 - drop.",
				 "N,n - new game.",
				 "P,p - pause game.",
				 "U,u - unpause game.",
				 "R,r - redisplay game.",
				 "H,h - show high scores.",
				 "G,g - show game.",
				 "Q,q - quit game.",
				 " ",
				 "--------------------",
				 "Created By:",
				 "  David Markley",
				 "Font By:",
				 "  Jon Slenk" };


    XSetFillStyle(display, gc, FillSolid);
    y_offset = 315;
    x_offset = (MAXCOLUMN + 4) * 20;
    for (i = 0; i < 19; i++)
	XDrawString(display, win, gc, x_offset, y_offset+(i*17), message[i],
		    strlen(message[i]));
    XFlush(display);
}

/* This is required by hextris!
 *
 * This displays the high score list.
 */
display_high_scores(high_scores)
high_score_t high_scores[MAXHIGHSCORES];
{
    int y_offset, i;
    static int x_offset[5] = {5,30,150,200,300};
    static char *header[] = {"#","Name","UID","Score","Rows"};
    char message[40];

    XClearWindow(display,win);
    XSetFillStyle(display, gc, FillSolid);
    y_offset = 40;
    for (i = 0; i < 5; i++)
      XDrawString(display, win, gc, x_offset[i], y_offset,
		  header[i],strlen(header[i]));
    y_offset = 60;
    for (i = 0; i < ((MAXHIGHSCORES > 40) ? 40 : MAXHIGHSCORES); i++) {
	itoa(i+1,message);
	XDrawString(display, win, gc, x_offset[0], y_offset+(i*17),
		    message,strlen(message));
	XDrawString(display, win, gc, x_offset[1], y_offset+(i*17),
		    high_scores[i].name,strlen(high_scores[i].name));
	XDrawString(display, win, gc, x_offset[2], y_offset+(i*17),
		    high_scores[i].userid,strlen(high_scores[i].userid));
	itoa(high_scores[i].score,message);
	XDrawString(display, win, gc, x_offset[3], y_offset+(i*17),
		    message,strlen(message));
	itoa(high_scores[i].rows,message);
	XDrawString(display, win, gc, x_offset[4], y_offset+(i*17),
		    message,strlen(message));
    }
    XFlush(display);
}

/* This is required by hextris!
 *
 * This displays the next piece to be dropped.
 */
show_next_piece(npiece)
piece_t *npiece;
{
    piece_t tpiece;

    tpiece.type = npiece->type;
    tpiece.rotation = npiece->rotation;
    tpiece.row = 4;
    tpiece.column = MAXCOLUMN+6;
    XClearArea(display,win,(MAXCOLUMN+3)*20,0,200,230, False);
    init_piece(&tpiece);
    XFlush(display);
}

/* This is required by hextris!
 *
 * This draws one hex at the specified row and column specified.
 */
draw_hex(row,column,fill,type)
int row,column,fill,type;
{
    int y_offset, x_offset;
    char hex[2];

    x_offset = 20 + column * 20;
    y_offset = 20 + row * 26 + (column & 1) * 13;
    hex[0] = 'b' + type;
    hex[1] = '\0';
    if (fill) {
	XSetForeground(display,hexgc,black);
    } else {
	XSetForeground(display,hexgc,white);
	strcpy(hex,"a");
    }
    XDrawString(display, win, hexgc, x_offset, y_offset, hex, strlen(hex));
    XFlush(display);
}

/* This is required by hextris!
 *
 * This ends the game by closing everything down and exiting.
 */
end_game()
{
	XFreeGC (display, gc);
	XFreeGC (display, hexgc);
	XDestroyWindow (display, win);
	XCloseDisplay (display);
	exit(0);
}
