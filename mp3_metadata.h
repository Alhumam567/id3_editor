#ifndef HEADER_INC
#define HEADER_INC

#include <stdio.h>

typedef struct ID3V2_HEADER {
    char fid[3];
    char ver[2];
    char flags;
    char size[4];
} ID3V2_HEADER;

typedef struct ID3V2_FRAME_HEADER {
    char fid[4];
    char size[4];
    char flags[2];
} ID3V2_FRAME_HEADER;

typedef struct ID3_METAINFO {
    int metadata_sz;
    int frame_count;
    char **fids;
} ID3_METAINFO;

typedef struct dsarray {
    char str[5];
} ARR;

#endif