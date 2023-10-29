#ifndef HEADER_INC
#define HEADER_INC

#include <stdio.h>

#define IS_SET(X,Y) ((X >> Y) & 0b1)
#define IS_READONLY(X) IS_SET(X,4)

#define E_FIDS 5

char fids[E_FIDS][5] = {"TPE1", "TALB", "TIT2", "TRCK", "APIC"};

typedef struct ID3V2_HEADER {
    char fid[3];
    char ver[2];
    char flags;
    char size[4]; 
} ID3V2_HEADER;

typedef struct ID3V2_EXT_HEADER {
    char size[4];
    char num_bytes;
    char flags;
} ID3V2_EXT_HEADER;

typedef struct ID3V2_FRAME_HEADER {
    char fid[4];
    char size[4];
    char flags[2];
} ID3V2_FRAME_HEADER;

typedef struct ID3_METAINFO {
    int metadata_sz; // Size in bytes of used metadata
    int frame_count;
    char (*fids)[4];
    int *fid_sz;
    int frame_pos;
} ID3_METAINFO;

#endif