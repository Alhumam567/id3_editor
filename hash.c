#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int n = 5;
int buckets = 5;
int p = 65805703;

// djb2 hash function
int djb2(char *str) {
    unsigned long hash = 5381;
    int i = 0;
    char c;

    while ((c = str[i++])) hash = ((hash << 5) + hash) + c;

    return hash;
}

unsigned int linear_hash(const unsigned int a, const unsigned int b, const unsigned int k) {
	return ((a*k + b) % p) % buckets;
}

unsigned int strtoint(const char k[5]) {
	return (k[3] * pow(36,3)) + (k[2] * pow(36,2)) + (k[1] * 36) + k[0];
}

int main() {
	char *keys[] = {"TPE1", "TALB", "TIT2", "TRCK", "APIC"};
	int *ikeys = calloc(n, sizeof(int));
	for (int i = 0; i < n; i++) {
		ikeys[i] = strtoint(keys[i]);
		printf("%d \n", ikeys[i]);
	}

	printf("buckets: %d\n", buckets);
	printf("djb2 hash:\n");
	for (int i = 0; i < 5; i++) {
		int h = djb2(keys[i]);
		printf("%s: %d (%d) [", keys[i], h % buckets, h);
		for (int j = 0; j < 4; j++) printf(" 0x%x", keys[i][j]);
		printf(" ]\n");
	}
	printf("\nspecialized hash 1:\n");
	for (int i = 0; i < 5; i++) {
		printf("%s: ", keys[i]);
		printf("%d \n", (keys[i][2] + keys[i][3] - ((keys[i][2] & 0x2) >> 1)) % buckets);
	}
	printf("\nspecialized hash 2:\n");
	for (int i = 0; i < 5; i++) {
		printf("%s: ", keys[i]);
		printf("%d \n", (((keys[i][2] & 0b00000010) << 1) + 
						 ((keys[i][2] & 0b00001000) >> 2) + 
						 (keys[i][1] & 0b00000001))
						  % buckets);
	}
	/* Universal hashing
	* m = n = # of buckets = 5
    * p = 65805703

    * Z_p = {0,1,2,3,4,5,6}
    * Z_p^* = {1,2,3,4,5,6}

    * h_{a,b}(k) = ((a*k + b) mod p) mod m

    * hash family:
    * H_{p,m} = {h_{a,b} : a in Z_p^* and b in Z_p}
	*/
	// for (int a=1; a < p; a++) {
	// 	for (int b=0; b<p; b++) {
	// 		printf("hash func %d,%d: ", a, b);
	// 		int *isset = calloc(buckets, sizeof(int));
	// 		for (int k = 0; k<buckets; k++) {
	// 			isset[linear_hash(a, b, strtoint(keys[k]))]++;
	// 		}
	// 		for (int _i = 0; _i < buckets; _i++) printf("%d ", isset[_i]);
	// 		free(isset);
	// 		printf("\n");
	// 	}
	// }
	srand(time(NULL));
	int a = (rand() % (p - 1)) + 1;
	int b = (rand() % (p));
	printf("\nrandom universal hashing (p=%d, a=%d, b=%d): \n", p, a, b);
	int no_col = 1;
	int *isset = calloc(buckets, sizeof(int));
 	for (int i = 0; i < n; i++) {
		int h = linear_hash(a, b, ikeys[i]);
		if (isset[h]) no_col = 0;
		else isset[h]=1;
		printf("%s: %d\n", keys[i], h);
	}
	printf("is perfect: %d\n", no_col);

	// e.g. min perfect hash function for keys (p=65805703, a=46856658, b=669512)

    return 0;
}