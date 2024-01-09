#include <stdio.h>

#define MAX_HASH_VALUE 129

extern const char *all_fids_reverse_lookup[];

extern unsigned int hash(const char *str, size_t len);
extern const char *in_word_set(const char *str, size_t len);

