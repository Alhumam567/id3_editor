/**
 * Author: Alhumam Jabakhanji
 * Date of Creation: 2023-12-26
 * 
 * Automated tester for id3_editor
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "id3.h"
#include "id3_parse.h"
#include "util.h"
#include "file_util.h"
#include "path.h"
#include "hashtable.h"

char t_fids[T_FIDS][5] = {t_fids_arr};
char s_fids[S_FIDS][5] = {s_fids_arr};
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr};

#define NUM_FILES 8
char *testfiles[] = { 
	"1.mp3", // Artist
	"2.mp3", // Album 
	"4.mp3", // Title
	"5.mp3", // Title, Artist
	"6.mp3", // Title, Album
	"7.mp3", // Artist, Album, Title
	"8.mp3", // Track
	"15.mp3",// All frames present
};
#define SF_NUM_FILES 3
char *subfolder_testfiles[] = {
	"1.mp3", 
	"2.mp3",
	"4.mp3",
};
char *testfile_bk;

typedef struct test_data {
	DIRECT_HT *data;
	DIRECT_HT *data_sz;
} TEST_DATA;

char *build_cmd_str(const char *testfile, const DIRECT_HT *args);
void read_arg_data(TEST_DATA *expected, const DIRECT_HT *args);
void get_file_data(TEST_DATA *tdata, const char *testfile_path);
void free_test_data(TEST_DATA *tdata);

int single_arg_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, const char key[4], char *arg);
int var_arg_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, const int n, ...);
char *setup_file(const char *filename, char **file_backup, const int dir);
int run_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, const DIRECT_HT *args);
void clean_file(char *filepath, char *filepath_backup);
int assert(const TEST_DATA *expected_data, const TEST_DATA *real_data);

int main() {
	char *apic = calloc(5 + strlen(test_image_path) + 1, sizeof(char));
	strncpy(apic, "APIC>", 6);
	strncat(apic, test_image_path, strlen(test_image_path));

	int total_tests = 0, total_fails = 0;

	/*
	 * Single File Tests
	 * - Single argument unit tests
	 * - All arguments combined test
	*/ 
	for (int i = 0; i < NUM_FILES; i++) { 
		int tests = 0, fails = 0;

		cprintf(YELLOW, "Test File: %s\n", testfiles[i]);
		cprintf(PURP, "\tSingle Argument Tests:\n");

		char *filepath = setup_file(testfiles[i], &testfile_bk, 0);

		fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "TPE1", "TEST AUTHOR NAME"); // TPE1: Artist		
		fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "TALB", "TEST ALBUM NAME"); // TALB: Album
		fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "TIT2", "TEST SONG TITLE"); // TIT2: Title
		fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "TRCK", "1"); // TRCK: Track Number
		fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "APIC", test_image_path); // APIC: Attached picture
		tests += 5;

		// All Arguments Combined Single File Test
		cprintf(PURP, "\tAll Arguments Test:\n ");

		fails += var_arg_test(filepath, &filepath, &testfile_bk, 1, 5, "TPE1>TEST AUTHOR NAME", "TALB>TEST ALBUM NAME", "TIT2>TEST SONG TITLE", "TRCK>1", apic);
		tests += 1;

		cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
		cprintf(FAIL, "\t\tFails: %d\n", fails);
		total_tests += tests;
		total_fails += fails;

		clean_file(filepath, testfile_bk);
	}

	// Metadata overflow test using APIC 
	cprintf(YELLOW, "Metadata Overflow Test: %s\n", "4.mp3");
	char *filepath = setup_file("4.mp3", &testfile_bk, 0);
	total_fails += single_arg_test(filepath, &filepath, &testfile_bk, 1, "APIC", test_image_path); // APIC: Attached picture
	total_tests += 1;
	clean_file(filepath, testfile_bk);

	// All Arguments Directory test
	cprintf(YELLOW, "Directory Test: %s\n", subfolder_path);
	cprintf(PURP, "\tAll Arguments Test:\n ");
	int tests = 0, fails = 0;
	char **path = calloc(SF_NUM_FILES, sizeof(char *));
	char **path_bk = calloc(SF_NUM_FILES, sizeof(char *));
	for (int i = 0; i < SF_NUM_FILES; i++) path[i] = setup_file(subfolder_testfiles[i], &(path_bk[i]), 1);
	char *folderpath = calloc(strlen(subfolder_path) + strlen(testfolder_path) + 1, sizeof(char));
	strncpy(folderpath, testfolder_path, strlen(testfolder_path) + 1);
	strncat(folderpath, subfolder_path, strlen(subfolder_path) + 1);

	fails += var_arg_test(folderpath, path, path_bk, SF_NUM_FILES, 5, "TPE1>TEST AUTHOR NAME", "TALB>TEST ALBUM NAME", "TIT2>TEST SONG TITLE", "TRCK>1", apic);

	cprintf(PURP, "\tSingle Title Test:\n ");
	fails += single_arg_test(folderpath, path, path_bk, SF_NUM_FILES, "TIT2", "TEST SINGLE TITLE");

	cprintf(PURP, "\tMulti-title Test:\n ");
	char *multititles = "TEST1,TEST2,TEST3";
	fails += single_arg_test(folderpath, path, path_bk, SF_NUM_FILES, "TIT2", multititles);
	tests += 3;

	for (int i = 0; i < SF_NUM_FILES; i++) clean_file(path[i], path_bk[i]);
	free(folderpath);
	free(path);
	free(path_bk);

	cprintf(PASS, "\t\tPasses: %d\n", tests - fails);
	cprintf(FAIL, "\t\tFails: %d\n", fails);
	total_tests += tests;
	total_fails += fails;

	cprintf(WHITE_BOLD, "\nResults\n");
	printf("Total Tests: %d\n", total_tests);
	cprintf(PASS, "Total Passes: %d\n", total_tests - total_fails);
	cprintf(FAIL, "Total Fails: %d\n", total_fails);
	
	free(apic);
	
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

int var_arg_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, int n, ...) {
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

	int c = run_test(test_path, test_path_files, bk_path_files, num_files, args);
	direct_address_destroy(args);
	return c;
}

int single_arg_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, const char key[4], char *arg) {
	DIRECT_HT *args = direct_address_create(E_FIDS, e_fids_hash);

	char *val = calloc(strlen(arg), sizeof(int));
	strncpy(val, arg, strlen(arg)+1);
	direct_address_insert(args, key, val);
	int c = run_test(test_path, test_path_files, bk_path_files, num_files, args);
	direct_address_destroy(args);
	return c;
}

char *setup_file(const char *filename, char **file_backup, const int dir) {
	char *filepath = calloc(strlen(testfolder_path) + strlen(subfolder_path) + strlen(filename) + 1, sizeof(char));
	strncpy(filepath, testfolder_path, strlen(testfolder_path));
	if (dir) strncat(filepath, subfolder_path, strlen(subfolder_path) + 1);
	strncat(filepath + strlen(testfolder_path), filename, strlen(filename));
	
	*file_backup = calloc(strlen(filepath) + 4, sizeof(char));
	strncpy(*file_backup, filepath, strlen(filepath) + 1);
	strncat(*file_backup, ".bk", 4);
	if (file_copy(filepath, *file_backup)) {
		printf("Error reading test file.\n");
		exit(1);
	}
	return filepath;
}

int run_test(const char *test_path, char **test_path_files, char **bk_path_files, const int num_files, const DIRECT_HT *args) {
	char *cmd = build_cmd_str(test_path, args);

	TEST_DATA *expected = calloc(num_files, sizeof(TEST_DATA));
	for (int i = 0; i < num_files; i++) {	
		get_file_data(expected + i, test_path_files[i]);
		read_arg_data(expected + i, args);
	}
	
	int fail = system(cmd);
	if (fail != 0) return fail;

	TEST_DATA *real = calloc(num_files, sizeof(TEST_DATA));
	for (int i = 0; i < num_files; i++) get_file_data(real + i, test_path_files[i]);

	for (int i = 0; i < num_files; i++) {
		fail += assert(expected + i, real + i);
		free_test_data(expected + i);
		free_test_data(real + i);
	}

	for (int i = 0; i < num_files; i++) {
		if (file_copy(bk_path_files[i], test_path_files[i])) {
			printf("Failed copy file.\n");
			exit(1);
		} 
	}
	free(expected);
	free(real);

	printf("\tTest ");
	if (!fail) cprintf(PASS, "PASS");
	else cprintf(FAIL, "FAIL");
	cprintf(BLUE, ": %s\n", cmd + strlen(exec_path) + 1);
	free(cmd);

	return fail;
}

void clean_file(char *filepath, char *filepath_backup) {
	if (file_copy(filepath_backup, filepath)) {
		printf("Failed to copy file.\n");
		exit(1);
	} 
	remove(filepath_backup);
	free(filepath_backup);
	free(filepath);
}

void read_arg_data(TEST_DATA *expected, const DIRECT_HT *args) {
	DIRECT_HT *exp_data = expected->data;
	DIRECT_HT *exp_sz = expected->data_sz;
	
	for (int i = 0; i < args->buckets; i++) {
		if (args->entries[i] == NULL) continue;

		char key[4];
		strncpy(key, e_fids_reverse_lookup[i], 4);

		int *arg_sz = calloc(1, sizeof(int));
		*arg_sz = sizeof_frame_data(key, args->entries[i]->val);
		char *arg_data = get_frame_data(key, args->entries[i]->val);
		direct_address_insert(exp_sz, key, arg_sz);
		direct_address_insert(exp_data, key, arg_data);
	}
}

void get_file_data(TEST_DATA *tdata, const char *testfile_path) {
	ID3_METAINFO testfile_info;
	FILE *f = fopen(testfile_path, "rb");
	get_ID3_metainfo(&testfile_info, f, testfile_path, 0);
	
	tdata->data = direct_address_create(MAX_HASH_VALUE, &all_fids_hash);
	tdata->data_sz = direct_address_create(MAX_HASH_VALUE, &all_fids_hash);
	read_data(testfile_info, tdata->data, tdata->data_sz, f);
	
	fclose(f);
	direct_address_destroy(testfile_info.fid_sz);
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
	strncat(cmd, "\"", 2);

	return cmd;
}

