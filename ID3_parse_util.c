#include "id3_editor.h"
#include "util.c"

/**
 * @brief Reads ID3 header data of a file. File pointer must be pointing to the start
 * of the ID3 header block. Moves file pointer to the end of header data.
 * 
 * @param header    - Pointer to header struct to read data into 
 * @param f         - File pointing to ID3 header data 
 * @param filename  - Filename of <f>
 * @param verbose   - Boolean to print header data to stdout
 * @return ID3V2_HEADER* - returns header struct pointer <header>
 */
ID3V2_HEADER *read_header(ID3V2_HEADER *header, FILE *f, char *filename, int verbose) {
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

    if (verbose) {
        printf("%s Header: \n", filename);
        printf("\tFile Identifier: %c%c%c\n", header->fid[0], header->fid[1], header->fid[2]);
        printf("\tVersion: 2.%d.%d\n", header->ver[0], header->ver[1]);
        printf("\tFlags:\n");
        printf("\t\tUnsynchronisation: %d\n", header->flags >> 7);
        printf("\t\tExtended Header: %d\n", header->flags >> 6);
        printf("\t\tExp. Indicator: %d\n", header->flags >> 5);
        printf("\t\tFooter present: %d\n", header->flags >> 4);
        printf("\tTag Size: %d\n\n", metadata_size);
    }

    return header;
}


/**
 * @brief Reads ID3 tag frame header. File pointer must be pointing to the start
 * of the ID3 tag frame header. Moves file pointer to the end of frame header.
 * 
 * @param h - Pointer to frame header struct to save data 
 * @param f - File pointer to read data from
 * @return ID3V2_FRAME_HEADER* - returns frame header struct <h> 
 */
ID3V2_FRAME_HEADER *read_frame_header(ID3V2_FRAME_HEADER *h, FILE *f) {
    if (fread(h->fid, 1, 4, f) != 4) {
        printf("Error occurred reading file identifier.\n");
        exit(1);
    }
    if (fread(h->size, 1, 4, f) != 4) {
        printf("Error occurred tag size.\n");
        exit(1);
    }
    if (fread(h->flags, 1, 2, f) != 2) {
        printf("Error occurred flags.\n");
        exit(1);
    }

    return h;
}


/**
 * @brief Get the ID3 meta info (list of frames, size of metadata block) used for efficiently traversing file. 
 * File pointer will be moved to the end of ID3 header. 
 * 
 * @param metainfo - Pointer to metainfo struct to save data 
 * @param header   - ID3 header info
 * @param f        - File pointer 
 * @param verbose  - Prints metainfo to stdout
 * @return ID3_METAINFO* - returns pointer to metainfo struct <metainfo>
 */
ID3_METAINFO *get_ID3_metainfo(ID3_METAINFO *metainfo, ID3V2_HEADER *header, FILE *f, int verbose) {
    fseek(f, 10, SEEK_SET);

    int sz = 0;
    int frames = 0;
    int metadata_alloc = synchsafeint32ToInt(header->size);
    
    // Count FILE *f metadata byte size and number of ID3 frames
    while (sz < metadata_alloc) {
        ID3V2_FRAME_HEADER frame_header;
        if (fread(frame_header.fid, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }

        // End of frame data
        if (strlen(frame_header.fid) == 0) break;

        if (fread(frame_header.size, 1, 4, f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);

        fseek(f, 2+frame_data_sz, SEEK_CUR);
        sz += 4 + 4 + 2 + frame_data_sz; // #fid_bytes + #sz_bytes + #flags_bytes + size of frame data
        frames += 1;
    }

    fseek(f, 10, SEEK_SET);

    metainfo->metadata_sz = sz;
    metainfo->frame_count = frames;
    metainfo->fids = calloc(frames, sizeof(char *));

    // Save ID3 frame IDs
    for (int i = 0; i < frames; i++) {
        if (fread(metainfo->fids[i], 1, 4, f) != 4) {
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

    if (verbose) {
        printf("Metadata Size: %d\n", metainfo->metadata_sz);
        printf("Frame Count: %d\n", metainfo->frame_count);
        printf("Frames: ");
        for (int i = 0; i < metainfo->frame_count; i++) printf("%.4s;", metainfo->fids[i]);
        printf("\n\n");
    }

    fseek(f, 10, SEEK_SET);
    return metainfo;
}