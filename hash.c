#include <stdio.h>

int buckets = 5;

// djb2 hash function
int djb2(char *str) {
    unsigned long hash = 5381;
    int i = 0;
    char c;

    while ((c = str[i++])) hash = ((hash << 5) + hash) + c;

    return hash;
}

int main() {
	char *keys[] = {"TPE1", "TALB", "TIT2", "TRCK", "APIC"};
	for (int i = 0; i < 5; i++) {
		int h = djb2(keys[i]);
		printf("%s: %d (%d) [", keys[i], h % buckets, h);
		for (int j = 0; j < 4; j++) printf(" 0x%x", keys[i][j]);
		printf(" ]\n");
	}
	printf("\n");
	for (int i = 0; i < 5; i++) {
		printf("%s: ", keys[i]);
		printf("%d \n", (keys[i][2] + keys[i][3] - ((keys[i][2] & 0x2) >> 1)) % buckets);
	}
	printf("\n");
	for (int i = 0; i < 5; i++) {
		printf("%s: ", keys[i]);
		printf("%d \n", (((keys[i][2] & 0b00000010) << 1) + 
						 ((keys[i][2] & 0b00001000) >> 2) + 
						 (keys[i][1] & 0b00000001))
						  % buckets);
	}

    return 0;
}