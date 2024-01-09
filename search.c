#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "id3_hash.h"

int p = 65805703;

// djb2 hash function
int djb2(const char *str, unsigned long hash) {
    int i = 0;
    char c;

    while ((c = str[i++])) hash = ((hash << 5) + hash) + c;

    return hash;
}

unsigned int linear_hash(const unsigned int a, const unsigned int b, const unsigned int k) {
	return ((a*k + b) % p);
}

unsigned int strtoint(const char k[5]) {
	return (k[3] * pow(36,3)) + (k[2] * pow(36,2)) + (k[1] * 36) + k[0];
}

unsigned int strtoint2(const char k[5]) {
	return (k[3] << 24) + (k[2] << 16) + (k[1] << 8) + k[0];
}

int main() {
	const int n_e = 5, n_all = 83;
	const char e_fids[5][5] = {"TPE1", "TALB", "TIT2", "TRCK", "APIC"};
	const char all_fids[83][5] = {
		"AENC", "APIC", "ASPI", "COMM", "COMR", "ENCR", "EQU2", "ETCO",
		"GEOB", "GRID", "LINK", "MCDI", "MLLT", "OWNE", "PRIV", "PCNT",
		"POPM", "POSS", "RBUF", "RVA2", "RVRB", "SEEK", "SIGN", "SYLT",
		"SYTC", "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDEN", "TDLY", 
		"TDOR", "TDRC", "TDRL", "TDTG", "TENC", "TEXT", "TFLT", "TIPL", 
		"TIT1", "TIT2", "TIT3", "TKEY", "TLAN", "TLEN", "TMCL", "TMED", 
		"TMOO", "TOAL", "TOFN", "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", 
		"TPE3", "TPE4", "TPOS", "TPRO", "TPUB", "TRCK", "TRSN", "TRSO", 
		"TSOA", "TSOP", "TSOT", "TSRC", "TSSE", "TSST", "TXXX", "UFID", 
		"USER", "USLT", "WCOM", "WCOP", "WOAF", "WOAR", "WOAS", "WORS", 
		"WPAY", "WPUB", "WXXX"
	};  
	int int_e_fids[n_e], int_all_fids[n_all];
	for (int i = 0; i < n_e; i++) int_e_fids[i] = strtoint(e_fids[i]);
	for (int i = 0; i < n_all; i++) int_all_fids[i] = strtoint2(all_fids[i]);

	printf("============Editable FIDs hash function search============\n\n");
	int buckets = n_e;

	for (int i = 0; i < n_e; i++) {
		printf("%s: [", e_fids[i]);
		for (int j = 0; j < 4; j++) 
			printf(" 0x%x", e_fids[i][j]);
		printf(" ]\n");
	}
	printf("\n");

	printf("djb2 hash:\n");
	for (int i = 0; i < n_e; i++) {
		int h = djb2(e_fids[i], 5381) ;
		printf("%s: %d (%d)\n", e_fids[i], h % buckets, h);
	}
	printf("\nspecialized hash 1 (3rd+4th byte - [2nd bit 3rd byte]):\n");
	for (int i = 0; i < n_e; i++) {
		printf("%s: ", e_fids[i]);
		printf("%d \n", (e_fids[i][2] + e_fids[i][3] - ((e_fids[i][2] & 0x2) >> 1)) % buckets);
	}
	printf("\nspecialized hash 2 (concat 0x2 3rd byte + 0x8 3rd byte + 0x1 2nd byte):\n");
	for (int i = 0; i < n_e; i++) {
		printf("%s: ", e_fids[i]);
		printf("%d \n", (((e_fids[i][2] & 2) << 1) + 
						 ((e_fids[i][2] & 8) >> 2) + 
						 (e_fids[i][1] & 1))
						  % buckets);
	}
	printf("\nBrute-force search (linear hashing): \n");
	int perf_a = -1, perf_b = -1, num_perf = 0;
	char perf_order[5][5] = { { 0 } };
	for (int a=1; a < 200; a++) {
		for (int b=0; b < 200; b++) {
			// printf("hash func %d,%d: ", a, b);
			int *isset = calloc(buckets, sizeof(int));
			int perf = 1;
			char order[5][5] = { { 0 } };
			for (int k = 0; k<buckets; k++) {
				unsigned int h = linear_hash(a, b, int_e_fids[k]) % buckets;
				if (isset[h]++ > 1) { 
					perf = 0;
					break;
				};
				strncpy(order[h], e_fids[k], 5);
			}
			
			if (perf) {
				num_perf++;
				perf_a = a;
				perf_b = b;
				for (int _i = 0; _i < n_e; _i++) strncpy(perf_order[_i], order[_i], 5);
			}
			free(isset);
		}
	}
	printf("number of perfect hash functions found: %d\n", num_perf);
	if (num_perf > 0) {
		printf("e.g. perfect hashing found: a=%d, b=%d, p=%d, order=[", perf_a, perf_b, p);
		for (int _i = 0; _i < n_e; _i++) printf("%s, ", perf_order[_i]);
		printf("]\n\n");
	}

	/* UNIVERSAL HASHING
	* m = n = # of buckets = 5
    * p = 65805703

    * Z_p = {0,1,2,3,4,5,6}
    * Z_p^* = {1,2,3,4,5,6}

    * h_{a,b}(k) = ((a*k + b) mod p) mod m

    * hash family:
    * H_{p,m} = {h_{a,b} : a in Z_p^* and b in Z_p}
	*/
	// srand(time(NULL));
	// int a = (rand() % (p - 1)) + 1;
	// int b = (rand() % (p));
	// printf("\nrandom universal hashing (p=%d, a=%d, b=%d): \n", p, a, b);
	// int no_col = 1;
	// int *isset = calloc(buckets, sizeof(int));
 	// for (int i = 0; i < n; i++) {
	// 	int h = linear_hash(a, b, ikeys[i]);
	// 	if (isset[h]) no_col = 0;
	// 	else isset[h]=1;
	// 	printf("%s: %d\n", keys[i], h);
	// }
	// printf("is perfect: %d\n", no_col);
	// e.g. min perfect hash function for <keys> (p=65805703, a=46856658, b=669512)

	printf("============All FIDs hash function search============\n");
	buckets = n_all;
	p = 4761373;

	int set[83+120];

	printf("\nBrute-force search for PHF (linear hashing): \n");
	for (int s = 234; s<234; s++) {
		printf("%d-%d: ", s*1000, (s+1)*1000);

		int min_perf = 0;
		int min_collisions = INT_MAX;
		perf_a = -1;
		perf_b = -1;
		num_perf = 0;
		for (int a=s*1000 + 1; a < (s+1)*1000; a++) {
			for (int b=s*1000; b < (s+1)*1000; b++) {
				for (buckets = n_all; buckets < n_all + 40; buckets++) {
					int collisions = 0;

					for (int k = 0; k<n_all; k++) {
						unsigned int h = linear_hash(a, b, int_all_fids[k]) % buckets;
						if (set[h]++) collisions++;
					}

					if (!collisions) {
						num_perf++;
						perf_a = a;
						perf_b = b;
						min_perf = buckets == n_all;
					} else if (min_collisions > collisions) min_collisions = collisions;
					
					for (int _i = 0; _i < 183; _i++) set[_i] = 0;
				}
			}
		}
		printf("number of perfect hash functions found: %d ", num_perf);
		if (num_perf > 0) printf("(e.g. perfect hashing found (minimal=%d): a=%d, b=%d, p=%d)\n", min_perf, perf_a, perf_b, p);
		else printf("(least collisions: %d)\n", min_collisions);
	}
	
	buckets = n_all;

	//	unsigned long hash;
	// for (hash = 0; hash < p; hash++) {
	// 	memset(isset, 0, buckets*sizeof(int));
	// 	no_col = 1;
	// 	for (int i = 0; i < id3_n; i++) {
	// 		int h = djb2(all_id3[i], hash) % buckets;
	// 		if (isset[h]) {
	// 			no_col = 0;
	// 			break;
	// 		}
	// 		isset[h]=1;
	// 	}
	// 	if (no_col) {
	// 		printf("all_id3: is perfect (p=%d, a=%d, b=%d)\n", p, a, b);
	// 		for (int i = 0; i < id3_n; i++) {
	// 			int h = djb2(all_id3[i], hash) % buckets;
	// 			printf("%s: %d\n", all_id3[i], h);
	// 		}
	// 		break;
	// 	}
	// }

	printf("\ngperf generated hash function:\n");
	int *isset = calloc(MAX_HASH_VALUE+1, sizeof(int));
	int no_col = 1, max_h = 0;
	for (int k = 0; k<buckets; k++) {
		unsigned int h = hash(all_fids[k], 4);
		// printf("%s: %d\n", all_fids[k], h);
		if (isset[h]) no_col = 0;
		isset[h]++;
		if (max_h < h) max_h = h;
	}
	printf("is perfect: %d\n", no_col);
	printf("is minimal: %d (bucket_size=%d)\n", max_h + 1 == n_all, max_h + 1);
	free(isset);

    return 0;
}
