#pragma once

#include "eth.h"

void initialize_e1000();

int send_e1000(eth_pkt *ep);
int recv_e1000(eth_pkt *res);

