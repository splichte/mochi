#include <stdint.h>
#include "net.h"

uint16_t htons(uint16_t i) {
    uint16_t o = 0;
    o |= (i >> 8) & 0xff;
    o |= (i << 8) & 0xff00;
    return o;
}

uint32_t hton(uint32_t i) {
    uint32_t o = 0;
    o |= (i >> 24) & 0xff;
    o |= (i >> 8) & 0xff00;
    o |= (i << 8) & 0xff0000;
    o |= (i << 24) & 0xff000000;
    return o;
}

uint8_t MAC_ADDR_ARR[6] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56
};

