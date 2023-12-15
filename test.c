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
#define WHITE_BOLD "\033[1m"
#define RESET "\033[0m"

char t_fids[T_FIDS][5] = {t_fids_arr};
char s_fids[S_FIDS][5] = {s_fids_arr};
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr};

#define NUM_FILES 1
char *testfiles[] = { 
	// "1.mp3", // Artist
	// "2.mp3", // Album 
	// "4.mp3", // Title
	// "8.mp3", // Track
	"16.mp3",// All frames present
};
char *testfile_bk;
char *testfolder_path = "test/";
ID3V2_HEADER testfile_header;
ID3_METAINFO testfile_info;

typedef struct test_data {
	char (*fid)[4]; // frame IDs
	int frame_count; // number of frames
	char **data; // frame data
	int *sz; // frame data sizes
} TEST_DATA;

void cprintf(const char *color_code, const char *fmt, ...);
int file_copy(const char *src, const char *dst);
char *get_cmd_str(char *testfile, const char args[E_FIDS][256]);
void read_arg_data(TEST_DATA *expected, const ID3_METAINFO metainfo, const char args[E_FIDS][256], const int frames_edited[E_FIDS]);
void get_test_data(TEST_DATA *tdata, char *testfile_path);
void free_test_data(TEST_DATA *tdata);

int single_arg_test(char *testfile_path, int index, char *arg);
int var_arg_test(char *testfile_path, int n, ...);
int run_test(char *testfile_path, char args[E_FIDS][256], int frames_edited[E_FIDS]);
int assert(const TEST_DATA *expected_data, const TEST_DATA *real_data);

int main() {
	int total_tests = 0, total_fails = 0;

	// Single File Unit Tests
	for (int i = 0; i < NUM_FILES; i++) { 
		int tests = 0, fails = 0;

		cprintf(YELLOW, "Test File: %s:\n", testfiles[i]);
		cprintf(PURP, "\tSingle Argument Single File Tests:\n ");

		char *filepath = malloc(strlen(testfolder_path) + strlen(testfiles[i]) + 1);
		char testfile_cp[128];
		strncpy(filepath, testfolder_path, strlen(testfolder_path));
		strncpy(filepath + strlen(testfolder_path), testfiles[i], strlen(testfiles[i]));
		strncpy(testfile_cp, testfiles[i], strlen(testfiles[i]));

		testfile_bk = malloc(strlen(filepath) + 3);
		strncpy(testfile_bk, filepath, strlen(filepath) + 1);
		strncpy(testfile_bk + strlen(filepath), ".bk", 4);
		if (file_copy(filepath, testfile_bk)) {
			printf("Error reading test file.\n");
			exit(1);
		}

		fails += single_arg_test(filepath, 0, "TEST AUTHOR NAME"); // TPE1: Artist		
		fails += single_arg_test(filepath, 1, "TEST ALBUM NAME"); // TALB: Album
		fails += single_arg_test(filepath, 2, "TEST SONG TITLE"); // TIT2: Title
		fails += single_arg_test(filepath, 3, strtok(testfile_cp, ".")); // TRCK: Track Number

		tests += 4;
		cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\t\tFails: %d\n", fails);

		total_tests += tests;
		total_fails += fails;

		// All Arguments Combined Single File Test
		tests = 0;
		fails = 0;
		cprintf(PURP, "\tAll Arguments Combined Single File Test:\n ");

		char *trck_num = calloc(5 + strlen(testfile_cp) + 1,sizeof(char));
		strncpy(trck_num, "TRCK>", 6);
		strncpy(trck_num + strlen("TRCK>"), strtok(testfile_cp, "."), strlen(testfile_cp));
		fails += var_arg_test(filepath, 4, "TPE1>TEST AUTHOR NAME", "TALB>TEST ALBUM NAME", "TIT2>TEST SONG TITLE", trck_num);

		tests += 1;
		cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\t\tFails: %d\n", fails);

		total_tests += tests;
		total_fails += fails;
		
		remove(testfile_bk);

		free(testfile_bk);
		free(filepath);
	}

	// TODO Test Cases:
	// 1. Multiple file tests
	// 		- Single argument tests
	// 		- Multi  argument tests
	// 2. Overwriting vs appending new frames on single file 
	// 3. Variable initial frames on multiple files
	// 4. Multithreaded testing

	printf("\n");
	cprintf(WHITE_BOLD, "Results\n");
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

int var_arg_test(char *testfile_path, int n, ...) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	int frames_edited[E_FIDS];
	memset(frames_edited, 0, sizeof(int)*E_FIDS);

	va_list nargs;
	va_start(nargs, n);

	for (int i = 0; i < n; i++) {
		const char *varg = va_arg(nargs, const char *);
		char *arg = malloc(strlen(varg) + 1); 
		strncpy(arg, varg, strlen(varg) + 1);

		int ind = get_index(fids, E_FIDS, strtok(arg, ">"));
		strncpy(args[ind], strtok(NULL, ">"), 256);
		frames_edited[ind] = 1;
	}

	va_end(nargs);
	return run_test(testfile_path, args, frames_edited);
}

int single_arg_test(char *testfile_path, int index, char *arg) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	int frames_edited[E_FIDS];
	memset(frames_edited, 0, sizeof(int)*E_FIDS);

	strncpy(args[index], arg, 256);
	frames_edited[index] = 1;

	return run_test(testfile_path, args, frames_edited);
}

int run_test(char *testfile_path, char args[E_FIDS][256], int frames_edited[E_FIDS]) {
	// Reset testing file
	if (file_copy(testfile_bk, testfile_path) == -1) {
		printf("Failed copy file.\n");
		exit(1);
	} 

	char *cmd = get_cmd_str(testfile_path, args);

	TEST_DATA expected;
	get_test_data(&expected, testfile_path);
	read_arg_data(&expected, testfile_info, args, frames_edited);
	
	int fail = 0;
	fail = system(cmd);
	free(cmd);
	if (fail != 0) return fail;

	TEST_DATA real;
	get_test_data(&real, testfile_path);

	fail = assert(&expected, &real);

	free_test_data(&real);
	free_test_data(&expected);

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

void get_test_data(TEST_DATA *tdata, char *testfile_path) {
	FILE *f = fopen(testfile_path, "rb");
	read_header(&testfile_header, f, testfile_path, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, f, 0);

	tdata->data = calloc(testfile_info.frame_count, sizeof(char *));
    tdata->sz = calloc(testfile_info.frame_count, sizeof(int));
	tdata->fid = calloc(testfile_info.frame_count, sizeof(char [4]));
	tdata->frame_count = testfile_info.frame_count;
	read_data(testfile_info, tdata->data, tdata->sz, f);
	memcpy(tdata->fid, testfile_info.fids, testfile_info.frame_count * 4);
	
	fclose(f);
}

void free_test_data(TEST_DATA *tdata) {
	free(tdata->fid);
	free(tdata->sz);

	for (int i = 0; i < tdata->frame_count; i++) free(tdata->data[i]);
	free(tdata->data);
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
	strncat(cmd, "\" ", 3);

	return cmd;
}

void cprintf(const char *color, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("%s", color);
	vprintf(fmt, args);
	printf("%s", RESET);
	va_end(args);
}
