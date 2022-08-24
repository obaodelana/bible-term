CFLAGS = -lncursesw -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600
TARGET = bible
FILES = main.c components/*.c ui/*.c util/*.c

$(TARGET): $(FILES)
	$(CC) $(FILES) lib/sqlite/sqlite3.o -o $(TARGET) $(CFLAGS)

clean:
	$(RM) $(TARGET)