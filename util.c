#ifndef UTIL_FUNCS
#define UTIL_FUNCS

int synchsafeint32ToInt(char c[4]) {
    return (c[0] << 21) | ((c[1] << 14) | ((c[2] << 7) | (c[3] | (int)0)));
}

void intToSynchsafeint32(int x, char ssint[4]) {
    for (int i=0; i<4; i++) ssint[3-i] = (x & (0x7F << i*7)) >> i*7;
}

char *concatenate(char *s1, char *s2) {
    char *s3 = calloc(strlen(s1) + strlen(s2) + 1, 1);

    strncat(s3, s1, strlen(s1));
    strncat(s3, s2, strlen(s2));

    return s3;
}

#endif