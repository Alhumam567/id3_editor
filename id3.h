#ifndef HEADER_INC
#define HEADER_INC

#define IS_SET(X,Y) ((X >> Y) & 0b1)
#define IS_READONLY(X) IS_SET(X,4)

#define T_FIDS 4
#define t_fids_arr "TPE1", "TALB", "TIT2", "TRCK"
#define S_FIDS 1
#define s_fids_arr "APIC"
#define E_FIDS (T_FIDS + S_FIDS)

extern char t_fids[T_FIDS][5];
extern char s_fids[S_FIDS][5];
extern char fids[E_FIDS][5];

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
    int *frame_sz;
    int frame_pos;
} ID3_METAINFO;

typedef struct TEXT_FRAME {
    char encoding; 
} TEXT_FRAME;

typedef struct APIC_FRAME {
    char encoding;
    char *mime_type;
    char pic_type;
    char *description;
} APIC_FRAME;

#endif