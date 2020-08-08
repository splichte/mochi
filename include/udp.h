#pragma once
#include <stdint.h>

int send_udp(uint8_t *pkt, uint16_t len, uint16_t src_port, uint16_t dst_port);

