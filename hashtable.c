/* Direct addressing hashtable implementation
 *
 * universe of keys are editable frame IDs:
 * "TPE1"
 * "TALB"
 * "TIT2"
 * "TRCK"
 * "APIC"
 * 
 * hash function chosen ensures minimal and perfect hash table
 */
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"
#include "id3.h"

const int buckets = E_FIDS;
const int a = 46856658;
const int b = 669512;
const int p = 65805703;

unsigned int linear_hash(const unsigned int k) {
	return ((a*k + b) % p) % buckets;
}

unsigned int strtoint(const char k[4]) {
	return (k[3] * pow(36,3)) + (k[2] * pow(36,2)) + (k[1] * 36) + k[0];
}

unsigned int hash_key(const char k[4]) {
    return linear_hash(strtoint(k));
}

int direct_address_insert(DIRECT_HT *ht, const char key[4], const char *val) {
    HT_ENTRY *new_entry = calloc(1, sizeof(HT_ENTRY));
    strncpy(new_entry->key, key, 4);
    new_entry->val = val;

    ht->entries[hash_key(key)] = new_entry;
    return 1;
}

int direct_address_delete(DIRECT_HT *ht, const HT_ENTRY *entry) {
    ht->entries[hash_key(entry->key)] = NULL;
    free(entry);

    return 1;
}

HT_ENTRY *direct_address_search(const DIRECT_HT *ht, const char key[4]) {
    return ht->entries[hash_key(key)];
}

DIRECT_HT *direct_address_create(const int sz) {
    DIRECT_HT *new_ht = calloc(1, sizeof(DIRECT_HT));
    new_ht->size = sz;
    new_ht->entries = calloc(sz, sizeof(HT_ENTRY *));
    return new_ht;
}

int direct_address_destroy(DIRECT_HT *ht) {
    for (int i = 0; i < ht->size; i++) if (ht->entries[i]) free(ht->entries[i]);
    free(ht->entries);

    return 1;
}

