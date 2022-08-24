#include <ncurses.h>

#ifndef INF_RECT
#define INF_RECT

typedef struct
{
    int w, h, x, y;
} Rect;
#endif

typedef enum
{
    INF_LETTERS_LC = 1, // Lower case letters only
    INF_LETTERS_UC = 2, // Upper case letters only
    INF_LETTERS = INF_LETTERS_LC | INF_LETTERS_UC, // Upper and lower case letters
    INF_NUMBERS = 4, // 0 - 9
    INF_MATH_SIGNS = 8, // %, ^, *, (), -, +, =, /, <, >
    INF_OTHER_SIGNS = 16, // @, #, $, &, _, {, }, \, |
    INF_PUNCTUATIONS = 32, // !, `, ~, (, ), [, ], ;, :, ', ", , ., ?
    INF_SYMBOLS = INF_MATH_SIGNS | INF_OTHER_SIGNS | INF_PUNCTUATIONS, // !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
    INF_ALL_CHARS = INF_LETTERS | INF_NUMBERS | INF_SYMBOLS, // All characters
} INF_AllowedChars;

// TODO: Add required boolean
// Structure for input fields
typedef struct
{
    WINDOW *win;
    Rect winDim;

    size_t strLen;
    union
    {
        // Text fields
        char *allowedChars;
        // Number fields
        struct
        {
            float min, max;
        } numberRange;
    };
    
    // Callback function when Enter key is pressed
    union
    {
        // For text fields
        bool (*onEnterPressedText)(const char*);
        // For number fields
        bool (*onEnterPressedNum)(float);
    };

    bool textField, wasTyping;
    
    char str[];
} InputField;

// Create text input field
int inf_new_text
(
    // Position and size
    Rect rect,
    // Text that appears when the user hasn't typed anything (can be empty)
    const char *placeholder,
    // Characters allowed to be typed (use [INF_AllowedChars] enum)
    INF_AllowedChars allowedChars,
    // Characters that are not allowed to be typed (can be empty)
    const char *forbiddenCharacters,
    // Function callback when user presses enter / return
    bool (*onEnterPressed)(const char*)
);
// Create number-only input field
int inf_new_number
(
    // Position and size
    Rect rect,
    // Text that appears when the user hasn't typed anything (can be empty)
    const char *placeholder,
    // Minimum number value (inclusive)
    float minNumber,
    // Maximum number value (inclusive)
    float maxNumber,
    // Function callback when user presses enter / return
    bool (*onEnterPressed)(float)
);
// Update input fields (must be placed in main loop)
void inf_update(int character);
// Change value of [index]th text field to [newText]
bool inf_set_text_value(size_t index, const char *newText);
// Change value of [index]th number field to [newNumber]
bool inf_set_number_value(size_t index, float newNumber);
// Switch focus to a [index]th input field, so user can type
void inf_switch_focus(size_t index);
// Get value from [index]th text field
const char* inf_get_text_value(size_t index);
// Get value from [index]th number field
float inf_get_number_value(size_t index);
// Free up used memory
void inf_cleanup(void);
