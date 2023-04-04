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
    fseek(f, -6, SEEK_CUR); // Seek back to frame header size 

    char synchsafe_nl[4];
    intToSynchsafeint32(new_len + 1, synchsafe_nl);
    
    if (verbose) {
        for (int i=0; i < 4; i++)
            printf("%d\n",synchsafe_nl[i]);
    }
    
    fwrite(synchsafe_nl, 1, 4, f);

    fseek(f, 2, SEEK_CUR); // Seek past frame header flag

    return new_len;
}



void write_frame_data(char *data, FILE *f) {
    fseek(f, 1, SEEK_CUR);
    fwrite(data, strlen(data), 1, f);

    fflush(f);
}



void write_frame_header(ID3V2_FRAME_HEADER header, FILE *f) {
    fseek(f, 0, SEEK_CUR);

    int num_written = fwrite(&header, sizeof(ID3V2_FRAME_HEADER), 1, f);
    if (num_written < 1) {
        printf("Failed to write frame header\n");
        exit(1);
    }

    fflush(f);
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

    int new_len = strlen(new_data);

    if (new_len + 1 > old_data_sz) {
        fseek(f, old_data_sz, SEEK_CUR);

        rewrite_buffer(new_len + 1 - old_data_sz, remaining_metadata_sz, 0, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        fwrite(new_data, new_len, 1, f);
    } else if (new_len + 1 == old_data_sz) {
        fseek(f, 1, SEEK_CUR);
        fwrite(new_data, new_len, 1, f);    
    } else {
        fseek(f, old_data_sz, SEEK_CUR);

        rewrite_buffer(new_len + 1 - old_data_sz, remaining_metadata_sz, 1, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        fwrite(new_data, new_len, 1, f);
    }

    fseek(f,-1 * (new_len + 1),SEEK_CUR);
    fflush(f);

    return new_len + 1;
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



void print_data(FILE *f, int frames) {
    int bytes_read = 0; 
    char *data;
    char fid_str[5] = {'\0'};

    fseek(f, 10, SEEK_SET);
    
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

        bytes_read += 10 + frame_data_sz; 
    }
}
