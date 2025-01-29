#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

const char LOG_FILE[] = ".log";
FILE *LOGGER = NULL;

void enable_logging()
{
	if (LOGGER != NULL)
	{
		return;
	}
	LOGGER = fopen(LOG_FILE, "a");
}

static void log_with_tag(const char *tag)
{
	time_t now;
	time(&now);

	char *time = ctime(&now);
	// Turn new line to end of character
	time[strlen(time)-1] = '\0'; 

	fprintf(LOGGER, "%s: (%s) ", time, tag);
}

void log_string(const char* str, const char* tag)
{
	log_with_tag(tag);
	fprintf(LOGGER, "%s\n", str);
	fflush(LOGGER);
}

void log_int(int i, const char* tag)
{
	log_with_tag(tag);
	fprintf(LOGGER, "%i\n", i);
	fflush(LOGGER);
}

void log_bool(bool b, const char* tag)
{
	log_with_tag(tag);
	fprintf(LOGGER, "%s\n", (b ? "TRUE" : "FALSE"));
	fflush(LOGGER);
}

void close_logging()
{
	if (LOGGER != NULL)
	{
		fclose(LOGGER);
	}
}