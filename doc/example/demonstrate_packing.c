#include <stdio.h>

typedef struct {
    short one;
    int two;
} unpacked;

typedef struct __attribute__((packed)) {
    short one;
    int two;
} packed;

int main() {
    printf("%lu\n", sizeof (unpacked)); // 8
    printf("%lu\n", sizeof (packed)); // 6
}
