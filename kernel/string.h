/* string.h -- implements some of the standard interface. */
/* https://en.wikibooks.org/wiki/C_Programming/string.h */
/* or K&R C, Appendix B Section B3. */

#include <stdint.h>
#include <stddef.h>

/* copies n bytes between two memory areas; areas may overlap.
 * returns pointer to dest. */
void *memmove(void *dest, const void *src, size_t n);

/* returns a pointer to the first occurrence of c in the first n bytes of s, or NULL if not found */
/* c is interpreted as unsigned char. https://man7.org/linux/man-pages/man3/memchr.3.html */
void *memchr(const void *s, int c, size_t n);

/* compares the first n bytes of two memory areas */
int memcmp(const void *s1, const void *s2, size_t n); 

/* overwrites a memory area with n copies of c
 * NOTE: c is given as an int to match the normal interface, 
 * but it should really be uint8_t (unsigned char), 
 * since that's what it's converted as. */
void *memset(void *dest, int c, size_t n); 


/* compares the two strings */
int strcmp(const char *s1, const char *s2);

char *strcpy(char *s, const char *ct);

size_t strlen(const char *s);

/* NOTE: you can't use string constants for this function. 
 * (matches standard lib)
 */
char *strtok(char *str, const char *delim);


