#define RED_COLOUR 1

void init_bible(void);
void scroll_bible(bool up);
void reset_bible_start_pos(void);
void display_bible(int verse);
void display_bible_error(const char *error);
void close_bible(void);