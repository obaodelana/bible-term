#include <stdio.h>
#include <ctype.h>
#include <ncurses.h>
#include <locale.h>
#include "util/db.h"
#include "components/input-field.h"
#include "ui/bible-display.h"
#include "util/store.h"
#include "ui/translation-selection.h"

static size_t bookInf, chapterInf, verseInf;

static char book[20] = "Genesis";
static int chapter = 1, verse = 1;

static bool book_callback(const char *);
static bool chapter_callback(float);
static bool verse_callback(float);

static void load_bible_path(int argCount, char **args);

static void hor_nav(bool right);

// TODO: Add blinking cursor
// TODO: Add search
// TODO: Refactor and document code

int main(int argc, char **argv)
{
    setlocale(LC_CTYPE, ""); // enable UTF-8
    initscr();
    noecho(); // Don't show user input
    cbreak(); // Make input immediately available to program (but still process signals)
    curs_set(FALSE); // Disable cursor
    keypad(stdscr, true); // Allow function and arrow keys and mouse
    // Capture mouse movement and check for left mouse button click
    mousemask(REPORT_MOUSE_POSITION | BUTTON1_CLICKED, NULL);
    use_default_colors(); // Allows transparent color pairs
    start_color(); // Enable colours

    // Colour pairs (id, fg, bg)
    init_pair(RED_COLOUR, COLOR_RED, -1 /* no bg*/);

    refresh();

    open_bible_db(0);
   
    bookInf = inf_new_text
    (
        (Rect) {.w = COLS / 2, .h = 1, .x = 1, .y = LINES - 3},
        "Genesis",
        INF_LETTERS | INF_NUMBERS,
        "",
        &book_callback
    );
    chapterInf = inf_new_number
    (
        (Rect) {.w = COLS / 5, .h = 1, .x = COLS / 2 + 1, .y = LINES - 3},
        "1",
        1,
        150,
        &chapter_callback
    );
    verseInf = inf_new_number
    (
        (Rect) {.w = COLS / 5, .h = 1, .x = COLS / 2 + COLS / 5 + 1, .y = LINES - 3},
        "1",
        1,
        176,
        &verse_callback
    );

    translation_selection();

    init_bible();
    
    load_bible_path(argc, argv);

    int c;
    while(tolower(c = getch()) != 'q')
    {
        if (c == KEY_UP || c == KEY_DOWN)
            scroll_bible(c == KEY_UP);
        else if (c == KEY_LEFT || c == KEY_RIGHT)
            hor_nav(c == KEY_RIGHT);
        else if (c == '\t' || c == 353 /* shift-tab */)
        {
            change_translation(c == '\t');
            
            reset_bible_start();
            if (store_bible_text(book, chapter, verse))
                display_bible(verse);
        }

        inf_update(c);
    }

    inf_cleanup();
    close_bible();
    close_translation();
    close_db();

    endwin();

    return 0;
}

static void load_bible_path(int argCount, char **args)
{
    if (argCount == 3 && isdigit(args[2][0]))
    {
        strncpy(book, args[1], sizeof(book) - 1);
        sscanf(args[2], "%d%*c%d", &chapter, &verse);
    }

    else
    {
        get_stored_path(book, &chapter, &verse);
    }

    if (strlen(book) > 0 && chapter > 0 && verse > 0)
    {
        int maxChapter = get_max_chapter(book);
        if (maxChapter > 0 && chapter <= maxChapter)
        {
            int verseCount = get_no_of_verses(book, chapter);
            if (verse <= verseCount)
            {
                if (store_bible_text(book, chapter, verse))
                {
                    inf_set_text_value(bookInf, book);
                    inf_set_number_value(chapterInf, chapter);
                    inf_set_number_value(verseInf, verse);

                    inf_switch_focus(bookInf);

                    display_bible(verse);
                }
            }
        }
    }
}

static bool book_callback(const char *bk)
{
    strncpy(book, bk, 19);

    int maxChapter = get_max_chapter(bk);
    if (maxChapter > 0)
    {
        get_book(book, 0);
        inf_set_text_value(bookInf, book);

        inf_set_number_value(chapterInf, maxChapter);
        inf_switch_focus(chapterInf);

        return true;
    }

    else
    {
        display_bible_error("Couldn't find that book. Check your spelling");
    }

    return false;
}

static bool chapter_callback(float ch)
{
    chapter = ch;

    int maxChapter = get_max_chapter(book);
    if (ch > 0 && ch <= maxChapter)
    {
        int verseCount = get_no_of_verses(book, ch);
        if (verseCount > 0)
        {
            bool bibleStored = store_bible_text(book, ch, 1);
            if (bibleStored)
            {
                inf_set_number_value(verseInf, verseCount);
                inf_switch_focus(verseInf);

                reset_bible_start();
                display_bible(1);
            }
            
            return bibleStored;
        }

        else
        {
            display_bible_error("Chapter not found");
        }
    }

    else
    {
        display_bible_error("This book doesn't have that chapter");
    }

    return false;
}

static bool verse_callback(float v)
{
    verse = v;

    int verseCount = get_no_of_verses(book, chapter);
    if (v > 0 && (int) v <= verseCount)
    {
        inf_switch_focus(bookInf);

        // Store verse to file 
        FILE *bibleStore = fopen(bibleStorePath, "r+b");
        if (bibleStore != NULL)
        {
            fseek(bibleStore, snprintf(NULL, 0, "%s %i:", book, chapter), SEEK_SET);
            fprintf(bibleStore, "%03i", (int) v);
            fclose(bibleStore);
        }

        display_bible(v);

        return true;
    }

    else
    {
        display_bible_error("This book doesn't have that verse");
    }

    return false;
}

static void hor_nav(bool right)
{
    int maxChapters = get_max_chapter(book);
    if (maxChapters <= 0) return;

    // If right arrow key is pressed and current chapter is less than max chapter
    if (right && chapter < maxChapters)
        chapter++;
    // If left arrow key is pressed and current chapter is greater than 1
    else if (!right && chapter > 1)
        chapter--;
    // Called when at the extreme
    // i.e. current chapter is 1 or max chapter
    else
    {
        if (get_book(book, right) == false)
            return;
        chapter = 1;
    }

    if (store_bible_text(book, chapter, verse))
    {
        inf_set_text_value(bookInf, book);
        inf_set_number_value(chapterInf, chapter);
        inf_set_number_value(verseInf, get_no_of_verses(book, chapter));

        inf_switch_focus(verseInf);

        reset_bible_start();
        display_bible(verse);
    }
}