#include <stdio.h>

#include "id3.h"

extern ID3V2_HEADER *read_header(ID3V2_HEADER *header, FILE *f, char *filename, int verbose);

extern int parse_frame_header_flags(char flags[2], int *readonly, FILE *f);

extern ID3V2_FRAME_HEADER *read_frame_header(ID3V2_FRAME_HEADER *h, FILE *f);

extern int read_data(const ID3_METAINFO metainfo, char **data, int *sizes, FILE *f);

extern void print_data(FILE *f, ID3_METAINFO metainfo);

extern ID3_METAINFO *get_ID3_metainfo(ID3_METAINFO *metainfo, ID3V2_HEADER *header, FILE *f, int verbose);

extern int sizeof_frame_data(char fid[4], const char arg_data[256]);

extern char *get_frame_data(char fid[4], const char arg_data[256]);