// Get the previous book, chapter and verse stored in file
void get_stored_path(char *book, int *chapter, int *verse);
// Get all translations in db folder (and return the count)
int get_translations(void);
// Get name of [index]th translation
const char *get_translation(size_t index);