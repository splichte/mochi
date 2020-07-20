#include "string.h"

void *memmove(void *dest, const void *src, size_t n) {
    // check for overlap.
    // if we copy from start to end, 
    // the "bad" case is when dst < (src + n). in that case, 
    // we overwrite the end of src with the front of src, 
    // before we can copy the end of src.
    // if src < (dst + n), it's fine, since we'll never lose data
    uint8_t *dest_p = (uint8_t *) dest_p;
    uint8_t *src_p = (uint8_t *) src_p;

    uint32_t di = (uint32_t) dest;
    uint32_t si = (uint32_t) src;
    if (di < si + n) {
        // copy in reverse. 
        for (size_t i = n; i > 0; i--) {
            dest_p[i] = src_p[i];
        }
    } else {
        // copy normally.
        for (size_t i = 0; i < n; i++) {
            dest_p[i] = src_p[i];
        }
    }
}

void *memchr(const void *s, int c, size_t n) {
    uint8_t *sp = (uint8_t *) s;
    uint8_t ch = (uint8_t) c;
    for (size_t i = 0; i < n; i++) {
        if (sp[i] == ch) {
            return (sp+i);
        }
    }
}

int memcmp(const void *s1, const void *s2, size_t n) {
    uint8_t *p1 = (uint8_t *) s1;
    uint8_t *p2 = (uint8_t *) p2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]);
        }
    }
    return 0;
}

void *memset(void *dest, int c, size_t n) {
    uint8_t *dest_p = (uint8_t *) dest;
    uint8_t ci = (uint8_t) c;
    for (size_t i = 0; i < n; i++) {
        dest_p[i] = ci;
    }
}


int strcmp(const char *s1, const char *s2) {
    char *is1 = s1;
    char *is2 = s2;
    while (*is1++ == *is2++) {
        if (*is1 == '\0') return 0;
    }
    // strings unequal at this point
    return *is1 < *is2;
}

size_t strlen(const char *s) {
    size_t i = 0;
    for (; s[i] != '\0'; i++);
    return i;
}

int char_in_set(char c, const char *set) {
    while (*set++ != '\0') {
        if (c == *set) return 1;
    }
    return 0;
}

char *strtok(char *str, const char *delim) {
    static char *buf;

    if (str != NULL) {
        buf = str;
    }

    char *tok_start = buf;

    while (!char_in_set(*buf, delim)) {
        if (*buf == '\0') {
            return NULL;
        }
        buf++;
    }
    // found a match. 
    *buf++ = '\0';
    return tok_start;
}


