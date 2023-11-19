#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id3.h"
#include "id3_parse.h"

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

char t_fids[T_FIDS][5] = {t_fids_arr};
char s_fids[S_FIDS][5] = {s_fids_arr};
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr};

char *testfile = "test\\testfile.mp3";
char *testfile_bk = "test\\testfile.mp3.bk";
ID3V2_HEADER testfile_header;
ID3_METAINFO testfile_info;

int file_copy(const char *src, const char *dst);

void verify(char args[E_FIDS][256]) {
	
}

void test(char args[E_FIDS][256]) {
	file_copy(testfile_bk, testfile); // Reset testing file

	system("echo Test");
}

int main(int argc, char *argv[]) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	FILE *tf = fopen(testfile, "rb");
	read_header(&testfile_header, tf, testfile, 1);
	get_ID3_metainfo(&testfile_info, &testfile_header, tf, 1);

	if (file_copy(testfile, testfile_bk)) {
		printf("Error reading test file.\n");
		exit(1);
	}

	test(args);

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