#pragma once

#include "eth.h"

void initialize_e1000();

int send_eth_to_e1000(eth_pkt *ep);
eth_pkt recv_eth_from_e1000();

