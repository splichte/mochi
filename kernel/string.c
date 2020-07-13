int strcmp(const char *s1, const char *s2) {
    char *is1 = s1;
    char *is2 = s2;
    while (*is1++ == *is2++) {
        if (*is1 == '\0') return 0;
    }
    // strings unequal at this point
    return *is1 < *is2;
}
