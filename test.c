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
char *get_cmd_str(char args[E_FIDS][256]);

void verify(char args[E_FIDS][256]) {
	
}

void test(char args[E_FIDS][256]) {
	file_copy(testfile_bk, testfile); // Reset testing file

	char *cmd = get_cmd_str(args);

	printf("%s\n", cmd);
}

int main(int argc, char *argv[]) {
	char args[E_FIDS][256];
	memset(args, 0, E_FIDS*256);

	FILE *tf = fopen(testfile, "rb");
	read_header(&testfile_header, tf, testfile, 0);
	get_ID3_metainfo(&testfile_info, &testfile_header, tf, 0);

	if (file_copy(testfile, testfile_bk)) {
		printf("Error reading test file.\n");
		exit(1);
	}

	strncpy(args[0], "Alhumam J.", strlen("Alhumam J.") + 1);
	strncpy(args[1], "My Album", strlen("My Album") + 1);
	test(args);
	memset(args, 0, E_FIDS*256);

	strncpy(args[2], "My Title", strlen("My Title") + 1);
	strncpy(args[3], "1", 1 + 1);
	test(args);
	memset(args, 0, E_FIDS*256);

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

char *get_cmd_str(char args[E_FIDS][256]) {
	char *cmd = calloc((E_FIDS+1)*256, sizeof(char));

	strncat(cmd, "./id3_editor.exe ", strlen("./id3_editor.exe ") + 1);
	
	char opts[E_FIDS][5] = {"-a ", "-b ", "-t ", "-n ", "-p "};
	for (int i = 0; i < E_FIDS; i++) {
		if (args[i][0] == '\0') continue;

		strncat(cmd, opts[i], 4);

		if (strncmp(opts[i], "-n ", 3) == 0) continue;

		strncat(cmd, "\"", 2);
		strncat(cmd, args[i], strlen(args[i])+1);
		strncat(cmd, "\" ", 3);
	}

	strncat(cmd, testfile, strlen(testfile));

	return cmd;
}