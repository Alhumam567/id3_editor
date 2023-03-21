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
    char fid[4];
    char size[4];
    char flags[2];
} ID3V2_FRAME_HEADER;

int synchsafeint32ToInt(char c[4]) {
    return (c[0] << 21) | ((c[1] << 14) | ((c[2] << 7) | (c[3] | (int)0)));
}

char *intToSynchsafeint32(int x) {
    char *ssint = malloc(4);
    for (int i=0; i<4; i++) ssint[3-i] = (x & (0x7F << i*7)) >> i*7;
    return ssint;
}

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

int get_metadata_size(FILE *f, ID3V2_HEADER *header, int metadata_alloc) {
    int i = 0;
    
    while (i < metadata_alloc) {
        ID3V2_FRAME_HEADER frame_header;
        if (fread(frame_header.fid, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }

        if (strlen(frame_header.fid) == 0) return i;

        if (fread(frame_header.size, 1, 4, f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);

        fseek(f, 2+frame_data_sz, SEEK_CUR);
        i += 4 + 4 + 2 + frame_data_sz;
    }

    return i;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }

    FILE *f = fopen(argv[1], "r+b");

    if (f == NULL) {
        printf("File does not exist.\n");
        exit(1);
    }

    printf("Mode: Reading metadata\n\n");

    ID3V2_HEADER *header = read_header(f);
    int metadata_alloc = synchsafeint32ToInt(header->size);
    
    printf("Metadata found: \n");

    fseek(f, 10, SEEK_SET);
    int metadata_sz = get_metadata_size(f, header, synchsafeint32ToInt(header->size));
    fseek(f, 10, SEEK_SET);

    printf("%d\n", metadata_sz);

    char *new_title = "Char";

    int i = 0;
    char *data;
    char fid_str[5] = {'\0'};

    // ISSUE, <metadata_sz> is variable when editing metadata
    while (i < metadata_sz) {
        ID3V2_FRAME_HEADER frame_header;
        
        if (fread(frame_header.fid, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }
        if (fread(frame_header.size, 1, 4, f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        if (fread(frame_header.flags, 1, 2, f) != 2) {
            printf("Error occurred flags.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);
        strncpy(fid_str, frame_header.fid, 4);

        printf("FID: %.4s, ", fid_str);
        printf("Size: %d\n", frame_data_sz);

        if (strncmp(fid_str, "TIT2", 4) == 0) {
            int l = strlen(new_title);
            char *synchsafe_l = intToSynchsafeint32(l+1);
            for (int i=0; i < 4; i++) {
                printf("%d\n",synchsafe_l[i]);
            }
            fseek(f, -6, SEEK_CUR);
            fwrite(synchsafe_l, 1, 4, f);
            free(synchsafe_l);

            fseek(f,2 + 1,SEEK_CUR); // Seek past constant first null byte

            if (l > frame_data_sz-1) {
                fseek(f, frame_data_sz-1, SEEK_CUR);

                int buf_size = metadata_sz-(i+10+frame_data_sz);
                char *buf = malloc(buf_size);
                fread(buf, 1, buf_size, f);

                fseek(f, -1*(buf_size + frame_data_sz - 1), SEEK_CUR);
                fwrite(new_title, 1, strlen(new_title), f);
                fwrite(buf, 1, buf_size, f);

                fseek(f,-1*(buf_size + l + 1),SEEK_CUR);
            } else if (l == frame_data_sz - 1) {
                fwrite(new_title, 1, l, f);
                fseek(f,-1*(l + 1),SEEK_CUR);
            } else {
                fseek(f, frame_data_sz-1, SEEK_CUR);

                int buf_size = metadata_sz-(i+10+frame_data_sz);
                char *buf = malloc(buf_size);
                fread(buf, 1, buf_size, f);
                fseek(f, -1*buf_size, SEEK_CUR);
                char *emp_buf = calloc(buf_size, 1);
                fwrite(emp_buf, 1, buf_size, f);

                fseek(f, -1*(buf_size + frame_data_sz - 1), SEEK_CUR);
                fwrite(new_title, 1, strlen(new_title), f);
                fwrite(buf, 1, buf_size, f);

                fseek(f,-1*(buf_size + l + 1),SEEK_CUR);
            }
            
            frame_data_sz = l+1;
            fflush(f);
        }
        
        data = malloc(frame_data_sz+1);
        data[frame_data_sz] = '\0';
        
        if (fread(data, 1, frame_data_sz, f) != frame_data_sz){
            printf("Error occurred reading frame data.\n");
            exit(1);
        }

        if (strncmp(fid_str, "APIC", 4) != 0) {\
            // Printing char array with intermediate null chars
            printf("\tData: ");
            for (int j = 0; j < frame_data_sz; j++) {
                if (data[j] != '\0') printf("%c", data[j]);
            }
            printf("\n");
        }  
        else printf("\tData is an image\n");

        free(data);

        i += 10 + frame_data_sz; 
    }
    
    free(header);
    fclose(f);
    return 0;
}