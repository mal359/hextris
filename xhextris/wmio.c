/*
 * hextris Copyright 1990 David Markley, dm3e@+andrew.cmu.edu, dam@cs.cmu.edu
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

/* This file contains the Andrew Window Manager I/O handling routines
 * for hextris.
 */

#include <wmclient.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <system.h>
#include "header.h"

/* Macros to make 4.2 BSD select compatible with 4.3 BSD select */
#ifndef FD_SET
#define fd_set int
#define FD_SET(fd,fdset) (*(fdset) |= (1<<(fd)))
#define FD_CLR(fd,fdset) (*(fdset) &= ~(1<<(fd)))
#define FD_ISSET(fd, fdset) (*(fdset) & (1<<(fd)))
#define FD_ZERO(fdset) (*(fdset) = 0)
#endif

program(hextris);

struct font *hexfont, *basefont;
int Black, White;
/* The following variables are required by hextris */
int score = 0, rows = 0, game_over = 1, game_view = 1;
high_score_t high_scores[MAXHIGHSCORES];
position_t grid[MAXROW][MAXCOLUMN];
piece_t npiece, piece;
char *name, *log_name;

FlagRedraw() {
    wm_ClearWindow();
    redraw_game(grid,&npiece,&piece,&score,&rows,game_view,
		high_scores);
}

main(argc,argv)
int argc;
char **argv;
{
    int i, inverse = 0, pleasure = 0;
    struct timeval tp, ltp;
    struct timezone tzp;
    double intvl = 0, newintvl;
    fd_set fdst;
    
#ifdef AFS
    Authenticate();
    bePlayer();
#endif
    for (i = 1; i < argc; i++) {
	if (! strcmp(argv[i],"-i")) {
	    inverse = 1;
	    continue;
	}
	if (! strcmp(argv[i],"-p"))
	  pleasure = 1;
    }
    if (inverse) {
	Black = f_white;
	White = f_black;
    } else {
	Black = f_black;
	White = f_white;
    }
    if ((log_name = (char *)getenv("USER")) == NULL)
      log_name = "anon";
    if ((name = (char *)getenv("XHEXNAME")) == NULL)
      name = log_name;
    printf("\nWelcome, %s...\n",name);
    gettimeofday(&tp, &tzp);
    srandom((int)(tp.tv_usec));
#ifdef LOG
    strcpy(log_message,log_name);
    strcat(log_message,"\t");
    strcat(log_message,SYS_NAME);
    strcat(log_message,"\t0.98\twm");
#endif

    if (wm_NewWindow(0) == NULL || fork())
      exit(0); 
    wm_EnableInput();
    wm_SetRawInput();
    wm_AddMenu("Hextris,New Game:N");
    wm_AddMenu("Hextris,Quit:Q");
    basefont = wm_DefineFont("Andy12b");
   
hexfont=wm_DefineFont("/afs/andrew.cmu.edu/usr0/games/fonts/wm/hex10.fwm");
    clear_display();
    redraw_game(grid,&npiece,&piece,&score,&rows,game_view,high_scores);
    fflush(winout);

    while(1) {
	if (! game_over)
	  if ((game_over = update_drop(grid,&npiece,&piece,&score,&rows))) {
	      read_high_scores(high_scores);
	      if (is_high_score(name, log_name, score, rows, high_scores))
		write_high_scores(high_scores,log_name);
	      read_high_scores(high_scores);
	  }
	gettimeofday(&ltp, NULL);
	if (pleasure) {
	    score = 0;
	    intvl = 400000;
	} else
	  intvl = 100000+(200000-((rows > 40) ? 20 : (rows/2))*10000);
	while (1) {
	    gettimeofday(&tp, NULL);
	    newintvl = intvl - (((tp.tv_sec - ltp.tv_sec)*1000000)+
	      (tp.tv_usec - ltp.tv_usec));
	    if (newintvl <= 0)
	      break;
	    tp.tv_sec = 0;
	    tp.tv_usec = newintvl;
	    FD_ZERO(&fdst);
	    FD_SET(fileno(winin),&fdst);
	    if(select(fileno(winin)+1,&fdst,0,0,&tp)) {
	      char tmp[1] = getc(winin);
	      do_choice(tmp,grid,&npiece,&piece,&score,&rows,
			&game_over,&game_view,high_scores);
	    }

	}
    }
}

/* This is required by hextris!
 *
 * This clears the window.
 */
clear_display()
{
    wm_ClearWindow();
}

/* This is required by hextris!
 *
 * This displays the current score and rows completed.
 */
display_scores(score,rows)
int *score, *rows;
{
    int x,y;
    char scores[40];

    wm_SelectFont(basefont);
    y = 250;
    x = (MAXCOLUMN + 5) * 20;
    wm_SetFunction(White);
    wm_RasterSmash(x,y-20,MAXCOLUMN*20,50);
    wm_SetFunction(Black);
    sprintf(scores,"Score: %6d", *score);
    wm_DrawString(x,y,wm_AtLeft | wm_AtBaseline,scores);
    sprintf(scores,"Rows: %3d", *rows);
    y += 20;
    wm_DrawString(x,y,wm_AtLeft | wm_AtBaseline,scores);
    fflush(winout);
}

/* This is required by hextris!
 *
 * This displays the help information.
 */
display_help()
{
    int y, x, i;
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


    y = 315;
    x = (MAXCOLUMN + 4) * 20;
    wm_SelectFont(basefont);
    wm_SetFunction(Black);
    for (i = 0; i < 19; i++)
      wm_DrawString(x,y+(i*17),wm_AtLeft | wm_AtBaseline,message[i]);
    fflush(winout);
}

/* This is required by hextris!
 *
 * This displays the high score list.
 */
display_high_scores(high_scores)
high_score_t high_scores[MAXHIGHSCORES];
{
    int y, i;
    static int x[5] = {5,30,150,200,300};
    static char *header[] = {"#","Name","UID","Score","Rows"};
    char message[40];

    wm_ClearWindow();
    wm_SelectFont(basefont);
    wm_SetFunction(Black);
    y = 40;
    for (i = 0; i < 5; i++)
      wm_DrawString(x[i],y,wm_AtLeft | wm_AtBaseline,header[i]);
    y = 60;
    for (i = 0; i < ((MAXHIGHSCORES > 40) ? 40 : MAXHIGHSCORES); i++) {
	itoa(i+1,message);
	wm_DrawString(x[0],y+(i*17),wm_AtLeft | wm_AtBaseline,message);
	wm_DrawString(x[1],y+(i*17),wm_AtLeft | wm_AtBaseline,
		      high_scores[i].name);
	wm_DrawString(x[2],y+(i*17),wm_AtLeft | wm_AtBaseline,
		      high_scores[i].userid);
	itoa(high_scores[i].score,message);
	wm_DrawString(x[3],y+(i*17),wm_AtLeft | wm_AtBaseline,message);
	itoa(high_scores[i].rows,message);
	wm_DrawString(x[4],y+(i*17),wm_AtLeft | wm_AtBaseline,message);
    }
    fflush(winout);
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
    wm_SetFunction(White);
    wm_RasterSmash((MAXCOLUMN+3)*20,0,200,230);
    wm_SetFunction(Black);
    init_piece(&tpiece);
    fflush(winout);
}

/* This is required by hextris!
 *
 * This draws one hex at the specified row and column specified.
 */
draw_hex(row,column,fill,type)
int row,column,fill,type;
{
    int x,y;
    char hex[2];

    x = 20 + column * 20;
    y = 20 + row * 26 + (column & 1) * 13;
    hex[0] = 'b' + type;
    hex[1] = '\0';
    if (fill) {
	wm_SetFunction(Black);
    } else {
	wm_SetFunction(White);
	strcpy(hex,"a");
    }
    wm_SelectFont(hexfont);
    wm_DrawString(x,y,wm_AtLeft | wm_AtBaseline,hex);
    fflush(winout);
}

/* This is required by hextris!
 *
 * This ends the game by closing everything down and exiting.
 */
end_game()
{
    exit(0);
}
