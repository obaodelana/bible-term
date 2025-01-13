#include <stdio.h>
#include <time.h>

const char LOG_FILE[] = "../.log";
FILE *LOGGER = NULL;

void enable_logging()
{
	if (LOGGER != NULL)
	{
		return;
	}
	LOGGER = fopen(LOG_FILE, "a");
}

static void log(const char *tag)
{
	time_t now = time(0);
	fprintf(LOGGER, "%s: (%s) ", ctime(&now), tag);
}

void log_string(const char* str, const char* tag)
{
	log(tag);
	fprintf(LOGGER, "%s\n", str);
}

void log_int(int i, const char* tag)
{
	log(tag);
	fprintf(LOGGER, "%i\n", i);
}