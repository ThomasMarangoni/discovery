#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdbool.h>
#include <sqlite3.h>

#include "ip.h"
#include "lib/gll.h"

typedef struct
{
    int id;
    ipv4_t management_address;
    int capabilities_supported;
    int capabilities_enabled;
    sds system_name;
} database_device_t;

typedef struct
{
    int id;
    int device_id;
    int interface_id;
    sds mac_address;
    uint32_t max_speed;
    int operating_status;
    sds name;
} database_port_t;

typedef struct
{
    int id;
    int port_a_id;
    int port_b_id;
    int link_type;
    uint32_t speed;
    uint32_t length;
} database_link_t;

int database_free_device(database_device_t *device);
int database_free_port(database_port_t *port);
int database_free_link(database_link_t *link);

int database_open(const char *database_name, sqlite3 **database);
int database_close(sqlite3 *database);
int database_generate(sqlite3 *database);
int database_drop(sqlite3 *database);

/* ------------ Device Section ------------ */
int database_get_id_from_device(sqlite3 *database, database_device_t *device);
bool database_does_device_exist(sqlite3 *database, database_device_t *device);
int database_insert_device(sqlite3 *database, database_device_t *device);
int database_update_device_by_management_address(sqlite3 *database, database_device_t *device);
int database_delete_device(sqlite3 *database, database_device_t *device);

/* ------------ Ports Section ------------ */
int database_get_id_from_port(sqlite3 *database, database_port_t *port);
bool database_does_port_exist(sqlite3 *database, database_port_t *port);
int database_insert_port(sqlite3 *database, database_port_t *port);
int database_update_port_by_mac_address(sqlite3 *database, database_port_t *port);
int database_get_ports_by_device(sqlite3 *database, database_device_t *device, gll_t *ports_list);
int database_delete_port(sqlite3 *database, database_port_t *port);

/* ------------ Links Section ------------ */
int database_get_id_from_link(sqlite3 *database, database_link_t *link);
bool database_does_link_exist(sqlite3 *database, database_link_t *link);
int database_insert_links(sqlite3 *database, database_link_t *link);
int database_update_link_by_port_ids(sqlite3 *database, database_link_t *link);
int database_get_link_by_port(sqlite3 *database, database_port_t *port, database_link_t *link);
int database_delete_link(sqlite3 *database, database_link_t *link);

#endif