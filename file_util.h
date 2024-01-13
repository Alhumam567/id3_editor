#include <stdio.h>

#include "id3.h"

extern void append_new_frame(ID3V2_FRAME_HEADER header, char *data, int new_data_sz, FILE *f);

extern int read_frame_data(FILE *f, int len_data);

extern void edit_frame_data(char *new_data, int new_data_len, int prev_data_len, int remaining_metadata_sz, int additional_bytes, FILE *f);

extern FILE *extend_header(int additional_metadata_sz, ID3_METAINFO header_metainfo, FILE *f, char *old_filename);

extern int isJPEG(char *filepath);

extern int file_copy(const char *src, const char *dst);