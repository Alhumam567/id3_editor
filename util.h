#ifndef UTIL
#define UTIL

// ANSI Color/Style
#define FAIL "\033[1;31m"
#define PASS "\033[1;32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURP "\033[1;35m"
#define WHITE_BOLD "\033[1;4;37m"

extern int synchsafeint32ToInt(const char c[4]);

extern void intToSynchsafeint32(int x, char ssint[4]);

extern char *concatenate(char *s1, char *s2);

extern int get_trck(char *filepath, int prefix_len);

extern int get_index(char (*str)[5], int arr_len, char fid[4]);

extern char *get_fid(int i);

extern void cprintf(const char *color, const char *fmt, ...);

extern int file_copy(const char *src, const char *dst);


#endif
