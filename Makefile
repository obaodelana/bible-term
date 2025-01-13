UNAME := $(shell uname -s)
CFLAGS = -lncursesw -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 
ifneq ($(OS), Windows_NT)
	ifeq ($(UNAME),Darwin)
		CFLAGS += -L/opt/homebrew/opt/ncurses/lib -I/opt/homebrew/opt/ncurses/include
	endif
endif

TARGET = bible

COMPONENTS := $(wildcard components/*.c)
UI := $(wildcard ui/*.c)
UTIL := $(wildcard util/*.c)

FILES = main.c lib/sqlite/sqlite3.o $(COMPONENTS) $(UI) $(UTIL)

default: $(FILES)
	$(CC) $(FILES) -o $(TARGET) $(CFLAGS)

lib/sqlite/sqlite3.o: lib/sqlite/sqlite3.c
	@cd lib/sqlite; $(CC) -c sqlite3.c

reset:
	@$(RM) .log
	@$(RM) .bibleStore

clean:
	$(RM) lib/sqlite/sqlite3.o
	$(RM) $(TARGET)
