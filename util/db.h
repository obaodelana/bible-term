#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#define SQLITE_OMIT_DEPRECATED
#include "../lib/sqlite/sqlite3.h"

extern const char bibleStorePath[];

bool open_bible_db(size_t index);
void close_db(void);
int get_max_chapter(const char *book);
int get_no_of_verses(const char *book, int chapter);
bool store_bible_text(const char *book, int chapter, int verse);
bool get_book(char *currBook, int option);
