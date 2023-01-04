UNAME := $(shell uname -s)
CFLAGS = -lncursesw -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 
ifeq ($(UNAME),Darwin)
	CFLAGS += -L/opt/homebrew/opt/ncurses/lib -I/opt/homebrew/opt/ncurses/include
endif

TARGET = bible
FILES = main.c lib/sqlite/sqlite3.o components/*.c ui/*.c util/*.c

default:
	@cd lib/sqlite; $(CC) -c sqlite3.c
	$(CC) $(FILES) -o $(TARGET) $(CFLAGS)

clean:
	$(RM) lib/sqlite/sqlite3.o
	$(RM) $(TARGET)
