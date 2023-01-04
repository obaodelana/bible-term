// Allows getline to work on MacOS
#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include "store.h"

extern const char bibleStorePath[];

char** translations = NULL;
size_t noOfTranslations = 0;

// Return line containing the bible path (first line)
static char *get_bible_path()
{
    FILE *store = fopen(bibleStorePath, "r");
    if (store != NULL)
    {
        char *line = malloc(1);
        size_t lineSize = 1;

        if (getline(&line, &lineSize, store) != EOF)
            return line;
    }

    return NULL;
}

void get_stored_path(char *book, int *chapter, int *verse)
{
    char *biblePath = get_bible_path();
    if (biblePath != NULL)
    {
		// If book contains number e.g., 1 Corinthians
        if (isdigit(biblePath[0]))
        {
            int bookNo = 1;
            char bookName[18] = "";

			// (book number e.g. 1) (book name e.g. Corinthians) (chapter):(verse)
            sscanf(biblePath, "%1d %17s %d%*c%d", &bookNo, bookName, chapter, verse);

			// Combine [bookNo] and [bookName] to give the book
            sprintf(book, "%d %s", bookNo, bookName);
        }

        else
			// (book e.g. John) (chapter):(verse)
            sscanf(biblePath, "%19s %d%*c%d", book, chapter, verse);

        free(biblePath);
    }

	// If there's no previously stored path, use default
    else
    {
        strcpy(book, "Genesis");
        *chapter = 1, *verse = 1;
    }
}

int get_translations(void)
{
	// If function was already called
    if (noOfTranslations > 0)
        return noOfTranslations;

    struct dirent *file;
    DIR *dir = opendir("db");

    if (dir != NULL)
    {
		size_t translationCount = 0;

		// While there are files in db folder
        while ((file = readdir(dir)))
        {
            // If file name has the extension ".SQLITE3"
            if (strstr(file->d_name, ".SQLite3"))
            {
				// Memory allocation for an expanding list
                if (translationCount++ == 0)
                    translations = (char**) calloc(1, sizeof(char*));
                else
                    translations = (char**) realloc(translations, sizeof(char*) * translationCount);
                
				// Length of bible translation
				// Find the distance from the period to the first letter
				// e.g. KJV.SQLITE3: the first letter is 3 units away from the period
                int strLength = strchr(file->d_name, '.') - file->d_name;

                translations[translationCount - 1] = calloc(strLength + 1, sizeof(char));
				// Store the name of current translation
                strncpy(translations[translationCount - 1], file->d_name, strLength);
            }
        }

        closedir(dir);

        return (noOfTranslations = translationCount);
    }

    return 0;
}

const char *get_translation(size_t index)
{
    if (translations != NULL
        && index >= 0 && index < noOfTranslations)
        return translations[index];
    
    return "";
}
