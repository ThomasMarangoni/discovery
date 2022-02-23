#ifndef NETWORK_TREE_NODES_H
#define NETWORK_TREE_NODES_H

#include "lib/gll.h"

#include "ip.h"

typedef struct
{
    ipv4_t* host_ip;
    gll_t* neighbors_ip_list;
} tree_node_t;

#endif