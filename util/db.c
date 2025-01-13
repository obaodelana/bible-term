#include "db.h"
#include "store.h"
#include <ctype.h>
#include <ncurses.h>

const char bibleStorePath[] = ".bibleStore";

// SQL queries

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
    "AND chapter = ? "
    "ORDER BY verse ASC";
static const char storyTableExists[] = 
	"SELECT COUNT(*) FROM sqlite_master "
	"WHERE type='table' "
	"AND name='stories'";
static const char getTitle[] =
	"SELECT title FROM stories "
	"WHERE book_number = "
		"(SELECT book_number FROM books "
        "WHERE long_name LIKE ? LIMIT 1) "
	"AND chapter = ? "
	"AND verse = ? LIMIT 1";
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
	// Get number of translations
    int maxIndex = get_translations();

	// If translation index is valid
    if (index >= 0 && index < maxIndex)
    {
		// If db hasn't been opened (or just closed)
        if (db == NULL)
        {
			// Path to db
            char path[20];
            snprintf(path, 19, "db/%s.SQLite3", get_translation(index));

            if (sqlite3_open(path, &db) == SQLITE_OK)
            {
                initialized = true;
                return true;
            }

			// If couldn't open db, still close it
            else
			{
                sqlite3_close(db);
			}
        }

		// If db is already opened, close it and open a new one
		// This is run when a new translation is needed
        else
        {
			close_db();

			return open_bible_db(index);
        }
    }

    return false;
}

void close_db(void)
{
    if (db != NULL)
	{
		sqlite3_close(db);

		db = NULL;
		initialized = false;
	}
}

static bool check_init(void)
{
    if (!initialized)
        fprintf(stderr, "db.c: error: Database not initialized\n");

    return initialized;
}

// Add a percent sign to [str] and save it to [newStr]
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
	// Run [maxChapter] sql code on [db] and save it into [sql]
    int rc = sqlite3_prepare_v2(db, maxChapter, -1, &sql, NULL);

    if (rc == SQLITE_OK)
    {
        char text[strlen(book) + 2];
        appendPercent(text, book);
		// Change question mark in sql statement to [text]
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);

        if (rc == SQLITE_OK)
        {
			// If a row is returned
            if (sqlite3_step(sql) == SQLITE_ROW)
				// Save first column of first row to variable
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
	// Run [maxVerse] sql code on [db] and save it into [sql]
    int rc = sqlite3_prepare_v2(db, maxVerse, -1, &sql, NULL);
    if (rc == SQLITE_OK)
    {
        char text[strlen(book) + 2];
        appendPercent(text, book);
		// Change question mark in sql statement to [text]
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);

        if (rc == SQLITE_OK)
        {
			// Change second question mark in sql statement to [chapter]
            rc = sqlite3_bind_int(sql, 2, chapter);
            if (rc == SQLITE_OK)
            {
				// If a row is returned
                if (sqlite3_step(sql) == SQLITE_ROW)
					// Save first column of first row to variable
                    verses = sqlite3_column_int(sql, 0);
            }
        }
    }

    sqlite3_finalize(sql);

    return verses;
}

static bool get_title(const char *book, int chapter, int verse, char *title)
{
	if (!check_init())
		return false;

	bool hasTitle = false;

	sqlite3_stmt *sql;
	// Run [storyTableExists] sql code on [db] and save it into [sql]
	int rc = sqlite3_prepare_v2(db, storyTableExists, -1, &sql, NULL);
	if (rc == SQLITE_OK)
	{
		if (sqlite3_step(sql) == SQLITE_ROW)
		{
			// If the first column of the first row returns a value greater than zero,
			// Then the table "stories" exists
			if (sqlite3_column_int(sql, 0) > 0)
			{
				// Reset statement, so it can be reused
				sqlite3_reset(sql);
				rc = sqlite3_prepare_v2(db, getTitle, -1, &sql, NULL);
				if (rc == SQLITE_OK)
				{
					// Change question mark in sql statement to [book]
					rc = sqlite3_bind_text(sql, 1, book, -1, SQLITE_STATIC);
					if (rc == SQLITE_OK)
					{
						// Change second question mark in sql statement to [chapter]
						rc = sqlite3_bind_int(sql, 2, chapter);
						if (rc == SQLITE_OK)
						{
							// Change third question mark in sql statement to [verse]
							rc = sqlite3_bind_int(sql, 3, verse);
							if (rc == SQLITE_OK && sqlite3_step(sql) == SQLITE_ROW)
							{
								// Clear [title]
								memset(title, 0, strlen(title));
								// Copy text returned to [title]
								strncpy(title, (const char*) sqlite3_column_text(sql, 0), 100);

								// If title is not an empty string, it has a title
								hasTitle = strlen(title) > 0;
							}
						}
					}
				}
			}
		}
		
	}

	sqlite3_finalize(sql);

	return hasTitle;
}

bool store_bible_text(const char *book, int chapter, int verse)
{
    if (!check_init())
        return false;

	bool stored = false;

    sqlite3_stmt *sql;
	// Run [getBible] sql code on [db] and save it into [sql]
    int rc = sqlite3_prepare_v2(db, getBible, -1, &sql, NULL);
    if (rc == SQLITE_OK)
    {
		// Change question mark in sql statement to [text]
		char text[strlen(book) + 2];
        appendPercent(text, book);
        rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);

        if (rc == SQLITE_OK)
        {
			// Change second question mark in sql statement to [chapter]
            rc = sqlite3_bind_int(sql, 2, chapter);
            if (rc == SQLITE_OK)
            {
                FILE *bibleStore = fopen(bibleStorePath, "w");
                if (bibleStore != NULL)
                {
                    int verse = 1;
					// If a row is returned
                    if (sqlite3_step(sql) == SQLITE_ROW)
                    {
						// Save bible path to first line
                        fprintf(bibleStore, "%s %i:%03i\n", book, chapter, verse);
                        do
                        {
                            if (verse > 1)
                            	fputc('\n', bibleStore);
							
							char title[100 + 1];
							// Add title of current verse (if it has)
							if (get_title(book, chapter, verse, title))
								fprintf(bibleStore, "<b>%s</b>\n", title);

							// Verse number
							fprintf(bibleStore, "<v>[%i] </v>", verse++);
							// Verse text
							fprintf(bibleStore, "%s", (const char*) sqlite3_column_text(sql, 0));
                        } while (sqlite3_step(sql) == SQLITE_ROW);
                    }

                    fclose(bibleStore);
                    
                    if (verse > 1)
                        stored = true;
                }
            }
        }        
    }

    sqlite3_finalize(sql);

    return stored;
}

bool get_book(char *currBook, int option)
{
	bool gotten = false;

    if (check_init() && currBook != NULL)
    {
        sqlite3_stmt *sql;
		// Select sql query depending on option
		// [option] = 0 -> [getBook]
		// [option] < 0 -> [prevBook]
		// [option] > 1 -> [nextBook]
        const char *query = (option < 0)
                            ? prevBook
                            : (option > 0)
                                ? nextBook
                                : getBook;

		// Run [query] sql code on [db] and save it into [sql]
        int rc = sqlite3_prepare_v2(db, query, -1, &sql, NULL);
        if (rc == SQLITE_OK)
        {
            char text[strlen(currBook) + 2];
            appendPercent(text, currBook);

			// Change question mark in sql statement to [text]
            rc = sqlite3_bind_text(sql, 1, text, -1, SQLITE_STATIC);
            if (rc == SQLITE_OK)
            {
				// If a row is returned
                if (sqlite3_step(sql) == SQLITE_ROW)
                {
					// Save returned string to [currBook]
                    strcpy(currBook, (const char*) sqlite3_column_text(sql, 0));
                    gotten = true;

	 				int n = strlen(currBook);
					// If books ends with whitespace
					if (isspace(currBook[n - 2]))
					{
						// Remove it
						currBook[n - 2] = '\0';
					}
                }
            }
        }
        
        sqlite3_finalize(sql);
    }

    return gotten;
}
