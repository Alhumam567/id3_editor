/* Direct addressing hashtable implementation

 * Editable FIDs table:
 * universe of keys for editable frame IDs:
 * "TPE1"
 * "TALB"
 * "TIT2"
 * "TRCK"
 * "APIC"
 * 
 * hash function <e_fids_hash> ensures minimal and perfect hash table 
 * 
 * All FIDs table:
 * universe of keys for all frame IDs:
 * const char all_fids[83][5] = {
        "AENC", "APIC", "ASPI", "COMM", "COMR", "ENCR", "EQU2", "ETCO",
        "GEOB", "GRID", "LINK", "MCDI", "MLLT", "OWNE", "PRIV", "PCNT",
        "POPM", "POSS", "RBUF", "RVA2", "RVRB", "SEEK", "SIGN", "SYLT",
        "SYTC", "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDEN", "TDLY", 
        "TDOR", "TDRC", "TDRL", "TDTG", "TENC", "TEXT", "TFLT", "TIPL", 
        "TIT1", "TIT2", "TIT3", "TKEY", "TLAN", "TLEN", "TMCL", "TMED", 
        "TMOO", "TOAL", "TOFN", "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", 
        "TPE3", "TPE4", "TPOS", "TPRO", "TPUB", "TRCK", "TRSN", "TRSO", 
        "TSOA", "TSOP", "TSOT", "TSRC", "TSSE", "TSST", "TXXX", "UFID", 
        "USER", "USLT", "WCOM", "WCOP", "WOAF", "WOAR", "WOAS", "WORS", 
        "WPAY", "WPUB", "WXXX"
    };
 * 
 * hash function <all_fids_hash> is only perfect but not minimal
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hashtable.h"
#include "id3.h"
#include "id3_hash.h"

const char e_fids_reverse_lookup[E_FIDS][5] = { "TALB", "TIT2", "APIC", "TRCK", "TPE1" };

unsigned int linear_hash(const int a, const int b, const int p, const unsigned int k) {
	return (a*k + b) % p;
}

unsigned int strtoint(const char k[4]) {
	return (k[3] * pow(36,3)) + (k[2] * pow(36,2)) + (k[1] * 36) + k[0];
}

unsigned int e_fids_hash(const char k[4]) {
    return linear_hash(198, 199, 65805703, strtoint(k));
}

unsigned int all_fids_hash(const char k[4]) {
    return hash(k, 4);
}

int direct_address_insert(DIRECT_HT *ht, const char key[4], void *val) {
    if (ht->entries[ht->hash_func(key) % ht->buckets] != NULL) {
        free(ht->entries[ht->hash_func(key) % ht->buckets]->val);
        ht->entries[ht->hash_func(key) % ht->buckets]->val = val;
        return 2;
    }
    
    HT_ENTRY *new_entry = calloc(1, sizeof(HT_ENTRY));
    strncpy(new_entry->key, key, 4);
    new_entry->val = val;

    ht->entries[ht->hash_func(key) % ht->buckets] = new_entry;
    ht->sz++;
    return 1;
}

int direct_address_delete(DIRECT_HT *ht, HT_ENTRY *entry) {
    if (ht->entries[ht->hash_func(entry->key) % ht->buckets] == NULL) return 0;

    ht->entries[ht->hash_func(entry->key) % ht->buckets] = NULL;
    free(entry->val);
    free(entry);
    ht->sz--;

    return 1;
}

HT_ENTRY *direct_address_search(const DIRECT_HT *ht, const char key[4]) {
    return ht->entries[ht->hash_func(key) % ht->buckets];
}

int in_key_set(const DIRECT_HT *ht, const char str[4]) {
    int i = ht->hash_func(str) % ht->buckets;
    return ht->entries[i] != NULL && !strncmp(ht->entries[i]->key, str, 4);
}

DIRECT_HT *direct_address_create(const int buckets, unsigned int (*hash_func)(const char key[4])) {
    DIRECT_HT *new_ht = calloc(1, sizeof(DIRECT_HT));
    new_ht->sz = 0;
    new_ht->buckets = buckets;
    new_ht->entries = calloc(buckets, sizeof(HT_ENTRY *));
    new_ht->hash_func = hash_func;
    return new_ht;
}

int direct_address_destroy(DIRECT_HT *ht) {
    for (int i = 0; i < ht->buckets; i++) {
        if (ht->entries[i]) { 
            free(ht->entries[i]->val);
            free(ht->entries[i]);
        }
    } 
    free(ht->entries);
    free(ht);
    return 1;
}
