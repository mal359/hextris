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

#include <stdio.h>
#include "header.h"

int
main()
{
    high_score_t high_scores[MAXHIGHSCORES];
    int i;

    read_high_scores(high_scores);
    printf("%-3s %-40s %-5s %-10s %-3s\n", "#", "Name", "UID", "Score", "Row");
    for (i = 0; i < MAXHIGHSCORES; i++)
      printf("%3d %-40s %-5s %10d %3d\n", i+1, high_scores[i].name,
	     high_scores[i].userid, high_scores[i].score,
	     high_scores[i].rows);
    return 0;
}
