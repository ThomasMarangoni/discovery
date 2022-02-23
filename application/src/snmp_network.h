#ifndef SNMP_NETWORK_H
#define SNMP_NETWORK_H

#include "lib/sds.h"
#include "lib/gll.h"

#include "ip.h"

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

int snmp_network_scan_run(sds* exec_path_str, sds* host_str, sds* community_str, gll_t** snmp_device_list);
int snmp_network_walk_batch_run(sds* exec_path_str, sds* community_str,  sds* oid_str, gll_t** network_tree_list, gll_t** snmp_device_list);
int snmp_network_walk_run(sds* exec_path_str, sds* community_str, ipv4_t host_ip,  sds* oid_str, gll_t** network_tree_list);
int snmp_network_walk_batch_run_str(sds* exec_path_str, sds* community_str, ipv4_t host_ip,  gll_t** oid_list, sds* return_str);
int snmp_network_walk_run_str(sds* exec_path_str, sds* community_str, ipv4_t host_ip,  sds* oid_str, sds* return_str);

#ifdef DEBUG
void snmp_network_print_snmp_device_list_debug(void* element);
#endif
#endif