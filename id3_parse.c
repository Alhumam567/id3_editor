#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id3.h"
#include "util.h"
#include "hashtable.h"

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
ID3V2_HEADER *read_header(ID3V2_HEADER *header, FILE *f, const char *filename, int verbose) {
    if (fread(header->fid, 1, 3, f) != 3) {
        printf("read_header: Error occurred reading file identifier.\n");
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

    int metadata_size = synchsafeint32ToInt(header->size); // HEADER SIZE IS SYNCHSAFE FOR ID3V2.3 and ID3V2.4
    if (verbose) {
        printf("%s Header: \n", filename);
        printf("\tFile Identifier: %c%c%c\n", header->fid[0], header->fid[1], header->fid[2]);
        printf("\tVersion: 2.%d.%d\n", header->ver[0], header->ver[1]);
        printf("\tFlags:\n");
        printf("\t\tUnsynchronisation: %d\n", header->flags >> 7);
        printf("\t\tExtended Header: %d\n", header->flags >> 6);
        printf("\t\tExp. Indicator: %d\n", header->flags >> 5);
        printf("\t\tFooter present: %d\n", header->flags >> 4);
        printf("\tTag Size: %d\n", metadata_size);
    }

    return header;
}


int parse_frame_header_flags(char flags[2], int *readonly, FILE *f);


/**
 * @brief Reads ID3 tag frame header. File pointer must be pointing to the start
 * of the ID3 tag frame header. Moves file pointer to the end of frame header.
 * 
 * @param h - Pointer to frame header struct to save data 
 * @param f - File pointer to read data from
 * @param err_str - Error message string header
 * @return ID3V2_FRAME_HEADER* - returns frame header struct <h> 
 */
ID3V2_FRAME_HEADER *read_frame_header(ID3V2_FRAME_HEADER *h, FILE *f, const char *err_str) {
    if (fread(h->fid, 4, 1, f) != 1) {
        printf("%s", err_str);
        printf("Error occurred reading file identifier.\n");
        exit(1);
    }
    if (fread(h->size, 4, 1, f) != 1) {
        printf("%s", err_str);
        printf("Error occurred tag size.\n");
        exit(1);
    }
    if (fread(h->flags, 2, 1, f) != 1) {
        printf("%s", err_str);
        printf("Error occurred flags.\n");
        exit(1);
    }

    return h;
}


/** Determines frame header size based on ID3 ver
 * @param metainfo - ID3 struct
 * @param size	   - char[4] ID3 size bytes
 * @return int - frame header byte size
 */
int get_frame_header_size(const ID3_METAINFO *metainfo, const char *size) {
	int _size;
	if (metainfo->is_ss) 
		_size = synchsafeint32ToInt(size);
	else {
		char rev[4] = {0};
		for (int i = 0; i < 4; i++) rev[i] = size[3-i];
		_size = *((int *) rev);
	}
	return _size;
}

/**
 * @brief Reads file frames and saves data and data size into hash tables <data>, <sizes>
 * 
 * @param metainfo - File metainfo struct
 * @param data - Data hash table to update
 * @param sizes - Data size hash table to update
 * @param f - ID3 file
 * @return int - Error code (pass=0)
 */
int read_data(const ID3_METAINFO metainfo, DIRECT_HT *data, DIRECT_HT *sizes, FILE *f) {
    fseek(f, metainfo.frame_pos, SEEK_SET);
    
    int *size;
    char *d;
    for (int i = 0; i < metainfo.frame_count; i++) {
        ID3V2_FRAME_HEADER frame_header;
        read_frame_header(&frame_header, f, "read_data: ");

        int readonly = 0;
        parse_frame_header_flags(frame_header.flags, &readonly, f);

        size = calloc(1, sizeof(int));
		*size = get_frame_header_size(&metainfo, frame_header.size);

        d = calloc(*size, sizeof(char));
        if (!fread(d, *size, 1, f)){
            printf("Error occurred reading frame data.\n");
            return 1;
        }

        direct_address_insert(sizes, frame_header.fid, size);
        direct_address_insert(data, frame_header.fid, d);
    }

    return 0;
}


/**
 * @brief Reads and prints ID3 file frame data
 * 
 * @param f - ID3 File
 * @param metainfo - Metainfo of <f>
 */
void print_data(FILE *f, const ID3_METAINFO *metainfo) {
    int frames = metainfo->frame_count;
    
    printf("Metadata Size: %d\n", metainfo->metadata_sz);
    printf("Frame Count: %d\n", metainfo->frame_count);
    printf("Frames: ");
    for (int i = 0; i < metainfo->fid_sz->buckets; i++) { 
        if (metainfo->fid_sz->entries[i]) 
			printf("%.4s(%d);", metainfo->fid_sz->entries[i]->key, *(int*)metainfo->fid_sz->entries[i]->val);
    }
    printf("\n");

    int bytes_read = 0; 
    char *data;
    char fid_str[5] = {'\0'};

    fseek(f, metainfo->frame_pos, SEEK_SET);
    
    // Read Final Data
    for (int i = 0; i < frames; i++) {
        ID3V2_FRAME_HEADER frame_header;
        read_frame_header(&frame_header, f, "print_data: ");

        int readonly = 0;
        int additional_bytes = parse_frame_header_flags(frame_header.flags, &readonly, f);

        int frame_data_sz = get_frame_header_size(metainfo, frame_header.size);

        strncpy(fid_str, frame_header.fid, 4);

        printf("FID: %.4s, ", fid_str);
        printf("Size: %d\n", frame_data_sz);

        data = malloc(frame_data_sz+1);
        data[frame_data_sz] = '\0';
        
        if (fread(data, 1, frame_data_sz, f) != frame_data_sz){
            printf("2. Error occurred reading frame data.\n");
            exit(1);
        }

        if (strncmp(fid_str, "APIC", 4) != 0) {
            // Printing char array with intermediate null chars
            printf("\tData: ");
            for (int i = 0; i < frame_data_sz; i++) {
                if (data[i] != '\0') printf("%c", data[i]);
            }
            printf("\n");
        }  
        else printf("\tImage\n");

        free(data);

        bytes_read += sizeof(ID3V2_FRAME_HEADER) + frame_data_sz + additional_bytes; 
    }
}




int parse_ext_header_flags(ID3V2_EXT_HEADER *ext_header, ID3V2_HEADER *header, FILE *f) {
    fread(ext_header->size, 4, 1, f);

    int ext_header_sz;
	if (header->ver[0] != 3) ext_header_sz = synchsafeint32ToInt(ext_header->size);
	else { 
		char rev[4] = {0};
		for (int i = 0; i < 4; i++) rev[i] = ext_header->size[3-i];
		ext_header_sz = *((int *) rev);
	}

    fread(&(ext_header->num_bytes), 1, 1, f);
    if (ext_header->num_bytes != 1) {
        printf("Error reading extended header, number of flag bytes is not 1.\n");
        exit(1);
    }

    fread(&(ext_header->flags), 1, 1, f);

    fseek(f, ext_header_sz - 6, SEEK_CUR);

    return ext_header_sz;
}



int parse_header_flags(ID3V2_HEADER *header, FILE *f) {
    int frame_pos = 10;

    if (IS_SET(header->flags, 6)) { // Extended Header bit
        ID3V2_EXT_HEADER ext_header;
        frame_pos = parse_ext_header_flags(&ext_header, header, f) + sizeof(ID3V2_FRAME_HEADER);
    }

    return frame_pos;
}


/**
 * @brief Parses frame header flags to calculate any additional bytes added through 
 * data length indicator bits, encryption bits, etc.. and reads past their position to 
 * setup for reading frame data
 * 
 * @param flags - Flags where flag[0] is the frame status byte, and flag[1] is the frame format byte
 * @param readonly - Boolean for readonly bit 
 * @return int - Number of additional bytes between frame header and frame data
 */
int parse_frame_header_flags(char flags[2], int *readonly, FILE *f) {
    int additional_bytes = 0;

    if (IS_READONLY(flags[0])) *readonly = 1;
    if (IS_SET(flags[1], 6)) additional_bytes++; // Grouping Identity Byte
    if (IS_SET(flags[1], 2)) additional_bytes++; // Encryption Type Byte
    if (IS_SET(flags[1], 0)) additional_bytes+=4; // Data length indicator bit set, additional synchsafe int
    
    fseek(f, additional_bytes, SEEK_CUR);

    return additional_bytes; 
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
ID3_METAINFO *get_ID3_metainfo(ID3_METAINFO *metainfo, FILE *f, const char *filename, int verbose) {
    ID3V2_HEADER *header = &(metainfo->header);
    fseek(f, 10, SEEK_SET); // Set FILE * to end of header
    metainfo->frame_pos = parse_header_flags(header, f); // Parse header flags and seek past extended header if necessary
    metainfo->is_ss = (header->ver[0] == 3) ? 0 : 1;
    read_header(&(metainfo->header), f, filename, verbose);

    int sz = 0;
    int frames = 0;
    int metadata_alloc = synchsafeint32ToInt(header->size);
    
    // Count FILE *f metadata byte size and number of ID3 frames
    while (sz < metadata_alloc) {
        ID3V2_FRAME_HEADER frame_header;
        read_frame_header(&frame_header, f, "get_id3_metadata (1): ");
        if (frame_header.fid[0] == '\0') break; // End of frame data

        int readonly = 0;
        int additional_bytes = parse_frame_header_flags(frame_header.flags, &readonly, f);

        int frame_data_sz = get_frame_header_size(metainfo, frame_header.size);
        fseek(f, frame_data_sz, SEEK_CUR);
        sz += sizeof(ID3V2_FRAME_HEADER) + additional_bytes + frame_data_sz; // #fid_bytes + #sz_bytes + #flags_bytes + size of frame data
        frames += 1;
    }

    fseek(f, metainfo->frame_pos, SEEK_SET);
    
    metainfo->metadata_sz = sz;
    metainfo->frame_count = frames;
    metainfo->fid_sz = direct_address_create(MAX_HASH_VALUE, &all_fids_hash);

    // Save ID3 frame IDs and sizes
    for (int i = 0; i < frames; i++) {
        ID3V2_FRAME_HEADER frame_header;
        read_frame_header(&frame_header, f, "get_id3_metadata (2): ");

        int *frame_data_sz = calloc(1, sizeof(int));
        *frame_data_sz = get_frame_header_size(metainfo, frame_header.size); 
        direct_address_insert(metainfo->fid_sz, frame_header.fid, frame_data_sz);

        fseek(f, *frame_data_sz, SEEK_CUR);
    }

    if (verbose) {
        printf("Metadata Size: %d\n", metainfo->metadata_sz);
        printf("Frame Count: %d\n", metainfo->frame_count);
        printf("Frames: ");
        for (int i = 0; i < metainfo->fid_sz->buckets; i++) { 
            if (metainfo->fid_sz->entries[i]) printf("%.4s(%d);", metainfo->fid_sz->entries[i]->key, *(int*)metainfo->fid_sz->entries[i]->val);
        }
        printf("\n\n");
    }

    fseek(f, metainfo->frame_pos, SEEK_SET);
    
    return metainfo;
}


/**
 * @brief Calculates the size of the given frame data. 
 * 
 * Text Information Frames: Include an encoding byte prior to text data, refer to TEXT_FRAME type
 * Attached Picture Frames: Include an encoding byte, MIME type, picture type, and description prior to 
 * text data, refer to APIC_FRAME type
 * 
 * @param fid - Frame ID 
 * @param arg_data - Provided argument data
 * @return int - Size of the frame
 */
int sizeof_frame_data(char fid[4], const char *arg_data) {
    int sz = 0;
    int id;

    if ((id = get_index(t_fids, T_FIDS, fid)) != -1) { // Text information frame
        sz += sizeof(TEXT_FRAME) + strlen(arg_data);
    } else if ((id = get_index(s_fids, S_FIDS, fid)) != -1) { // Attached Picture Frame
        switch(id) {
            case 0: {
                FILE *f = fopen(arg_data, "rb");
                fseek(f, 0, SEEK_END);

                sz += 1 + strlen("image/jpeg") + 1 + 1 + 1 + ftell(f);
                fclose(f);
                break;
            }
            default:
                break;
        } 
    } 

    return sz;
}


/**
 * @brief Byte array of the entire frame data, including necessary data header info (encoding, image type, etc.)
 * 
 * @param fid - Frame ID
 * @param arg_data - Provided argument data
 * @return char* - Frame data byte array
 */
char *get_frame_data(char fid[4], const char arg_data[256]) { 
    int sz = sizeof_frame_data(fid, arg_data);
    char *frame_data = malloc(sz + 1);
    int id;

    frame_data[sz] = '\0';

    if ((id = get_index(t_fids, T_FIDS, fid)) != -1) { // Text information frame
        frame_data[0] = '\0';
        strncpy(frame_data + 1, arg_data, strlen(arg_data));
    } else if ((id = get_index(s_fids, S_FIDS, fid)) != -1) { // Attached Picture Frame
        switch(id) {
            case 0: {
                FILE *f = fopen(arg_data, "rb");
                char *mime_type = "\0image/jpeg";
                int mime_type_len = strlen(mime_type);
                int i = 0;

                frame_data[i++] = '\0'; // text encoding

                // MIME type
                strncpy(frame_data + i, mime_type, mime_type_len);
                i += mime_type_len;
                frame_data[i++] = '\0';

                frame_data[i++] = '\0'; // picture type
                frame_data[i++] = '\0'; // description

                // Picture data
                fseek(f, 0, SEEK_END);
                int pic_data_len = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (fread(frame_data + i, pic_data_len, 1, f) != 1) {
                    printf("Failed to read picture data.\n");
                    fclose(f);
                    exit(1);
                }

                fclose(f);
                break;
            }
            default:
                break;
        } 
    } 

    return frame_data;
}
