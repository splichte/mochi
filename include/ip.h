#pragma once

// https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

int send_ip(int8_t *tpkt, uint16_t tlen, uint8_t protocol);

