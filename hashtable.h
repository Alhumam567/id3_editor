#ifndef DA_HT
#define DA_HT

typedef struct entry {
    char key[4];
    char *val;
} HT_ENTRY;

typedef struct direct_ht {
    HT_ENTRY **entries;

    int size;
} DIRECT_HT;

extern DIRECT_HT *direct_address_create(const int sz);

extern int direct_address_destroy(DIRECT_HT *ht);

extern int direct_address_insert(DIRECT_HT *ht, const char key[4], const char *val);

extern int direct_address_delete(DIRECT_HT *ht, const HT_ENTRY *entry);

extern HT_ENTRY *direct_address_search(const DIRECT_HT *ht, const char key[4]);

#endif
