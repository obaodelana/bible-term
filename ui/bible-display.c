#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "bible-display.h"

extern const char bibleStorePath[];

static WINDOW *win = NULL;
static int w = 80, h = 24, startTermLine = 1;

void init_bible(void)
{
    w = COLS - 2, h = LINES - 3;
    win = newwin(h, w, 0, 1);
    
    keypad(win, true);
}

void scroll_bible(bool up)
{
    if (up)
    {
        if (startTermLine > 1)
            startTermLine--;
    }

    else
    {
        startTermLine++;
    }

    display_bible(0);
}

void reset_bible_start(void)
{
    startTermLine = 1;
}

static int get_word(const char *str, char *word)
{
    if (str == NULL || word == NULL)
        return 0;

    if (*str == ' ') str++;
    
    // Clear current [word] content
    memset(word, 0, strlen(word));

    char endChar = ' ';
    if (*str == '<')
        endChar = '>';
    // If there's no space after word
    else if (strchr(str, ' ') == NULL)
    {
        endChar = '\n';
        // If there's no new line after word
        if (strchr(str, '\n') == NULL)
            endChar = '\0';
    }

    int distToChar = strchr(str, endChar) - str;
    if (endChar == '>') distToChar++;

    // Get word
    strncpy(word, str, distToChar);
    word[distToChar] = '\0';

    // If tag is in word
    if (word[0] != '<' && strchr(word, '<') != NULL)
    {
        int indexOfChar = strchr(word, '<') - word;
        int lenOfTag = strlen(word) - indexOfChar;

        // Clear tag part
        memset(&word[indexOfChar], 0, lenOfTag);
    }

    return strlen(word);
}

static inline bool str_equal(const char *s1, const char *s2)
{
    return (strcmp(s1, s2) == 0);
}

static void handle_tag(char* tag)
{
    if (str_equal(tag, "<J>"))
        wattron(win, COLOR_PAIR(RED_COLOUR));
    else if (str_equal(tag, "</J>"))
        wattroff(win, COLOR_PAIR(RED_COLOUR));
    else if (str_equal(tag, "<v>"))
        wattron(win, A_DIM);
    else if (str_equal(tag, "</v>"))
        wattroff(win, A_DIM);
	else if (str_equal(tag, "<b>"))
		wattron(win, A_BOLD);
 	else if (str_equal(tag, "</b>"))
		wattroff(win, A_BOLD);
	// else if (str_equal(tag, "<pb/>"))
    //    wprintw(win, "\n ");
    // else if (str_equal(tag, "<t>"))
    //     waddch(win, '\n');
    // else if (str_equal(tag, "<br/>"))
    //     waddch(win, '\n');
}

void display_bible(int verse)
{
    FILE *bible = fopen(bibleStorePath, "r");
    if (bible != NULL)
    {
        wclear(win), wmove(win, 0, 0);
        wattrset(win, A_NORMAL);
        
        char *line = malloc(1);
        size_t lineSize = 1;

        // Get first line
        getline(&line, &lineSize, bible);

        int cursorY = 0, charCount = 0;
        int currTermLine = 1, currVerse = 1;
        bool canPrint = false;
        char word[25 + 1] = "";
        
        // Get each line in file until end-of-file
        while((getline(&line, &lineSize, bible) != EOF)
            // Stop when cursor reaches window height
            && getcury(win) < h)
        {
            const char *str = line;
            while (*str != '\0' && *str != '\n')
            {
                cursorY = getcury(win);

                int wordLen = get_word(str, word); 
                str += wordLen;

                if ((verse <= 1 && currTermLine >= startTermLine)
                    || verse == currVerse)
                {
                    canPrint = true;
                    if (verse > 1)
                        startTermLine = currTermLine;
                }
                
                // Tag
                if (word[0] == '<' && canPrint)
                {
                    if (str_equal(word, "<f>") || str_equal(word, "<e>"))
                    {
                        str = strchr(str, '>') + 1;
                        if (*str == ' ') str++;
                    }

                    else
                    {
                        handle_tag(word);
                    }
                }

                else
                {
                    charCount += (wordLen + (*str == ' '));
                    if (charCount >= w)
                    {
                        charCount = wordLen + (*str == ' ');
                        currTermLine++;

                        if (canPrint)
                        {
                            waddch(win, '\n');
                            if (getcury(win) == cursorY)
                                goto break2;
                            else
                                cursorY = getcury(win);
                        }
                    }

                    if (canPrint)
                    {
                        wprintw(win, "%s", word);
                        
                        if (getcury(win) != cursorY)
                        {
                            charCount = getcurx(win);
                            currTermLine++;
                        }

                        else if (*str == ' ')
                        {
                            waddch(win, ' ');
                        }
                    }
                }

                if (*str == ' ') str++;
            }

            if (canPrint)
            {
                cursorY = getcury(win);

                wprintw(win, "\n\n");
                
                if (getcury(win) < cursorY + 2)
                    break;
            }


            if (feof(bible))
            {
                startTermLine--;
            }

            else
            {
                currTermLine += 2;
                charCount = 0, currVerse++;
            }
        }

        break2:

        free(line);

        wrefresh(win);

        fclose(bible);
    }
}

void display_bible_error(const char *error)
{
    wclear(win), wmove(win, 0, 0);

    wattron(win, COLOR_PAIR(RED_COLOUR));
    wprintw(win, error);
    wattroff(win, COLOR_PAIR(RED_COLOUR));

    wrefresh(win);
}

void close_bible(void)
{
    delwin(win);
}