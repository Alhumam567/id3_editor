#ifndef DA_HT
#define DA_HT

#include "id3_hash.h"

typedef struct entry {
    char key[4];
    void *val;
} HT_ENTRY;

typedef struct direct_ht {
    HT_ENTRY **entries;

    int buckets;
    int sz;

    unsigned int (*hash_func)(const char key[4]);
} DIRECT_HT;

#define MAX_HASH_VALUE 129

extern const char e_fids_reverse_lookup[][5];

extern unsigned int dt_hash(const DIRECT_HT *ht, const char k[4]); 

extern DIRECT_HT *direct_address_create(const int buckets, unsigned int (*hash_func)(const char key[4]));

extern int direct_address_destroy(DIRECT_HT *ht);

extern int direct_address_insert(DIRECT_HT *ht, const char key[4], void *val);

extern int direct_address_delete(DIRECT_HT *ht, HT_ENTRY *entry);

extern HT_ENTRY *direct_address_search(const DIRECT_HT *ht, const char key[4]);

extern int in_key_set(const DIRECT_HT *ht, const char str[4]);

extern unsigned int all_fids_hash(const char k[4]);
extern unsigned int e_fids_hash(const char k[4]);

#endif
