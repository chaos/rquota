char *size2str(unsigned long long size, char *str, int len);
char *xstrdup(char *str);
void *xmalloc(size_t size);
int match_path(char *dir, const char *mountpoint);
void test_match_path(void);
unsigned long parse_blocksize(char *s, unsigned long *b);

