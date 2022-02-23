#ifndef SNMP_TRAP_H
#define SNMP_TRAP_H

#include "ip.h"

#include <stdbool.h>

#ifndef IP_BUFFER_SIZE
#define IP_BUFFER_SIZE 1000
#endif

#ifndef TRAP_SLEEP_US
#define TRAP_SLEEP_US 100000
#endif

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

int snmp_trap_read_data(ipv4_t **ip_buffer_out, int* ip_buffer_pos_out);
void snmp_trap_wait_for_thread(void);
int snmp_trap_daemon_setup(sds* exec_path_str, sds* community_str);

#endif