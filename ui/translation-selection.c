#include <stdio.h>
#include <ncurses.h>
#include "../util/store.h"
#include "../util/db.h"
#include "translation-selection.h"
#include "bible-display.h"

WINDOW *win;
int currTranslation = 0;

// Setup window and show first translation
void translation_selection(void)
{
    win = newwin(1, 5, LINES - 2, COLS - 5);
    keypad(win, true);

    wprintw(win, "%s", get_translation(0));
    wrefresh(win);
}

void change_translation(bool next)
{
    wclear(win), wmove(win, 0, 0);

    int maxIndex = get_translations() - 1;

    currTranslation += (next ? 1 : -1);
    if (currTranslation > maxIndex)
	{
        currTranslation = 0;
	}

    else if (currTranslation < 0)
	{
        currTranslation = maxIndex;
	}

    wprintw(win, "%s", get_translation(currTranslation));
    wrefresh(win);
    
	// Open db of current translation
    if (open_bible_db(currTranslation) == false)
	{
		display_bible_error("Failed to switch to selected translation.\n"
			"Press [TAB] to try another translation");
	}
}

void close_translation(void)
{
    delwin(win);
}