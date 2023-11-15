#ifndef UTIL
#define UTIL

extern int synchsafeint32ToInt(char c[4]);

extern void intToSynchsafeint32(int x, char ssint[4]);

extern char *concatenate(char *s1, char *s2);

extern int get_trck(char *filepath, int prefix_len);

extern int get_index(char (*str)[5], int arr_len, char fid[4]);

#endif