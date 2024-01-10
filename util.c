#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


/**
 * Synchsafe int32: Integer type represented in 28 bits instead of typical 32, where the most significant 
 * bits of each byte are always 0: 0XXXXXXX 0XXXXXXX 0XXXXXXX 0XXXXXXX
 */


/**
 * @brief Converts synchsafe int32 to int data type
 * 
 * @param c - Synchsafe int32
 * @return int - Equivalent integer as INT
 */
int synchsafeint32ToInt(char c[4]) {
    return (c[0] << 21) | ((c[1] << 14) | ((c[2] << 7) | (c[3] | (int)0)));
}

/**
 * @brief Converts int to synchsafe int32 
 * 
 * @param x - Int to convert to synchsafe int32
 * @param ssint - Equivalent synchsafe int32
 */
void intToSynchsafeint32(int x, char ssint[4]) {
    for (int i=0; i<4; i++) ssint[3-i] = (x & (0x7F << i*7)) >> i*7;
}

/**
 * @brief Concatenate two strings: s1 + s2
 * 
 * @param s1 - First String
 * @param s2 - Second string
 * @return char* - s1 + s2
 */
char *concatenate(char *s1, char *s2) {
    char *s3 = calloc(strlen(s1) + strlen(s2) + 1, 1);

    strncat(s3, s1, strlen(s1));
    strncat(s3, s2, strlen(s2));

    return s3;
}

/**
 * @brief Get the track number of a file 
 * 
 * @param filepath - Name of the file including a track number
 * @param prefix_len - Length of the directory prefix to filepath
 * @return int - Track number
 */
int get_trck(char *filepath, int prefix_len) {
    return atoi(filepath + prefix_len);
}

/**
 * @deprecated
 * @brief Lookup of frame ID str to index
 * 
 * @param fids - String array of FIDs
 * @param fids_len - Size of string array
 * @param fid - String to find
 * @return int - Index of string
 */
int get_index(char (*str)[5], int arr_len, char fid[4]) {
    for (int i = 0; i < arr_len; i++) {
        if (strncmp(str[i], fid, 4) == 0) return i;
    }

    return -1;
}

/**
 * @deprecated
 * @brief Reverse lookup from index to frame ID str
 * 
 * @param i - Index of str
 * @return char* - Frame ID, len 4
 */
char *get_fid(int i) {
    switch(i) {
        case 0: return "TPE1";
        case 1: return "TALB";
        case 2: return "TIT2";
        case 3: return "TRCK";
        case 4: return "APIC";
    }

    return "\0";
}

#define RESET "\033[0m"

/**
 * @brief ANSI escape sequences printf wrapper for editing color and font of terminal output
 * 
 * @param color - ANSI escape sequence string
 * @param fmt - printf formatting string
 * @param ... 
 */
void cprintf(const char *color, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("%s", color);
	vprintf(fmt, args);
	printf("%s", RESET);
	va_end(args);
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
