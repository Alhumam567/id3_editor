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

typedef struct test_data {
	char (*fid)[4];
	int frame_count;
	char **data;
	int *sz;
} TEST_DATA;

void cprintf(const char *color_code, const char *fmt, ...);
int file_copy(const char *src, const char *dst);
char *get_cmd_str(char *testfile, const char args[E_FIDS][256]);
int test(char *testfile_path, char args[E_FIDS][256], int frames_edited[E_FIDS]);
int assert(const TEST_DATA *expected_data, const TEST_DATA *real_data);
void read_arg_data(TEST_DATA *expected, const ID3_METAINFO metainfo, const char args[E_FIDS][256], const int frames_edited[E_FIDS]);

int main(int argc, char *argv[]) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	int frames_edited[E_FIDS];
	memset(frames_edited, 0, sizeof(int)*E_FIDS);

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
			strncpy(args[0], "New Author", 256);
			frames_edited[0] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(int)*E_FIDS);
		}
		
		{ // TALB: Album
			strncpy(args[1], "My Album", 256);
			frames_edited[1] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(int)*E_FIDS);
		}
		
		{ // TIT2: Title
			strncpy(args[2], "My Title", 256);
			frames_edited[2] = 1;
			fails += test(testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(int)*E_FIDS);
		}

		{ // TRCK: Track Number
			strncpy(args[3], "1", 2);
			frames_edited[3] = 1;
			char *trck_testfile = "test\\1 testfile.mp3";
			fails += test(trck_testfile, args, frames_edited);
			memset(args, 0, E_FIDS*256);
			memset(frames_edited, 0, sizeof(int)*E_FIDS);
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
			memset(frames_edited, 0, sizeof(int)*E_FIDS);
		}

		tests += 1;
		cprintf(PASS, "\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\tFails: %d\n", fails);

		total_tests += tests;
		total_fails += fails;
	}

	// TODO Test Cases:
	// 1. Multiple file tests
	// 		- Single argument tests
	// 		- Multi  argument tests
	// 2. Overwriting vs appending new frames on single file 
	// 3. Variable initial frames on multiple files

	printf("\n");
	printf("Total Tests: %d\n", total_tests);
	cprintf(PASS, "Total Passes: %d\n", total_tests - total_fails);
	cprintf(FAIL, "Total Fails: %d\n", total_fails);
	
    return 0;
}

int assert(const TEST_DATA *expected, const TEST_DATA *real) {
	if (expected->frame_count != real->frame_count) return 1;
	
	for (int i = 0; i < expected->frame_count; i++) {
		// TODO: readonly checks
		if (expected->sz[i] != real->sz[i] || 
			memcmp(expected->data[i], real->data[i], real->sz[i])) 
			return 1;
	}

	return 0;
}

int test(char *testfile_path, char args[E_FIDS][256], int frames_edited[E_FIDS]) {
	// Reset testing file
	FILE *f = fopen(testfile_path, "rb");
	file_copy(testfile_bk, testfile_path); 

	char *cmd = get_cmd_str(testfile_path, args);

	read_header(&testfile_header, f, testfile_path, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, f, 0);

	TEST_DATA expected;
	expected.data = calloc(testfile_info.frame_count, sizeof(char *));
    expected.sz = calloc(testfile_info.frame_count, sizeof(int));
	expected.fid = calloc(testfile_info.frame_count, sizeof(char [4]));
	expected.frame_count = testfile_info.frame_count;
	read_data(testfile_info, expected.data, expected.sz, f);
	memcpy(expected.fid, testfile_info.fids, testfile_info.frame_count * 4);
	read_arg_data(&expected, testfile_info, args, frames_edited);
	fclose(f);

	
	f = fopen(testfile_path, "rb");
	int fail = system(cmd);
	if (fail != 0) return fail;

	read_header(&testfile_header, f, testfile_path, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, f, 0);

	TEST_DATA real;
	real.data = calloc(testfile_info.frame_count, sizeof(char *));
    real.sz = calloc(testfile_info.frame_count, sizeof(int));
	real.fid = calloc(testfile_info.frame_count, sizeof(char [4]));
	real.frame_count = testfile_info.frame_count;
	read_data(testfile_info, real.data, real.sz, f);
	memcpy(real.fid, testfile_info.fids, testfile_info.frame_count * 4);
	real.frame_count = testfile_info.frame_count;

	fail = assert(&expected, &real);

	printf("\tTest ");
	if (!fail) cprintf(PASS, "PASS");
	else cprintf(FAIL, "FAIL");
	cprintf(BLUE, ": %s\n", cmd + strlen(exec_path) + 1);

	return fail;
}

void read_arg_data(TEST_DATA *expected, ID3_METAINFO metainfo, const char args[E_FIDS][256], const int frames_edited[E_FIDS]) {
	int frames_edited_cp[E_FIDS];
	memcpy(frames_edited_cp, frames_edited, E_FIDS * sizeof(int));

	for (int i = 0; i < expected->frame_count; i ++) {
		int id = get_index(fids, E_FIDS, expected->fid[i]);

		if (id != -1 && frames_edited_cp[id] == 1) {
			expected->sz[i] = sizeof_frame_data(expected->fid[i], args[id]);
			free(expected->data[i]);
			expected->data[i] = calloc(expected->sz[i], sizeof(char));
            memcpy(expected->data[i], get_frame_data(expected->fid[i], args[id]), expected->sz[i]);

			frames_edited_cp[id] = 0;
		}
	}

	int additional_frames = 0;
	for (int i = 0; i < E_FIDS; i ++) if (frames_edited_cp[i]) additional_frames++;

	if (additional_frames > 0) {
		int total_frame_count = expected->frame_count + additional_frames;

		char (*_fid)[4] = calloc(total_frame_count, 4);
		char **_data = calloc(total_frame_count, sizeof(char *));
		int *_sz = calloc(total_frame_count, sizeof(int));

		memcpy(_fid, expected->fid, expected->frame_count * 4);
		for (int i = 0; i < expected->frame_count; i ++) {
			_data[i] = calloc(expected->sz[i], sizeof(char));
			_sz[i] = expected->sz[i];
			memcpy(_data[i], expected->data[i], expected->sz[i]);
		}	
		
		free(expected->data);
		free(expected->sz);
		free(expected->fid);

		int ind = expected->frame_count;
			
		for (int i = 0; i < E_FIDS; i ++) { 
			if (frames_edited_cp[i]) {
				_sz[ind] = sizeof_frame_data(get_fid(i), args[i]);
				_data[ind] = calloc(_sz[ind], sizeof(char));
            	memcpy(_data[ind], get_frame_data(get_fid(i), args[i]), _sz[ind]);

				ind++;
				frames_edited_cp[i] = 0;
			}
		}

		expected->data = _data;
		expected->sz = _sz;
		expected->fid = _fid;
		expected->frame_count = total_frame_count;
	}
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
