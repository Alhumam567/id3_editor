#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * @brief Reverse lookup of frame ID strs to index
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
