#ifndef SNMP_PARSE_H
#define SNMP_PARSE_H

#include <string.h>
#include <stdbool.h>

#include "lib/sds.h"

#include "ip.h"
#include "database.h"

#define STR_CONTAINS(str_a, str_b) (strstr(str_a, str_b) != NULL)
#define STR_EQUAL(str_a, str_b) (strcmp(str_a, str_b) == 0)

typedef struct
{
    sds oid_str_ptr;
    sds oid_id_str_ptr;
    sds data_type_str_ptr;
    sds data_str_ptr;
} oid_string_tuple_t;

typedef struct
{
    ipv4_t host;
    gll_t* oid_string_tuple_list;
} host_data_pair_t;


bool snmp_parse_check_from_list(sds* snmp_data_str, gll_t** oid_list);
void snmp_parse_from_list(sds* snmp_data_str, ipv4_t host_ip, gll_t** oid_list, gll_t** oid_string_tuple_list);
void snmp_parse_free_oid_string_tuple_t(void* oid_string_tuple);
void snmp_parse_free_host_data_pair_t(void* host_data_pair);
void snmp_host_data_pair_to_database(host_data_pair_t *host_data_pair, sqlite3  *database);

#endif