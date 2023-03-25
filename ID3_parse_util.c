#include "mp3_metadata.h"

ID3V2_HEADER *read_header(FILE *f) {
    ID3V2_HEADER* header = malloc(sizeof(ID3V2_HEADER));

    if (fread(header->fid, 1, 3, f) != 3) {
        printf("Error occurred reading file identifier.\n");
        exit(1);
    }

    if (fread(header->ver, 1, 2, f) != 2) {
        printf("Error occurred reading version.\n");
        exit(1);
    }

    if (fread(&header->flags, 1, 1, f) != 1) {
        printf("Error occurred flags.\n");
        exit(1);
    }

    if (fread(&header->size, 1, 4, f) != 4) {
        printf("Error occurred tag size.\n");
        exit(1);
    }

    int metadata_size = synchsafeint32ToInt(header->size);

    printf("Header: \n");
    printf("\tFile Identifier: %c%c%c\n", header->fid[0], header->fid[1], header->fid[2]);
    printf("\tVersion: 2.%d.%d\n", header->ver[0], header->ver[1]);
    printf("\tFlags:\n");
    printf("\t\tUnsynchronisation: %d\n", header->flags >> 7);
    printf("\t\tExtended Header: %d\n", header->flags >> 6);
    printf("\t\tExp. Indicator: %d\n", header->flags >> 5);
    printf("\t\tFooter present: %d\n", header->flags >> 4);
    printf("\tTag Size: %d\n\n", metadata_size);

    return header;
}



ID3_METAINFO *get_ID3_meta_info(FILE *f, ID3V2_HEADER *header, int metadata_alloc) {
    fseek(f, 10, SEEK_SET);
    int sz = 0;
    int frames = 0;
    
    while (sz < metadata_alloc) {
        ID3V2_FRAME_HEADER frame_header;
        if (fread(frame_header.fid, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }

        if (strlen(frame_header.fid) == 0) break;

        if (fread(frame_header.size, 1, 4, f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);

        fseek(f, 2+frame_data_sz, SEEK_CUR);
        sz += 4 + 4 + 2 + frame_data_sz;
        frames += 1;
    }

    fseek(f, 10, SEEK_SET);

    ID3_METAINFO *info = malloc(sizeof(ID3_METAINFO));
    info->metadata_sz = sz;
    info->frame_count = frames;
    info->fids = malloc(frames*sizeof(char[4]));

    for (int i = 0; i < frames; i++) {
        if (fread(info->fids + i, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }
        char ss_sz[4];
        if (fread(&ss_sz, 1, sizeof(int), f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(ss_sz);
        fseek(f, 2 + frame_data_sz, SEEK_CUR);
    }

    fseek(f, 10, SEEK_SET);
    return info;
}