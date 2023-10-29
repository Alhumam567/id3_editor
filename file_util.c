#include <stdio.h>

#include "id3_editor.h"
#include "util.c"


int get_frame_data_len(ID3V2_FRAME_HEADER h) {
    return synchsafeint32ToInt(h.size);
}



/**
 * @brief Writes new length as synchsafe int of size 4. File pointer must be pointing to first byte (big endian)
 * 
 * @param new_len - New length of ID3 frame data (must include initial null byte)
 * @param f - File pointer
 * @param verbose - Prints out new length integer as synchsafe int 
 * @return int - returns new length
 */
int write_new_len(int new_len, FILE *f, int verbose) {
    char synchsafe_nl[4];
    intToSynchsafeint32(new_len + 1, synchsafe_nl);
    
    if (verbose) {
        for (int i=0; i < 4; i++)
            printf("%d\n",synchsafe_nl[i]);
    }
    
    fwrite(synchsafe_nl, 1, 4, f);

    return new_len + 1;
}


/**
 * @brief Writes frame data when file pointer points to the beginning of the frame
 * data block.
 * 
 * @param data - New data to be written 
 * @param f - File pointer
 */
void write_frame_data(char *data, FILE *f) {
    fseek(f, 1, SEEK_CUR);

    int written = fwrite(data, strlen(data), 1, f);
    if (written < 1) {
        printf("Failed to write frame data\n");
        exit(1);
    }

    fflush(f);
}


/**
 * @brief Writes frame header when file pointer points to the beginning of the frame
 * header block.
 * 
 * @param data - New data to be written 
 * @param f - File pointer
 */
void write_frame_header(ID3V2_FRAME_HEADER header, FILE *f) {
    fseek(f, 0, SEEK_CUR);

    int written = fwrite(&header, sizeof(ID3V2_FRAME_HEADER), 1, f);
    if (written < 1) {
        printf("Failed to write frame header\n");
        exit(1);
    }

    fflush(f);
}



void append_new_frame(ID3V2_FRAME_HEADER header, char *data, FILE *f) {
    write_frame_header(header, f);
    write_frame_data(data, f);
}



/**
 * @brief Helper function for rewriting a buffer of bytes in order to accommodate larger
 * and smaller frame data for intermediate frames. Buffer of size <remaining_metadata_size>
 * will be rewritten at its current position + offset. If zero_buf is enabled, the buffer
 * will be nulled prior to rewriting.  
 * 
 * @param offset - Number of bytes to move buffer by, can be negative
 * @param remaining_metadata_size - Remaining metadata bytes from current file pointer <f> position till the end
 * @param zero_buf - Boolean for zeroing buffer prior to rewrite
 * @param f - File pointer
 */
void rewrite_buffer(signed int offset, int remaining_metadata_size, int zero_buf, FILE *f) {
    int buf_size = remaining_metadata_size;
    char *buf = malloc(buf_size);
    fread(buf, 1, buf_size, f);

    if (zero_buf) {
        fseek(f, -1*buf_size, SEEK_CUR);
        char *emp_buf = calloc(buf_size, 1);
        fwrite(emp_buf, 1, buf_size, f);
    }

    fseek(f, -1*(buf_size) + offset, SEEK_CUR);
    fwrite(buf, 1, buf_size, f);

    fseek(f,-1*buf_size,SEEK_CUR);

    free(buf);
}


/**
 * @brief Helper function overwriting any existing frame data with a new, possibly longer or shorter,
 * array of bytes. 
 * 
 * @param data - New data to write
 * @param old_data_sz - Byte size of the old data
 * @param remaining_metadata_sz - Size of the remaining buffer of ID3 metadata 
 * @param f - File pointer
 * @return int - Returns the number of bytes written
 */
int overwrite_frame_data(char *new_data, int old_data_sz, int remaining_metadata_sz, FILE *f) {
    //Clear current data
    char *null_buf = calloc(old_data_sz, 1);
    fwrite(null_buf, old_data_sz, 1, f);
    fseek(f, -1 * old_data_sz, SEEK_CUR);
    free(null_buf);

    int new_len = strlen(new_data) + 1; // Includes null byte in total length

    if (new_len > old_data_sz) {
        fseek(f, old_data_sz, SEEK_CUR);

        rewrite_buffer(new_len - old_data_sz, remaining_metadata_sz, 0, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        write_frame_data(new_data, f);
    } else if (new_len == old_data_sz) {
        write_frame_data(new_data, f);
    } else {
        fseek(f, old_data_sz, SEEK_CUR);

        rewrite_buffer(new_len - old_data_sz, remaining_metadata_sz, 1, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        write_frame_data(new_data, f);
    }

    fseek(f,-1 * new_len,SEEK_CUR);
    fflush(f);

    return new_len;
}



void int_to_header_ssint(int new_len, char header_sz[4]) {
    intToSynchsafeint32(new_len, header_sz);
}



/**
 * @brief Moves file pointer past frame data to the next frame. 
 * 
 * @param f - File pointer
 * @param len_data - Full length of the data
 * @return int - Bytes read
 */
int read_frame_data(FILE *f, int len_data) {
    char *data = malloc(len_data + 1);
    data[len_data] = '\0';
    int bytes_read = fread(data, 1, len_data, f);

    if (bytes_read != len_data){
        printf("Error occurred reading frame data. %d %d\n", bytes_read, len_data);
        exit(1);
    }

    free(data);

    return len_data;
}



void edit_frame_data(char *data, int *prev_len_data, int remaining_metadata_sz, int additional_bytes, FILE *f) {
    fseek(f, -1 * (6 + additional_bytes), SEEK_CUR); 
    int new_len_data = write_new_len(strlen(data), f, 0);
    fseek(f, 2 + additional_bytes, SEEK_CUR); 

    overwrite_frame_data(data, *prev_len_data, remaining_metadata_sz, f);

    *prev_len_data = new_len_data;
}



void print_data(FILE *f, ID3_METAINFO metainfo) {
    int frames = metainfo.frame_count;

    printf("Printing file info:\n");
    printf("\tMetadata Size: %d\n", metainfo.metadata_sz);
    printf("\tFrame Count: %d\n", metainfo.frame_count);
    printf("\tFrames: ");
    for (int i = 0; i < metainfo.frame_count; i++) printf("%.4s;", metainfo.fids[i]);
    printf("\n");

    int bytes_read = 0; 
    char *data;
    char fid_str[5] = {'\0'};

    fseek(f, metainfo.frame_pos, SEEK_SET);
    
    // Read Final Data
    for (int i = 0; i < frames; i++) {
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

        int readonly = 0;
        int additional_bytes = parse_frame_header_flags(frame_header.flags, &readonly, f);

        int frame_data_sz = synchsafeint32ToInt(frame_header.size);
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



void extend_header(int additional_mtdt_sz, 
                   ID3_METAINFO header_metainfo,
                   FILE *f,
                   char *old_filename) {            
    int additional_sz = additional_mtdt_sz + 2000;
    int new_sz = header_metainfo.metadata_sz + additional_sz;
    
    FILE *f2 = fopen("tmp.mp3", "w+b"); // TODO: Change tmp file naming
    
    int buf_sz = header_metainfo.metadata_sz + sizeof(ID3V2_HEADER);
    char *buf = malloc(buf_sz);

    char *empty_buf = calloc(additional_sz, 1);
    
    fseek(f, 0, SEEK_SET);
    fread(buf, buf_sz, 1, f);

    fseek(f2, 0, SEEK_SET);
    fwrite(buf, buf_sz, 1, f2);
    fwrite(empty_buf, additional_sz, 1, f2);

    fseek(f, 0, SEEK_END);
    int mp3_buf_sz = ftell(f) - buf_sz;
    char *mp3_buf = malloc(mp3_buf_sz); 
    fseek(f, buf_sz, SEEK_SET);
    fread(mp3_buf, mp3_buf_sz, 1, f);

    fwrite(mp3_buf, mp3_buf_sz, 1, f2);

    fclose(f);
    fflush(f2);
    fclose(f2);

    if (remove(old_filename) != 0) {
        printf("Delete file failed.\n");
        exit(1);
    }
    if (rename("tmp.mp3", old_filename) != 0) {
        printf("Rename file failed.\n");
        exit(1);
    }
    f2 = fopen(old_filename, "r+b");
    if (f2 == NULL) {
        printf("File does not exist.\n");
        exit(1);
    }
    fseek(f2, 6, SEEK_SET);

    char ssint[4];
    intToSynchsafeint32(new_sz, ssint);
    fwrite(ssint, 4, 1, f2);

    fseek(f2, header_metainfo.frame_pos, SEEK_SET);
    fclose(f2);

    free(buf);
    free(mp3_buf);
}


int isJPEG(char *filepath) {
    FILE *img = fopen(filepath, "rb");
    unsigned char buf[4];
    unsigned char jfifHeader[] = {0xFF, 0xD8, 0xFF, 0xE0};
    int bytes_read = fread(buf, 4, 1, img);

    // SOI
    if (bytes_read != 1 || strncmp(buf, jfifHeader, 4) != 0) {
        fclose(img);
        printf("\n 1 Error: image specified is not JPEG.\n");
        return 0;
    }

    unsigned char idBuf[5];
    unsigned char jfifId[] = {0x4A, 0x46, 0x49, 0x46, 0x0};
    fseek(img, 2, SEEK_CUR);
    bytes_read = fread(idBuf, 5, 1, img);

    // ID
    if (bytes_read != 1 || strncmp(idBuf, jfifId, 5) != 0) {
        fclose(img);
        printf("\n 3 Error: image specified is not JPEG.\n");
        return 0;
    }

    return 1;
}
