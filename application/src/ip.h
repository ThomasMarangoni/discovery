#ifndef IP_H
#define IP_H

#include "lib/sds.h"

typedef uint32_t ipv4_t;

void print_ipv4(void* ip);
ipv4_t* malloc_ipv4(ipv4_t ip);
ipv4_t free_ipv4(ipv4_t* ip_ptr);
void free_ipv4_void(void* ip_ptr);
ipv4_t ipv4_from_hex_str(sds* ip_hex_str);
ipv4_t ipv4_from_str(sds* ip_str);
sds str_from_ipv4(ipv4_t ipv4);

#endif