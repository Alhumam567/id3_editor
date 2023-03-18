#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ID3V2_HEADER {
    char fid[3];
    char ver[2];
    char flags;
    char size[4];
} ID3V2_HEADER;

typedef struct ID3V2_FRAME_HEADER {
    char fid[4+1];
    char size[4];
    char flags[2];
} ID3V2_FRAME_HEADER;

int synchsafeint32ToInt(char i[4]) {
    return (i[0] << 21) | ((i[1] << 14) | ((i[2] << 7) | (i[3] | (int)0)));
}

int main(int argc, int *argv) {
    FILE *f = fopen("./testfile.mp3", "rb");

    if (f == NULL) {
        printf("File does not exist.\n");
        exit(1);
    }

    ID3V2_HEADER* header = malloc(sizeof(ID3V2_HEADER));

    int fid_sz = fread(header->fid, 1, 3, f);
    if (fid_sz != 3) {
        printf("Error occurred reading file identifier.\n");
        exit(1);
    }

    int ver_sz = fread(header->ver, 1, 2, f);
    if (ver_sz != 2) {
        printf("Error occurred reading version.\n");
        exit(1);
    }

    int flags_sz = fread(&header->flags, 1, 1, f);
    if (flags_sz != 1) {
        printf("Error occurred flags.\n");
        exit(1);
    }

    int size_sz = fread(&header->size, 1, 4, f);
    if (size_sz != 4) {
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
    printf("\tTag Size: %d\n", metadata_size);
    
    int i = 0;

    while (i < metadata_size) {
        ID3V2_FRAME_HEADER frame_header;

        fid_sz = fread(frame_header.fid, 1, 4, f);
        if (fid_sz != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }
        size_sz = fread(frame_header.size, 1, 4, f);
        if (size_sz != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        flags_sz = fread(frame_header.flags, 1, 2, f);
        if (flags_sz != 2) {
            printf("Error occurred flags.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);

        if (frame_data_sz > 0) { 
            printf("FID: %.4s, ", frame_header.fid);
            printf("Size: %d\n", frame_data_sz);
        }

        i += 10 + frame_data_sz;

        fseek(f, frame_data_sz, SEEK_CUR);
    }

    free(header);
    fclose(f);
    return 0;
}