#include "db.h"
#include "store.h"

const char bibleStorePath[] = ".bibleStore";

static const char maxChapter[] =
    "SELECT MAX(chapter) FROM verses "
    "WHERE book_number = "
        "(SELECT book_number FROM books "
        "WHERE long_name LIKE ? LIMIT 1)";
static const char maxVerse[] =
    "SELECT MAX(verse) FROM verses "
    "WHERE book_number = "
        "(SELECT book_number FROM books "
        "WHERE long_name LIKE ? LIMIT 1) "
    "AND chapter = ?";
static const char getBible[] =
    "SELECT verses.text, books.long_name FROM verses "
    "JOIN books ON verses.book_number = books.book_number "
    "WHERE verses.book_number = "
        "(SELECT book_number FROM books "
        "WHERE long_name LIKE ? LIMIT 1) "
    "AND chapter = ?"
    "ORDER BY verse ASC";
static const char getBook[] =
    "SELECT long_name FROM books "
    "WHERE book_number = "
        "(SELECT book_number FROM books "
        "WHERE long_name LIKE ? LIMIT 1)";
static const char nextBook[] =
    "SELECT long_name FROM books "
    "WHERE book_number > "
        "(SELECT book_number from books "
        "WHERE long_name LIKE ? LIMIT 1) "
    "ORDER BY book_number ASC LIMIT 1";
static const char prevBook[] =
    "SELECT long_name FROM books "
    "WHERE book_number < "
        "(SELECT book_number from books "
        "WHERE long_name LIKE ? LIMIT 1) "
    "ORDER BY book_number DESC LIMIT 1";

static bool initialized = false;

static sqlite3 *db = NULL;

bool open_bible_db(size_t index)
{
    int maxIndex = get_translations();

    if (index >= 0 && index <= maxIndex)
    {
        if (db == NULL)
        {
            char path[20];
            snprintf(path, 19, "db/%s.SQLite3", get_translation(index));

            if (sqlite3_open(path, &db) == SQLITE_OK)
            {
                initialized = true;
                return true;
            }

            else
                sqlite3_close(db);
        }

        else
        {
            sqlite3_close(db);

            db = NULL;
            initialized = false;

            return open_bible_db(index);
        }
    }

    return false;
}

void close_db(void)
{
    if (db != NULL)
        sqlite3_close(db);
}

static bool check_init(void)
{
    if (!initialized)
        fprintf(stderr, "db.c: error: Database not initialized\n");

    return initialized;
}

static inline void appendPercent(char *newStr, const char *str)
{
    sprintf(newStr, "%s%%", str);
}

int get_max_chapter(const char *book)
{
    int noOfChapters = 0;
    if (!check_init())
        return 0;
    
    sqlite3_stmt *sql;
    int rc = sqlite3_prepare_v2(db, maxChapter, -1, &sql, NULL);

    if (rc == SQLITE_OK)
    {
        char text[strlen(book) + 2];
        appendPercent(text, book);
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);
        if (rc == SQLITE_OK)
        {
            if (sqlite3_step(sql) == SQLITE_ROW)
                noOfChapters = sqlite3_column_int(sql, 0);
        }
    }

    sqlite3_finalize(sql);

    return noOfChapters;
}

int get_no_of_verses(const char *book, int chapter)
{
    int verses = 0;
    if (!check_init())
        return 0;    

    sqlite3_stmt *sql;
    int rc = sqlite3_prepare_v2(db, maxVerse, -1, &sql, NULL);
    if (rc == SQLITE_OK)
    {
        char text[strlen(book) + 2];
        appendPercent(text, book);
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);

        if (rc == SQLITE_OK)
        {
            rc = sqlite3_bind_int(sql, 2, chapter);
            if (rc == SQLITE_OK)
            {
                if (sqlite3_step(sql) == SQLITE_ROW)
                    verses = sqlite3_column_int(sql, 0);
            }
        }
    }

    sqlite3_finalize(sql);

    return verses;
}

bool store_bible_text(const char *book, int chapter, int verse)
{
    if (!check_init())
        return false;

    sqlite3_stmt *sql;
    int rc = sqlite3_prepare_v2(db, getBible, -1, &sql, NULL);
    if (rc == SQLITE_OK)
    {
        char text[strlen(book) + 2];
        appendPercent(text, book);
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);
        if (rc == SQLITE_OK)
        {
            rc = sqlite3_bind_int(sql, 2, chapter);
            if (rc == SQLITE_OK)
            {
                FILE *bibleStore = fopen(bibleStorePath, "w");
                if (bibleStore != NULL)
                {
                    int verse = 1;
                    if (sqlite3_step(sql) == SQLITE_ROW)
                    {
                        fprintf(bibleStore, "%s %i:%03i\n", book, chapter, verse);
                        do
                        {
                            if (verse > 1)
                            fputc('\n', bibleStore);
                        fprintf(bibleStore, "<v>[%i] </v>", verse++);
                        fprintf(bibleStore, "%s", sqlite3_column_text(sql, 0));
                        } while (sqlite3_step(sql) == SQLITE_ROW);
                    }


                    fclose(bibleStore);
                    
                    if (verse > 1)
                        return true;
                }
            }
        }        
    }

    sqlite3_finalize(sql);

    return false;
}

bool get_book(char *currBook, int option)
{
    if (check_init() && currBook != NULL)
    {
        sqlite3_stmt *sql;
        const char *query = (option < 0)
                            ? prevBook
                            : (option > 0)
                                ? nextBook
                                : getBook;
        int rc = sqlite3_prepare_v2(db, query, -1, &sql, NULL);
        if (rc == SQLITE_OK)
        {
            char text[strlen(currBook) + 2];
            appendPercent(text, currBook);
            rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);
            if (rc == SQLITE_OK)
            {
                if (sqlite3_step(sql) == SQLITE_ROW)
                {
                    strcpy(currBook, sqlite3_column_text(sql, 0));
                    return true;
                }
            }
        }
        
        sqlite3_finalize(sql);
    }

    return false;
}