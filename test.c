#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "id3.h"
#include "id3_parse.h"
#include "util.h"
#include "path.h"

/*
	Class of tests:
		1. Unit test Arguments:
			- TPE1: Single File, Multi file Folder
			- TRCK: Single File, Multi file Folder
				- Multi file different numbers
			- TIT2: Single File, Multi file Folder
				- Multi file multi titles
			- TALB: Single File, Multi file Folder
		2. Arguments Combined:
			- TPE1, TRCK, TIT2, TALB: Single File, Multi file Folder

		Test Inputs:
			- argument data array containing only the necessary test args
		
		Test Verification:
			- All frame data
			- Metadata size
*/

#define FAIL "\033[1;31m"
#define PASS "\033[1;32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURP "\033[1;35m"
#define RESET "\033[0m"

char t_fids[T_FIDS][5] = {t_fids_arr};
char s_fids[S_FIDS][5] = {s_fids_arr};
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr};

char *testfile = "test\\testfile.mp3";
char *testfile_bk = "test\\testfile.mp3.bk";
ID3V2_HEADER testfile_header;
ID3_METAINFO testfile_info;

void cprintf(const char *color_code, const char *fmt, ...);
int file_copy(const char *src, const char *dst);
char *get_cmd_str(char *testfile, const char args[E_FIDS][256]);

int verify(char *testfile, char args[E_FIDS][256], char frames_edited[E_FIDS]) {
	FILE *tf = fopen(testfile, "rb");
	read_header(&testfile_header, tf, testfile, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, tf, 0);

	int frames = testfile_info.frame_count;

    char *data;

    fseek(tf, testfile_info.frame_pos, SEEK_SET);
    
    for (int i = 0; i < frames; i++) {
        ID3V2_FRAME_HEADER frame_header;
        read_frame_header(&frame_header, tf);

        int readonly = 0;
        parse_frame_header_flags(frame_header.flags, &readonly, tf);

        int frame_data_sz = synchsafeint32ToInt(frame_header.size);
        data = malloc(frame_data_sz+1);
        data[frame_data_sz] = '\0';
        
        if (!fread(data, frame_data_sz, 1, tf)){
            printf("Error occurred reading frame data.\n");
            return 1;
        }

		// TODO: Verify integrity of all data, not just edited data

		// Verify frame data of edits is valid and correct
		int id = get_index(fids, E_FIDS, frame_header.fid);
		if (readonly && frames_edited[id] == 1) return 1;
		if (frames_edited[id] == 1) { // Edited frame
			char *data_arg = get_frame_data(frame_header.fid, args[id]);
			int data_arg_sz = sizeof_frame_data(frame_header.fid, args[id]);
			
			if (frame_data_sz != data_arg_sz) {
				cprintf(FAIL, "Failed Size diff %.4s: \n\tfdz: %d | das: %d\n", frame_header.fid, frame_data_sz, data_arg_sz);
				return 1;
			}

			for (int j = 0; j < frame_data_sz; j++) {
				if (data[j] != data_arg[j]) {
					cprintf(FAIL, "Data is not equal %.4s.\033[0m\n", frame_header.fid);
					return 1;
				}
			}
		} 

        free(data);
    }

	return 0;
}

int test(char *testfile, char args[E_FIDS][256], char frames_edited[E_FIDS]) {
	// Reset testing file
	file_copy(testfile_bk, testfile); 

	char *cmd = get_cmd_str(testfile, args);
	
	int fail = system(cmd);
	if (fail != 0) return fail;

	fail = verify(testfile, args, frames_edited);

	printf("\tTest ");
	if (!fail) cprintf(PASS, "PASS");
	else cprintf(FAIL, "FAIL");
	cprintf(BLUE, ": %s\n", cmd + strlen(exec_path) + 1);

	return fail;
}

int main(int argc, char *argv[]) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	char frames_edited[E_FIDS];
	memset(frames_edited, 0, sizeof(char)*E_FIDS);

	FILE *tf = fopen(testfile, "rb");
	read_header(&testfile_header, tf, testfile, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, tf, 0);
	fclose(tf);

	if (file_copy(testfile, testfile_bk)) {
		printf("Error reading test file.\n");
		exit(1);
	}

	int total_tests = 0, total_fails = 0;

	{ // Single File Unit Tests
		int tests = 0, fails = 0;
		cprintf(PURP, "Single File Unit Tests:\n");

		{ // TPE1: Artist
			strncpy(args[0], "Alhumam J.", 256);
			frames_edited[0] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(char)*E_FIDS);
		}
		
		{ // TALB: Album
			strncpy(args[1], "My Album", 256);
			frames_edited[1] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(char)*E_FIDS);
		}
		
		{ // TIT2: Title
			strncpy(args[2], "My Title", 256);
			frames_edited[2] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(char)*E_FIDS);
		}

		{ // TRCK: Track Number
			strncpy(args[3], "1", 2);
			frames_edited[3] = 1;
			char *trck_testfile = "test\\1 testfile.mp3";
			fails += test(trck_testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(char)*E_FIDS);
		}

		tests += 4;
		cprintf(PASS, "\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\tFails: %d\n", fails);

		total_tests += tests;
		total_fails += fails;
	}

	{ // All Arguments Combined Single File Test
		int tests = 0, fails = 0;
		cprintf(PURP, "All Arguments Combined Single File Test:\n");

		{
			strncpy(args[0], "Alhumam J.", 256);
			strncpy(args[2], "My Title", 256);
			strncpy(args[1], "My Album", 256);
			strncpy(args[3], "1", 2);
			frames_edited[0] = 1;
			frames_edited[1] = 1;
			frames_edited[2] = 1;
			frames_edited[3] = 1;
			char *trck_testfile = "test\\1 testfile.mp3";
			fails += test(trck_testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(char)*E_FIDS);
		}

		tests += 1;
		cprintf(PASS, "\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\tFails: %d\n", fails);

		total_tests += tests;
		total_fails += fails;
	}

	printf("\n");
	printf("Total Tests: %d\n", total_tests);
	cprintf(PASS, "Total Passes: %d\n", total_tests - total_fails);
	cprintf(FAIL, "Total Fails: %d\n", total_fails);
	
    return 0;
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

char *get_cmd_str(char *testfile, const char args[E_FIDS][256]) {
	char *cmd = calloc((E_FIDS+1)*256, sizeof(char));

	strncat(cmd, exec_path, strlen(exec_path) + 1);
	strncat(cmd, " ", 2);
	
	char opts[E_FIDS][5] = {"-a ", "-b ", "-t ", "-n ", "-p "};
	for (int i = 0; i < E_FIDS; i++) {
		if (args[i][0] == '\0') continue;

		strncat(cmd, opts[i], 4);

		if (strncmp(opts[i], "-n ", 3) == 0) continue;

		strncat(cmd, "\"", 2);
		strncat(cmd, args[i], strlen(args[i])+1);
		strncat(cmd, "\" ", 3);
	}

	strncat(cmd, "\"", 2);
	strncat(cmd, testfile, strlen(testfile));
	strncat(cmd, "\"", 3);

	return cmd;
}

void cprintf(const char *color, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf(color);
	vprintf(fmt, args);
	printf(RESET);
	va_end(args);
}
