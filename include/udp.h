#pragma once
#include <stdint.h>
#include "kalloc.h"

int send_pkt_to_udp(uint8_t *pkt, uint16_t len, uint16_t src_port, uint16_t dst_port);

uint8_t *recv_pkt_from_udp(uint16_t *src_port, uint16_t *dst_port);
