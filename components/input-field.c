#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>
#include "input-field.h"

static InputField **infs = NULL;
static size_t infsLen = 0, infsIndex = 0, currentFocusIndex = 0;
static MEVENT mouseEvent;

// Set of allowed characters
static char letters_lc[] = "abcdefghijklmnopqrstuvwxyz", letters_uc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char numbers[] = "0123456789";
static char mathSigns[] = "%^*()-+=/<>", otherSigns[] = "@#$&_{}[]\\|", punctuations[] = "!`~()[];:'\",.?";

// Create new input field
static InputField* inf_new(Rect rect, const char *placeholder)
{
    assert(rect.w > 0 && rect.h > 0);

    // Make sure array has enough space
    if (infsIndex >= infsLen)
    {
        // Setup array for the first time
        if (infs == NULL)
            infs = (InputField**) malloc((infsLen = 2) * sizeof(InputField*));
        // Resize array
        else
            infs = (InputField**) realloc(infs, (infsLen *= 2) * sizeof(InputField*));
    }

    // Allocate enough memory for the object + space for string
    // Space of string is equal to the width * line height of the input field
    InputField *inf = (InputField*) malloc(sizeof(InputField) + (sizeof(char) * ((rect.w * rect.h) + 1)));
    
    // Create new window for input string, so it can be easier to manage and draw text and stuff
    inf->win = newwin(rect.h + 2, rect.w, rect.y, rect.x);
    // Save window position and dimension for future purposes
    inf->winDim = rect;
    // Border requires + 2 space
    inf->winDim.h += 2;

    nodelay(inf->win, true); // Don't stop when asked for input
    keypad(inf->win, true); // Allow to read arrow and functions keys, and allows mouse event to work properly
    if (infsIndex == 0) box(inf->win, 0, 0); // Place border around first input field
    mvwprintw(inf->win, 1, 1, placeholder); // Print placeholder text inside window
    wrefresh(inf->win); // Refresh window to show text printed above
    
    // Save placeholder text in [str]
    strncpy(inf->str, placeholder, rect.w * rect.h);
    // Store length of [str]
    inf->strLen = strlen(placeholder);
    
    // Used to check if user typed before
    /*
        In inf_update(), if the user types for the first time
        or the input field is refocused on after the focus was on another input field,
        the previous text value is cleared and new text overwrites the old.
        So this boolean helps to keep track of if the user just started typing or has been typing before 
    */
    inf->wasTyping = false;

    // Add input field to [infs] array
    infs[infsIndex] = inf;

    return inf;
}

// Add to [str] ([add] ∩ [dontAdd])`
static void add_to_str(char *str, const char *add, const char *dontAdd)
{
    if (str != NULL)
    {
        for (const char *c = add; *c != '\0'; c++)
        {
            if (strchr(dontAdd, *c) == NULL)
                strncat(str, c, 1);
        }
    }
}

// Special setup for text input fields
int inf_new_text(Rect rect, const char *placeholder, INF_AllowedChars allowedChars, const char *forbiddenChars, bool (*onEnterPressed)(const char*))
{
    // Make sure [allowedChars] is not out of bounds
    assert(allowedChars > INF_LETTERS_LC && allowedChars < INF_ALL_CHARS);

    // Create new input field with basic setup
    InputField *inf = inf_new(rect, placeholder);
    // Pointer to callback function
    inf->onEnterPressedText = onEnterPressed;
    // Enable special text field only features
    inf->textField = true;

    // Set up allowed characters
    inf->allowedChars = calloc(100, sizeof(char));
    
    // Add to [inf->allowedChars] only allowed characters

    // If a space is not in [forbiddenChars], add to [inf->allowedChars]
    if (strchr(forbiddenChars, ' ') == NULL)
        strcat(inf->allowedChars, " ");
    if (allowedChars & INF_LETTERS_LC)
        add_to_str(inf->allowedChars, letters_lc, forbiddenChars);
    if (allowedChars & INF_LETTERS_UC)
        add_to_str(inf->allowedChars, letters_uc, forbiddenChars);
    if (allowedChars & INF_NUMBERS)
        add_to_str(inf->allowedChars, numbers, forbiddenChars);
    if (allowedChars & INF_MATH_SIGNS)
        add_to_str(inf->allowedChars, mathSigns, forbiddenChars);
    if (allowedChars & INF_OTHER_SIGNS)
        add_to_str(inf->allowedChars, otherSigns, forbiddenChars);
    if (allowedChars & INF_PUNCTUATIONS)
        add_to_str(inf->allowedChars, punctuations, forbiddenChars);

    // Return the index of the current input field
    return infsIndex++;
}

// Max string length is given as the (width of input field) * (line height of input field)
// Line height is given as (height of input field) - 2. -2 is there because it was added in [inf_new()]
static inline int get_max_str_len(InputField *inf)
{
    return (inf->winDim.w * (inf->winDim.h - 2));
}

// Special setup for number input fields
int inf_new_number(Rect rect, const char *placeholder, float min, float max, bool (*onEnterPressed)(float))
{
    // Create new input field with basic setup
    InputField *inf = inf_new(rect, placeholder);
    // Pointer to callback function
    inf->onEnterPressedNum = onEnterPressed;
    // Enable special features for non-text fields or number fields
    inf->textField = false;

    // If min and max is not set, set it to the minimum and maximum possible values
    if (min == 0.0f && max == 0.0f)
    {
        min = -FLT_MAX;
        max = FLT_MAX;
    }

    // If the min and max are flipped, un-flip them
    else if (min > max)
    {
        int min1 = min;
        min = max, max = min1;
    }

    // Set the range for the number field
    inf->numberRange.min = min;
    inf->numberRange.max = max;

    // Return the index of the current input field
    return infsIndex++;
}

// Add or remove border
static void inf_border(WINDOW *win, bool add)
{
    // Add border
    if (add)
        box(win, 0, 0);
    // Remove border
    else
        wborder(win, ' ', ' ', ' ',' ',' ',' ',' ',' ');

    wrefresh(win);
}

// Clear input field text [str] and update it on the terminal
static void inf_clear(InputField *inf)
{
    if (inf != NULL)
    {
        // Remove border
        inf_border(infs[currentFocusIndex]->win, false);

        // Remove previous input field text displayed in the terminal
        for (int i = 0; i < inf->strLen; i++)
            mvwdelch(inf->win, getcury(inf->win), getcurx(inf->win) - 1);
        // Move cursor to start    
        wmove(inf->win, 1, 1);

        // Clear [str] by setting all its elements to zero
        memset(inf->str, 0, inf->strLen);
        // Reset string length
        inf->strLen = 0;
    }
}

// Get number of digits [no] has
static int inf_get_no_of_digits(float no)
{
    int noOfDigits = 0;
    
    // snprintf() returns the number of bytes that would be written -> no of digits

    // Check if float is an integer e.g. 3.0000
    if (no == (int) no)
        noOfDigits = snprintf(NULL, 0, "%i", (int) no);
    else
        noOfDigits = snprintf(NULL, 0, "%.4f", no);

    return noOfDigits;
}

// Convert [no] to string
static void inf_number_to_str(float no, char *str)
{
    if (str != NULL)
    {
        // Integers i.e. 3.0000 == 3 
        if (no == (int) no)
            sprintf(str, "%i", (int) no);
        // Floating numbers
        else
            sprintf(str, "%.4f", no);
    }
}

// Change value of [i]th text field to [newText]
bool inf_set_text_value(size_t i, const char *newText)
{
    // Make sure index [i] is valid &&
    // Make sure [i]th input field is a text field
    // Make sure new text is not too long
    if (i >= 0 && i < infsIndex
        && infs[i]->textField
        && strlen(newText) < get_max_str_len(infs[i]))
    {
        // Get specified input field
        InputField *inf = infs[i];
        if (inf != NULL)
        {
            // Clear its original contents
            inf_clear(inf);

            // Set [str] to [newText]
            strcpy(inf->str, newText);
            // Set the new string length
            inf->strLen = strlen(newText);

            // Show the new text
            mvwprintw(inf->win, 1, 1, newText);
            wrefresh(inf->win);

            return true;
        }
    }

    return false;
}

// Change value of [i]th number field to [newNumber]
bool inf_set_number_value(size_t i, float newNumber)
{
    // Make sure index [i] is valid &&
    // Make sure [i]th input field is a number field
    if ((i >= 0 && i < infsIndex) && !infs[i]->textField)
    {
        // Get specified input field
        InputField *inf = infs[i];
        if (inf != NULL)
        {
            // Clear its original contents
            inf_clear(inf);

            // Turn number to string
            char newText[inf_get_no_of_digits(newNumber) + 1];
            inf_number_to_str(newNumber, newText);

            // Make sure new text is not too long
            if (strlen(newText) < get_max_str_len(inf))
            {
                // Set [str] to [newText]
                strcpy(inf->str, newText);
                // Set the new string length
                inf->strLen = strlen(newText);

                // Show the new text
                mvwprintw(inf->win, 1, 1, newText);
                wrefresh(inf->win);

                return true;
            }
        }
    }

    return false;
}

// Switch focus to a [index]th input field, so user can type in it
void inf_switch_focus(size_t i)
{
    // Make sure [i] doesn't exceed max index
    if (i >= 0 && i < infsIndex)
    {
        // Remove border from current focused window
        inf_border(infs[currentFocusIndex]->win, false);
        // Reset [wasTyping]
        infs[currentFocusIndex]->wasTyping = false;
        
        // New focused index
        currentFocusIndex = i;
        // Add border to new focused window
        inf_border(infs[currentFocusIndex]->win, true);
    }
}

// Check if the mouse is on top of rectangle using AABB
// https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
static bool inf_is_mouse_touching_rect(MEVENT m, Rect rect)
{
    int mouseSize = 1;
    return (rect.x < m.x + mouseSize
            && rect.x + rect.w > m.x
            && rect.y < m.y + mouseSize
            && rect.y + rect.h > m.y);
}

// Check to see if character is allowed in text field
static bool inf_validate_text_field(InputField *inf, char ch)
{
    return (strchr(inf->allowedChars, ch) != NULL);
}

// Make sure value of number field is within range
static bool inf_validate_number_field(InputField *inf)
{
    // Turn string to number
    float no = strtof(inf->str, NULL);

    // Check if [no] is out of bounds
    if ((no < inf->numberRange.min) || (no > inf->numberRange.max))
    {
        // Set [no] to be in range of [inf->numberRange.min] < [no] < [inf->numberRange.max]
        if (no < inf->numberRange.min)
            no = inf->numberRange.min;
        else if (no > inf->numberRange.max)
            no = inf->numberRange.max;

        // Update value of number field
        inf_set_number_value(currentFocusIndex, no);
        // Add border around it
        inf_switch_focus(currentFocusIndex);

        return false; // Not within range
    }

    return true; // Within range
}

// Update input fields (must be placed in main loop)
void inf_update(int ch)
{
    InputField *inf = infs[currentFocusIndex];
    WINDOW *win = inf->win;

    // If mouse movement or click is detected
    if (ch == KEY_MOUSE)
    {
        // Capture mouse event
        if (getmouse(&mouseEvent) == OK)
        {
            // If click on the left-mouse button is captured
            if (mouseEvent.bstate & BUTTON1_CLICKED)
            {
                // Go through all input fields
                for (int i = 0; i < infsIndex; i++)
                {
                    // If the mouse is on an input field
                    if (inf_is_mouse_touching_rect(mouseEvent, infs[i]->winDim))
                    {
                        inf_switch_focus(i);
                        break;
                    }
                }
            }
        }
    }

    // Not mouse: key press
    else if (ch != ERR)
    {
        if (ch == KEY_ENTER || ch == '\n')
        {
            // If it has a callback function
            if ((inf->textField && inf->onEnterPressedText != NULL)
                || (!inf->textField && inf->onEnterPressedNum != NULL))
            {
                // If a number field and valid, allow enter to be pressed
                if ((!inf->textField && inf_validate_number_field(inf))
                    // If a text field allow enter all the time
                    || inf->textField)
                {
                    // Save index in case call to [onEnterPressed] switches focus to another input field 
                    int currentIndex = currentFocusIndex;
                    
                    // If enter registered by programmer i.e. true is returned
                    if((inf->textField && (*inf->onEnterPressedText)(inf->str))
                        || (!inf->textField && (*inf->onEnterPressedNum)(inf_get_number_value(currentIndex))))
                        // Stopped typing 
                        inf->wasTyping = false;
                    // If enter is not registered, prompt user to type again
                    else
                        inf_switch_focus(currentIndex);
                }
            }

            // No callback function
            else
            {
                // For number fields
                if (!inf->textField)
                    inf_validate_number_field(inf);
                inf->wasTyping = false;
            }

            return;
        }

        else if (ch == KEY_BACKSPACE || ch == '\b' || ch == 127)
        {
            int x = getcurx(win) - 1;
            // Make sure to not "over delete"
            if (x >= 1)
            {
                // If user just started typing
                if (!inf->wasTyping)
                {
                    inf->wasTyping = true;
                    // Remove border
                    inf_border(win, false);
                }

                // Delete last character
                mvwdelch(win, getcury(win), x);
                wrefresh(win);

                // Remove last character in [str]
                inf->str[inf->strLen - 1] = '\0';
                // Update length
                inf->strLen--;
            }

            return;
        }

        // If character is a printable character e.g. letter, number, space or symbol
        if (isprint(ch))
        {
            char c = ch;

            // For text fields
            if (inf->textField)
            {
                if (!inf_validate_text_field(inf, c))
                    return;
            }

            // For number fields
            else
            {
                // If not a digit and not a sign (minus sign for negative numbers, period for floats)
                if (!isdigit(c) && strchr("-.", c) == NULL)
                    return;
                // If it's a minus sign make sure it's at the beginning of the number e.g. -34, not 3-4
                else if (c == '-' && inf->strLen != 0 && inf->wasTyping)
                    return;
                // If it's a period, make sure it's the only period i.e. 3.4, not 3.4.5
                else if (c == '.' && strchr(inf->str, '.') != NULL)
                    return;
            }

            // If first time typing, remove placeholder or previous text
            if (!inf->wasTyping)
            {
                inf->wasTyping = true;
                inf_clear(inf);
            }
            
            // Make sure text doesn't exceed width
            if (inf->strLen < get_max_str_len(inf))
            {
                // Show new character
                waddch(win, c), wrefresh(win);

                // Add character to [str] to keep track of typed text
                strncat(inf->str, &c, 1);
                // Update string length
                inf->strLen++;
            }
        }
    }
}

// Get value from [i]th text field
const char* inf_get_text_value(size_t i)
{
    // Make sure [i] is valid and is a text field
    if ((i >= 0 && i < infsIndex) && infs[i]->textField)
        return infs[i]->str;
    // If [i] doesn't exist, return empty string
    return "";
}

// Get value from [i]th number field
float inf_get_number_value(size_t i)
{
    // Make sure [i] doesn't exceed max index and is a number field
    if (i < infsIndex && !infs[i]->textField)
        return strtof(infs[i]->str, NULL);
    // If [i] doesn't exist, return zero
    return 0;
}

// Free up used memory
void inf_cleanup(void)
{
    // Go through all used spots (that's why I used [infsIndex] and not [infsLen]) in [inf] array
    for (int i = 0; i < infsIndex; i++)
    {
        // Free up memory used by window
        delwin(infs[i]->win);
        // If it's a text field, free up memory used by [forbiddenCharacters]
        if (infs[i]->textField)
            free(infs[i]->allowedChars);
        free(infs[i]);
    }
    
    if (infs != NULL)
        free(infs);
}
