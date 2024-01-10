#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "id3.h"
#include "id3_parse.h"
#include "util.h"
#include "path.h"
#include "hashtable.h"

/* TODO:
	1. Modify backup files instead of originals
	2. Fill out test files
		- "-p" tests
		- Missing: 1-3, 8-14, 15-31
*/

char t_fids[T_FIDS][5] = {t_fids_arr};
char s_fids[S_FIDS][5] = {s_fids_arr};
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr};

#define NUM_FILES 5
char *testfiles[] = { 
	// "1.mp3", // Artist
	// "2.mp3", // Album 
	"4.mp3", // Title
	"5.mp3", // Title, Artist
	"6.mp3", // Title, Album
	"7.mp3", // Artist, Album, Title
	// "8.mp3", // Track
	"15.mp3",// All frames present
};
char *testfile_bk;
ID3V2_HEADER testfile_header;
ID3_METAINFO testfile_info;

typedef struct test_data {
	DIRECT_HT *data;
	DIRECT_HT *data_sz;
} TEST_DATA;

char *build_cmd_str(const char *testfile, const DIRECT_HT *args);
void read_arg_data(TEST_DATA *expected, const ID3_METAINFO metainfo, const DIRECT_HT *args);
void get_file_data(TEST_DATA *tdata, const char *testfile_path);
void free_test_data(TEST_DATA *tdata);

int single_arg_test(const char *testfile_path, const char key[4], char *arg);
int var_arg_test(const char *testfile_path, const int n, ...);
int run_test(const char *testfile_path, const DIRECT_HT *args);
int assert(const TEST_DATA *expected_data, const TEST_DATA *real_data);

int main() {
	int total_tests = 0, total_fails = 0;

	// Single File Unit Tests
	for (int i = 0; i < NUM_FILES; i++) { 
		int tests = 0, fails = 0;

		cprintf(YELLOW, "Test File: %s\n", testfiles[i]);
		cprintf(PURP, "\tSingle Argument Single File Tests:\n ");

		char *filepath = calloc(strlen(testfolder_path) + strlen(testfiles[i]) + 1, sizeof(char));
		char testfile_cp[128] = { 0 };
		strncpy(filepath, testfolder_path, strlen(testfolder_path));
		strncpy(filepath + strlen(testfolder_path), testfiles[i], strlen(testfiles[i]));
		strncpy(testfile_cp, testfiles[i], strlen(testfiles[i]));
		testfile_bk = calloc(strlen(filepath) + 4, sizeof(char));
		strncpy(testfile_bk, filepath, strlen(filepath) + 1);
		strncpy(testfile_bk + strlen(filepath), ".bk", 4);
		if (file_copy(filepath, testfile_bk)) {
			printf("Error reading test file.\n");
			exit(1);
		}

		fails += single_arg_test(filepath, "TPE1", "TEST AUTHOR NAME"); // TPE1: Artist		
		fails += single_arg_test(filepath, "TALB", "TEST ALBUM NAME"); // TALB: Album
		fails += single_arg_test(filepath, "TIT2", "TEST SONG TITLE"); // TIT2: Title
		fails += single_arg_test(filepath, "TRCK", strtok(testfile_cp, ".")); // TRCK: Track Number

		tests += 4;
		cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\t\tFails: %d\n", fails);
		total_tests += tests;
		total_fails += fails;

		// All Arguments Combined Single File Test
		tests = 0;
		fails = 0;
		cprintf(PURP, "\tAll Arguments Combined Single File Test:\n ");

		char *trck_num = calloc(5 + strlen(testfile_cp) + 1, sizeof(char));
		strncpy(trck_num, "TRCK>", 6);
		strncat(trck_num, testfile_cp, strlen(testfile_cp));

		fails += var_arg_test(filepath, 4, "TPE1>TEST AUTHOR NAME", "TALB>TEST ALBUM NAME", "TIT2>TEST SONG TITLE", trck_num);

		tests += 1;
		cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\t\tFails: %d\n", fails);
		total_tests += tests;
		total_fails += fails;

		remove(testfile_bk);
		free(testfile_bk);
		free(filepath);
		free(trck_num);
	}

	cprintf(WHITE_BOLD, "\nResults\n");
	printf("Total Tests: %d\n", total_tests);
	cprintf(PASS, "Total Passes: %d\n", total_tests - total_fails);
	cprintf(FAIL, "Total Fails: %d\n", total_fails);
	
    return 0;
}

int assert(const TEST_DATA *expected, const TEST_DATA *real) {
	if (expected->data->sz != real->data->sz) return 1; // identical number of frames

	DIRECT_HT *exp_data = expected->data, *exp_sizes = expected->data_sz;
	DIRECT_HT *real_data = real->data, *real_sizes = real->data_sz; 

	for (int i = 0; i < expected->data->sz; i++) {
		// TODO: readonly checks
		if (exp_sizes->entries[i] != NULL && 
			(exp_sizes->entries[i]->val != real_sizes->entries[i]->val || 
			 memcmp(exp_data->entries[i]->val, real_data->entries[i]->val, *((int *) real_sizes->entries[i]->val))))
			return 1;
	}

	return 0;
}

int var_arg_test(const char *testfile_path, int n, ...) {
	DIRECT_HT *args = direct_address_create(E_FIDS, &e_fids_hash);
	va_list nargs;
	va_start(nargs, n);

	for (int i = 0; i < n; i++) {
		const char *varg = va_arg(nargs, const char *);
		char *arg = calloc(strlen(varg) + 1, sizeof(char)); 
		strncpy(arg, varg, strlen(varg) + 1);
		int arg_len = strlen(arg) - 5;

		char fid[4] = { 0 };
		strncpy(fid, strtok(arg, ">"), 4);
		char *fid_arg = calloc(arg_len + 1, sizeof(char));
		strncpy(fid_arg, strtok(NULL, ">"), arg_len + 1);

		direct_address_insert(args, fid, fid_arg);
		free(arg);
	}
	va_end(nargs);

	int c = run_test(testfile_path, args);
	free(args);
	return c;
}

int single_arg_test(const char *testfile_path, const char key[4], char *arg) {
	DIRECT_HT *args = direct_address_create(E_FIDS, e_fids_hash);

	direct_address_insert(args, key, arg);

	int c = run_test(testfile_path, args);
	free(args);
	return c;
}

int run_test(const char *testfile_path, const DIRECT_HT *args) {
	// Reset testing file
	if (file_copy(testfile_bk, testfile_path) == -1) {
		printf("Failed copy file.\n");
		exit(1);
	} 

	char *cmd = build_cmd_str(testfile_path, args);

	TEST_DATA expected;
	get_file_data(&expected, testfile_path);
	read_arg_data(&expected, testfile_info, args);
	
	int fail = system(cmd);
	if (fail != 0) return fail;

	TEST_DATA real;
	get_file_data(&real, testfile_path);

	fail = assert(&expected, &real);
	free_test_data(&real);
	free_test_data(&expected);

	printf("\tTest ");
	if (!fail) cprintf(PASS, "PASS");
	else cprintf(FAIL, "FAIL");
	cprintf(BLUE, ": %s\n", cmd + strlen(exec_path) + 1);
	free(cmd);

	return fail;
}

void read_arg_data(TEST_DATA *expected, const ID3_METAINFO metainfo, const DIRECT_HT *args) {
	DIRECT_HT *exp_data = expected->data;
	DIRECT_HT *exp_sz = expected->data_sz;
	
	for (int i = 0; i < args->buckets; i++) {
		if (args->entries[i] == NULL) continue;

		char key[4];
		strncpy(key, e_fids_reverse_lookup[i], 4);

		int *arg_sz = calloc(1, sizeof(int));
		*arg_sz = sizeof_frame_data(key, args->entries[i]->val);
		char *arg_data = calloc(*arg_sz, sizeof(char));
		strncpy(arg_data, get_frame_data(key, args->entries[i]->val), *arg_sz);
		direct_address_insert(exp_sz, key, arg_sz);
		direct_address_insert(exp_data, key, arg_data);
	}
}

void get_file_data(TEST_DATA *tdata, const char *testfile_path) {
	FILE *f = fopen(testfile_path, "rb");
	read_header(&testfile_header, f, testfile_path, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, f, 0);
	
	tdata->data = direct_address_create(MAX_HASH_VALUE, &all_fids_hash);
	tdata->data_sz = direct_address_create(MAX_HASH_VALUE, &all_fids_hash);
	read_data(testfile_info, tdata->data, tdata->data_sz, f);
	
	fclose(f);
}

void free_test_data(TEST_DATA *tdata) {
	direct_address_destroy(tdata->data);
	direct_address_destroy(tdata->data_sz);
}

char *build_cmd_str(const char *testfile, const DIRECT_HT *args) {
	char *cmd = calloc((E_FIDS+1)*256, sizeof(char));

	strncat(cmd, exec_path, strlen(exec_path) + 1);
	strncat(cmd, " ", 2);
	
	char opts[E_FIDS][5] = {"-b ", "-t ", "-p ", "-n ", "-a "}; // convert to direct_address_table
	for (int i = 0; i < E_FIDS; i++) {
		if (args->entries[i] == NULL) continue;

		strncat(cmd, opts[i], 4);

		if (strncmp(opts[i], "-n ", 3) == 0) continue;

		strncat(cmd, "\"", 2);
		strncat(cmd, args->entries[i]->val, strlen(args->entries[i]->val)+1);
		strncat(cmd, "\" ", 3);
	}

	strncat(cmd, "\"", 2);
	strncat(cmd, testfile, strlen(testfile));
	strncat(cmd, "\" ", 3);

	return cmd;
}

