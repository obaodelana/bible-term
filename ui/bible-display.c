#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "bible-display.h"

extern const char bibleStorePath[];

static WINDOW *win = NULL;
static int w = 80, h = 24, startTermLine = 1;

// Setup bible window
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

void reset_bible_start_pos(void)
{
    startTermLine = 1;
}

static int get_word(const char *str, char *word)
{
    if (str == NULL || word == NULL)
        return 0;

	// Move to next character, if at space
    if (*str == ' ') str++;
    
    // Clear current [word] content
    memset(word, 0, strlen(word));

	// Determines the end of a word
	// Words are separated by spaces, so that is the [endChar]
    char endChar = ' ';
	// If [str] starts at '<', it's a tag
	// So end at tag end
    if (*str == '<')
        endChar = '>';
    // If there's no space after word
	// That is, we're at the end of the line
    else if (strchr(str, ' ') == NULL)
    {
		// Lines end with the new line character
        endChar = '\n';
        // If there's no new line after word
		// Files end with the null character
        if (strchr(str, '\n') == NULL)
            endChar = '\0';
    }

	// Length of word is the distance from the first character to a white space or tag end
    int distToChar = strchr(str, endChar) - str;
	// Since a tag also includes its end character, increment its length
    if (endChar == '>') distToChar++;

    // Get word
    strncpy(word, str, distToChar);
    word[distToChar] = '\0';

    // If tag is in word e.g. Adam<e>
    if (word[0] != '<' && strchr(word, '<') != NULL)
    {
		// Get the distance of the first character in word to tag start
		// This is the actual length of the word (without the tag)
        int indexOfChar = strchr(word, '<') - word;
        int lenOfTag = strlen(word) - indexOfChar;

        // Clear tag part starting from the beginning of the tag
        memset(&word[indexOfChar], 0, lenOfTag);
    }

    return strlen(word);
}

static inline bool str_equal(const char *s1, const char *s2)
{
    return (strcmp(s1, s2) == 0);
}

// Enable attributes depending on the tag
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
    //     waddch(win, '\t');
    // else if (str_equal(tag, "<br/>"))
    //     waddch(win, '\n');
}

// Display bible text in window from text file
void display_bible(int verse)
{
    FILE *bible = fopen(bibleStorePath, "r");
    if (bible != NULL)
    {
        wclear(win), wmove(win, 0, 0);
        wattrset(win, A_NORMAL);
        
        char *line = malloc(1);
        size_t lineSize = 1;

        // Get first line (and throw it away, kinda)
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
			// Not all lines are verses e.g., titles
			bool isAVerse = false;
            while (*str != '\0' && *str != '\n')
            {	
                cursorY = getcury(win);

                int wordLen = get_word(str, word); 
                str += wordLen;

				// Verses starting with a "<v>"
				if (str_equal(word, "<v>"))
					isAVerse = true;

				// Print if at correct cursor location
				// [startTermLine] is the line the user is currently reading
                if ((verse <= 1 && currTermLine >= startTermLine)
				// OR verse
                    || verse == currVerse)
                {
                    canPrint = true;
                    if (verse > 1)
						// Save the line where the verse occurs
                        startTermLine = currTermLine;
                }
                
                // Tag
                if (word[0] == '<' && canPrint)
                {
					// If the tag is "<f>" or "<e>", skip it
					// i.e., dismiss text in between tag and it's closing tag
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
					// Implement word wrap
					// If the number of characters on the screen + number of characters about to be printed
					// 	is greater than the width of the terminal, go to a new line
                    if (charCount >= w)
                    {
						// Number of characters on new line is the length of the word to be printed
                        charCount = wordLen + (*str == ' ');
                        currTermLine++;

                        if (canPrint)
                        {
                            waddch(win, '\n');
							// If we're at the max height (terminal screen height)
                            if (getcury(win) == cursorY)
                                goto break2;
                            else
								// Save new cursor position
                                cursorY = getcury(win);
                        }
                    }

                    if (canPrint)
                    {
                        wprintw(win, "%s", word);
                        
						// If after printing word, cursor moves to next line
                        if (getcury(win) != cursorY)
                        {
							// Reset [charCount] to number of characters on new line (should be 0)
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
                
				// If we exceed the terminal's height
                if (getcury(win) < cursorY + 2)
                    break;
            }

			// If we're at the end of the text file, don't allow futher movement
            if (feof(bible))
            {
                startTermLine--;
            }

            else
            {
                currTermLine += 2;
                charCount = 0;
				if (isAVerse) currVerse++;
            }
        }

		// For breaking out of double while loops (see line ~209)
        break2:

        free(line);

        wrefresh(win);

        fclose(bible);
    }
}

// Display error (in a red colour) in bible window
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