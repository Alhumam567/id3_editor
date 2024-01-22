#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id3.h"
#include "util.h"



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
    intToSynchsafeint32(new_len, synchsafe_nl);
    
    if (verbose) {
        for (int i=0; i < 4; i++)
            printf("%d\n",synchsafe_nl[i]);
    }
    
    fwrite(synchsafe_nl, 1, 4, f);

    return new_len;
}


/**
 * @brief Writes frame data when file pointer points to the beginning of the frame
 * data block.
 * 
 * @param data - New data to be written 
 * @param new_data_sz - Length of bytes of new_data
 * @param f - File pointer
 */
void write_frame_data(char *data, int new_data_sz, FILE *f) {
    fseek(f, 0, SEEK_CUR);

    int written = fwrite(data, new_data_sz, 1, f);
    if (written < 1) {
        printf("Failed to write frame data\n");
        printf("%d\n", new_data_sz);

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


/**
 * @brief Appends a new frame in the current position of <f>
 * 
 * @param header - Frame header of new frame
 * @param data - Data of new frame 
 * @param new_data_sz - size of frame data
 * @param f - File
 */
void append_new_frame(ID3V2_FRAME_HEADER header, char *data, int new_data_sz, FILE *f) {
    write_frame_header(header, f);
    write_frame_data(data, new_data_sz, f); 
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
        free(emp_buf);
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
 * @param new_data_sz - Byte size of the new frame data
 * @param old_data_sz - Byte size of the old data
 * @param remaining_metadata_sz - Size of the remaining buffer of ID3 metadata 
 * @param f - File pointer
 */
void overwrite_frame_data(char *new_data, int new_data_sz, int old_data_sz, int remaining_metadata_sz, FILE *f) {
    //Clear current data
    char *null_buf = calloc(old_data_sz, 1);
    fwrite(null_buf, old_data_sz, 1, f);
    fseek(f, -1 * old_data_sz, SEEK_CUR);
    free(null_buf);

    // Shift remaining metadata if the new frame is different sized than the current
    if (new_data_sz != old_data_sz) {
        fseek(f, old_data_sz, SEEK_CUR);

        int zero_buf = (new_data_sz > old_data_sz) ? 0 : 1;
        rewrite_buffer(new_data_sz - old_data_sz, remaining_metadata_sz, zero_buf, f);
        
        fseek(f, -1 * new_data_sz, SEEK_CUR);
    } 

    write_frame_data(new_data, new_data_sz, f);

    fseek(f, -1 * new_data_sz, SEEK_CUR);
    fflush(f);
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


/**
 * @brief Edits existing frame data where <f> file position points to the start of frame
 * 
 * @param new_data - New data to update frame
 * @param new_data_len - Length of new data
 * @param prev_data_len - Length of current data
 * @param remaining_metadata_sz - Metadata size remaining in header
 * @param additional_bytes - Length of additional bytes in frame header
 * @param f - File
 */
void edit_frame_data(char *new_data, int new_data_len, int prev_data_len, int remaining_metadata_sz, int additional_bytes, FILE *f) {
    // Return file pointer to beginning of new length and write new length
    fseek(f, -1 * (6 + additional_bytes), SEEK_CUR); 
    write_new_len(new_data_len, f, 0);
    fseek(f, 2 + additional_bytes, SEEK_CUR); 

    overwrite_frame_data(new_data, new_data_len, prev_data_len, remaining_metadata_sz, f);
}


/**
 * @brief Extends ID3 file to accomodate extra header space
 * 
 * @param additional_mtdt_sz - Extra space needed
 * @param header_metainfo - File metainfo struct
 * @param f - File to extend
 * @param old_filename - Filename of <f>
 * @return FILE* - new FILE *
 */
FILE* extend_header(int additional_mtdt_sz, 
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
    free(empty_buf);

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

    char ssint[4] = {0};
    intToSynchsafeint32(new_sz, ssint);
    fwrite(ssint, 4, 1, f2);

    fseek(f2, header_metainfo.frame_pos, SEEK_SET);

    free(buf);
    free(mp3_buf);

    return f2;
}


int isJPEG(char *filepath) {
    FILE *img = fopen(filepath, "rb");
    char buf[4];
    char jfifHeader[] = {0xFF, 0xD8, 0xFF, 0xE0};
    int read = fread(buf, 4, 1, img);

    // SOI
    if (read != 1 || strncmp(buf, jfifHeader, 4) != 0) {
        fclose(img);
        printf("\n 1 Error: image specified is not JPEG.\n");
        return 0;
    }

    char idBuf[5];
    char jfifId[] = {0x4A, 0x46, 0x49, 0x46, 0x0};
    fseek(img, 2, SEEK_CUR);
    read = fread(idBuf, 5, 1, img);

    // ID
    if (read != 1 || strncmp(idBuf, jfifId, 5) != 0) {
        fclose(img);
        printf("\n 3 Error: image specified is not JPEG.\n");
        return 0;
    }

    return 1;
}


int file_copy(const char *src, const char *dst) {
	FILE *src_f = fopen(src, "rb");
	FILE *dst_f = fopen(dst, "wb");

	if (src_f == NULL) return -1;

	char buf[1024];
	while (!feof(src_f)) {
		int bytes_read = fread(buf, 1, sizeof(buf), src_f);
		fwrite(buf, 1, bytes_read, dst_f);
	}

	fclose(src_f);
	fclose(dst_f);

	return 0;
}
