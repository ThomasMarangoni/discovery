#include "database.h"

#include <string.h>
#include <stdlib.h>

#include "kcolor.h"
#include "lib/sds.h"

/**
 * @brief Frees a device
 * 
 * @param device the device to be unallocated
 * @return 0 on success, 1 on failure.
 */
int database_free_device(database_device_t *device)
{
    sdsfree(device->system_name);
    free(device);

    return EXIT_SUCCESS;
}

/**
 * @brief Frees a port
 * 
 * @param port the port to be unallocated
 * @return 0 on success, 1 on failure.
 */
int database_free_port(database_port_t *port)
{
    sdsfree(port->mac_address);
    sdsfree(port->name);
    free(port);

    return EXIT_SUCCESS;
}

/**
 * @brief Frees a link
 * 
 * @param link the link to be unallocated
 * @return 0 on success, 1 on failure.
 */
int database_free_link(database_link_t *link)
{
    free(link);

    return EXIT_SUCCESS;
}

/**
 * @brief opens a sqlite3 database connection with the given name if the database doesn't exist it will be created.
 * 
 * @param database_name name of the database file to be open
 * @param database returns a open connetion to a sqlite3 database.
 * @return 0 on success, 1 on failure.
 */
int database_open(const char *database_name, sqlite3 **database)
{
    const char* sql_pragma_fkeys = "PRAGMA foreign_keys = ON";
    char *zErrMsg = 0;

    if(sqlite3_open(database_name, database))
    {
        printf(KRED"[ERROR] Can't open database: %s\n"KNORMAL, sqlite3_errmsg(*database));
        return EXIT_FAILURE;
    }

    if( sqlite3_exec(*database, sql_pragma_fkeys, NULL, 0, &zErrMsg) != SQLITE_OK ){
        printf(KRED"[ERROR] database_open - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief closes a open database connection
 * 
 * @param database open connection to a sqlite3 database.
 * @return 0 on success, 1 on failure.
 */
int database_close(sqlite3 *database)
{
    if(sqlite3_close(database))
    {
        printf(KRED"[ERROR] Can't close database: %s\n"KNORMAL, sqlite3_errmsg(database));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief generates the database structure if it doesn't exist
 * 
 * @param database open connection to a sqlite3 database.
 * @return 0 on success, 1 on failure.
 */
int database_generate(sqlite3 *database)
{
    const char* sql_create_devices = "CREATE TABLE IF NOT EXISTS \"Devices\" (\"Id\" INTEGER,  \"ManagementAddress\" INTEGER,  \"CapabilitiesSupported\" INTEGER,  \"CapabilitiesEnabled\" INTEGER, \"SystemName\" TEXT, PRIMARY KEY(\"Id\" AUTOINCREMENT));";

    const char* sql_create_ports = "CREATE TABLE IF NOT EXISTS \"Ports\" (\"Id\" INTEGER, \"DeviceId\" INTEGER, \"InterfaceId\" INTEGER, \"MACAddress\" TEXT, \"MaxSpeed\" INTEGER, \"OperatingStatus\" INTEGER, \"Name\" TEXT, PRIMARY KEY(\"Id\" AUTOINCREMENT), FOREIGN KEY(\"DeviceId\") REFERENCES \"Devices\"(\"Id\"));";

    const char* sql_create_links = "CREATE TABLE IF NOT EXISTS \"Links\" (\"Id\" INTEGER, \"PortAId\" INTEGER, \"PortBId\" INTEGER, \"LinkType\" INTEGER, \"Speed\" INTEGER, \"Length\" INTEGER, PRIMARY KEY(\"Id\" AUTOINCREMENT), FOREIGN KEY(\"PortAId\") REFERENCES \"Ports\"(\"Id\"), FOREIGN KEY(\"PortBId\") REFERENCES \"Ports\"(\"Id\"));";

    char *zErrMsg = 0;

    if( sqlite3_exec(database, sql_create_devices, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_generate - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    if( sqlite3_exec(database, sql_create_ports, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_generate - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    if( sqlite3_exec(database, sql_create_links, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_generate - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief deletes the database structure
 * 
 * @param database open connection to a sqlite3 database.
 * @return 0 on success, 1 on failure.
 */
int database_drop(sqlite3 *database)
{
    const char *sql_drop_links = "DROP TABLE IF EXISTS \"Links\";";
    const char *sql_drop_ports = "DROP TABLE IF EXISTS \"Ports\";";
    const char *sql_drop_devices = "DROP TABLE IF EXISTS \"Devices\";";

    char *zErrMsg = 0;

    if( sqlite3_exec(database, sql_drop_links, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_drop - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    if( sqlite3_exec(database, sql_drop_ports, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_drop - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    if( sqlite3_exec(database, sql_drop_devices, NULL, 0, &zErrMsg) != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_drop - SQL error: %s\n"KNORMAL, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* ------------ Device Section ------------ */
static int database_get_id_from_device_callback(void *device_ptr,  int argc, char **argv, char **col_name)
{
    database_device_t *device = (database_device_t *) device_ptr;

    if(argc > 0 && !strcmp("Id", col_name[0]))
        device->id = strtol(argv[0], NULL, 10);

    return 0;
}

/**
 * @brief gets the id of an device, the management address is used as a search key.
 * 
 * @param database open connection to a sqlite3 database.
 * @param device the device to be searched, id will be -1 if not found in database.
 * @return 0 on success, 1 on failure.
 */
int database_get_id_from_device(sqlite3 *database, database_device_t *device)
{
    sds sql_select_device = sdscatfmt(sdsempty(), "SELECT id FROM \"Devices\" WHERE ManagementAddress = %u LIMIT 1;", device->management_address);

    char *zErrMsg = 0;
    device->id = -1;

    int rc = sqlite3_exec(database, sql_select_device, database_get_id_from_device_callback, device, &zErrMsg);
    sdsfree(sql_select_device);
    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_get_id_from_device - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief checks if the given device exists in the database, management is used as a search key.
 * 
 * @param database open connection to a sqlite3 database.
 * @param device the device to check, id will be -1 if not found.
 * @return true, if device exists
 * @return false, if device doesn't exists
 */
bool database_does_device_exist(sqlite3 *database, database_device_t *device)
{
    if(database_get_id_from_device(database, device))
        return false;

    return device->id != -1;
}

/**
 * @brief inserts given device into database.
 * 
 * @param database open connection to a sqlite3 database.
 * @param device device to be inserted, id will be set to database value after insert.
 * @return 0 on success, 1 on failure.
 */
int database_insert_device(sqlite3 *database, database_device_t *device)
{
    sds sql_insert_device = sdscatfmt(sdsempty(), "INSERT INTO \"Devices\" (ManagementAddress, CapabilitiesSupported, CapabilitiesEnabled, SystemName) VALUES ( %u, %i, %i, \"%S\");", device->management_address, device->capabilities_supported, device->capabilities_enabled, device->system_name);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_insert_device, NULL, 0, &zErrMsg);
    sdsfree(sql_insert_device);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_insert_device - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    database_get_id_from_device(database, device);

    return EXIT_SUCCESS;
}

/**
 * @brief updates given device in database, managemant address will be used as search key.
 * 
 * @param database open connection to a sqlite3 database.
 * @param device device to be updated, id will be set to database value after update.
 * @return 0 on success, 1 on failure.
 */
int database_update_device_by_management_address(sqlite3 *database, database_device_t *device)
{
    sds sql_update_device = sdscatfmt(sdsempty(), "UPDATE \"Devices\" SET CapabilitiesSupported = %i, CapabilitiesEnabled = %i, SystemName = \"%S\" WHERE ManagementAddress = %u;", device->capabilities_supported, device->capabilities_enabled, device->system_name, device->management_address);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_update_device, NULL, 0, &zErrMsg);
    sdsfree(sql_update_device);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_update_device_by_management_address - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    database_get_id_from_device(database, device);

    return EXIT_SUCCESS;
}

/**
 * @brief Deletes a device and all its ports and links.
 * 
 * @param database open connection to a sqlite3 database.
 * @param device the device that should be deleted.
 * @return 0 on success, 1 on failure.
 */
int database_delete_device(sqlite3 *database, database_device_t *device)
{
    /// Deletes all existing ports and their links
    gll_t *ports_list = gll_init();
    database_get_ports_by_device(database, device, ports_list);

    if(ports_list->size > 0)
    {
        gll_node_t *node = ports_list->first;
        for(int i = 0; i < ports_list->size; i++)
        {
            database_port_t *port = (database_port_t *)node;
            database_delete_port(database, port);
            database_free_port(port);

            node = node->next;
        }
        gll_destroy(ports_list);
    }

    /// Delete device
    sds sql_delete_device = sdscatfmt(sdsempty(), "DELETE FROM \"Devices\" WHERE id = %i;", device->id);
    
    char *zErrMsg = 0;

    int rc = sqlite3_exec(database, sql_delete_device, NULL, 0, &zErrMsg);
    sdsfree(sql_delete_device);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_delete_device - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* ------------ Ports Section ------------ */
static int database_get_id_from_port_callback(void *port_ptr,  int argc, char **argv, char **col_name)
{
    database_port_t *port = (database_port_t *) port_ptr;

    if(argc > 0 && !strcmp("Id", col_name[0]))
        port->id = strtol(argv[0], NULL, 10);

    return 0;
}

/**
 * @brief gets id from database for given port, MAC address is used as search key.
 * 
 * @param database open connection to a sqlite3 database.
 * @param port the port that is used for search, id will be set if found, else id will be set to -1
 * @return 0 on success, 1 on failure. 
 */
int database_get_id_from_port(sqlite3 *database, database_port_t *port)
{
    sds sql_select_port = sdscatfmt(sdsempty(), "SELECT id FROM \"Ports\" WHERE MACAddress = \"%S\" LIMIT 1;", port->mac_address);

    char *zErrMsg = 0;
    port->id = -1;

    int rc = sqlite3_exec(database, sql_select_port, database_get_id_from_port_callback, port, &zErrMsg);
    sdsfree(sql_select_port);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_get_id_from_port - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Checks if a port exists in given database
 * 
 * @param database open connection to a sqlite3 database.
 * @param port  the port that is used for search, id will be set if found, else id will be set to -1
 * @return true, if port exists
 * @return false, if port doesn't exists
 */
bool database_does_port_exist(sqlite3 *database, database_port_t *port)
{
    if(database_get_id_from_port(database, port))
        return false;

    return port->id != -1;
}

/**
 * @brief inserts port into given database
 * 
 * @param database open connection to a sqlite3 database.
 * @param port the port to be insert, id will be set to database value after insert.
 * @return 0 on success, 1 on failure.
 */
int database_insert_port(sqlite3 *database, database_port_t *port)
{
    sds sql_insert_port = sdscatfmt(sdsempty(), "INSERT INTO \"Ports\" (DeviceId, InterfaceId, MACAddress, MaxSpeed, OperatingStatus, Name) VALUES ( %i, %i, \"%S\", %u, %i, \"%S\");", port->device_id, port->interface_id, port->mac_address, port->max_speed, port->operating_status, port->name);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_insert_port, NULL, 0, &zErrMsg);
    sdsfree(sql_insert_port);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_insert_port - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    database_get_id_from_port(database, port);

    return EXIT_SUCCESS;
}

/**
 * @brief update port in database, MAC address will be used as search key.
 * 
 * @param database open connection to a sqlite3 database.
 * @param port port to be updated in database.
 * @return 0 on success, 1 on failure.
 */
int database_update_port_by_mac_address(sqlite3 *database, database_port_t *port)
{
    sds sql_update_port = sdscatfmt(sdsempty(), "UPDATE \"Ports\" SET InterfaceId = %i, MaxSpeed = %u, OperatingStatus = %i, Name = \"%S\" WHERE MacAddress = %u;", port->interface_id, port->max_speed, port->operating_status, port->name, port->mac_address);

    char *zErrMsg = 0;

    int rc = sqlite3_exec(database, sql_update_port, NULL, 0, &zErrMsg);
    sdsfree(sql_update_port);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_update_port_by_mac_address - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int database_get_ports_by_device_callback(void *ports_list_ptr,  int argc, char **argv, char **col_name)
{
    gll_t *ports_list = (gll_t *)ports_list_ptr;

    database_port_t *port = (database_port_t *)malloc(sizeof(database_port_t));

    port->id = strtol(argv[0], NULL, 10);
    port->device_id = strtol(argv[1], NULL, 10);
    port->interface_id = strtol(argv[2], NULL, 10);
    port->mac_address = sdsnew(argv[3]);
    port->max_speed = strtol(argv[4], NULL, 10);
    port->operating_status = strtol(argv[5], NULL, 10);
    port->name = sdsnew(argv[6]);

    gll_pushBack(ports_list, port);

    return 0;
}

/**
 * @brief getting a list of ports of a device, device management address is used as search key
 * 
 * @param database open connection to a sqlite3 database.
 * @param device the device used as look up.
 * @param ports_list gll list of found ports, need to be unallocated manually
 * @return 0 on success, 1 on failure.
 */
int database_get_ports_by_device(sqlite3 *database, database_device_t *device, gll_t *ports_list)
{
    sds sql_select_ports = sdscatfmt(sdsempty(), "SELECT * FROM \"Ports\" WHERE ManagementAddress = %u;", device->management_address);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_select_ports, database_get_ports_by_device_callback, ports_list, &zErrMsg);
    sdsfree(sql_select_ports);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_get_ports_by_device - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Deletes a port and its link.
 * 
 * @param database open connection to a sqlite3 database.
 * @param port the port that should be deleted.
 * @return 0 on success, 1 on failure.
 */
int database_delete_port(sqlite3 *database, database_port_t *port)
{
    /// Delete existing link
    database_link_t *link = (database_link_t *)malloc(sizeof(database_link_t));
    link->id = -1;

    database_get_link_by_port(database, port, link);
    if(link->id != -1)
    {
        database_delete_link(database, link);
    }
    database_free_link(link);

    /// Delete the port
    sds sql_delete_port = sdscatfmt(sdsempty(), "DELETE FROM \"Ports\" WHERE id = %i;", port->id);
    
    char *zErrMsg = 0;

    int rc = sqlite3_exec(database, sql_delete_port, NULL, 0, &zErrMsg);
    sdsfree(sql_delete_port);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_delete_port - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* ------------ Links Section ------------ */
static int database_get_id_from_link_callback(void *link_ptr,  int argc, char **argv, char **col_name)
{
    database_link_t *link = (database_link_t *) link_ptr;

    if(argc > 0 && !strcmp("Id", col_name[0]))
        link->id = strtol(argv[0], NULL, 10);

    return 0;
}

/**
 * @brief gets the id of a link, portA or portB is used as search key
 * 
 * @param database open connection to a sqlite3 database. 
 * @param link the link to get the id of, id is set to found database value, else set to -1
 * @return 0 on success, 1 on failure.
 */
int database_get_id_from_link(sqlite3 *database, database_link_t *link)
{
    sds sql_select_link = sdscatfmt(sdsempty(), "SELECT id FROM \"Links\" WHERE PortAId = %u AND PortBId = %u LIMIT 1;", link->port_a_id, link->port_b_id);

    char *zErrMsg = 0;
    link->id = -1;

    int rc = sqlite3_exec(database, sql_select_link, database_get_id_from_link_callback, link, &zErrMsg);
    sdsfree(sql_select_link);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_insert_links - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief checks if a link exist in given database
 * 
 * @param database open connection to a sqlite3 database. 
 * @param link the link which will be used for search, id will be set when found, else will be set to -1
 * @return true, if link exists
 * @return false, if link doesn't exist
 */
bool database_does_link_exist(sqlite3 *database, database_link_t *link)
{
    if(database_get_id_from_link(database, link))
        return false;

    return link->id != -1;
}

/**
 * @brief inserts link into given database
 * 
 * @param database open connection to a sqlite3 database.
 * @param link given link to be inserted, id will be set to database value after insert
 * @return 0 on success, 1 on failure.
 */
int database_insert_links(sqlite3 *database, database_link_t *link)
{
    sds sql_insert_link = sdscatfmt(sdsempty(), "INSERT INTO \"Links\" (PortAId, PortBId, LinkType, Speed, Length) VALUES ( %i, %i, %i, %u, %u );", link->port_a_id, link->port_b_id, link->link_type, link->speed, link->length);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_insert_link, NULL, 0, &zErrMsg);
    sdsfree(sql_insert_link);
    
    if( rc != SQLITE_OK ){
        printf(KRED"[ERROR] database_insert_links - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    database_get_id_from_link(database, link);

    return EXIT_SUCCESS;
}

/**
 * @brief updates information of a link, the link is searched by portA and portB
 * 
 * @param database open connection to a sqlite3 database.
 * @param link link to be updated.
 * @return 0 on success, 1 on failure.
 */
int database_update_link_by_port_ids(sqlite3 *database, database_link_t *link)
{
    sds sql_update_link = sdscatfmt(sdsempty(), "UPDATE \"Links\" SET LinkType = %u, Speed = %u, Length = %u WHERE PortAId = %u AND PortBId = %u;", link->link_type, link->speed, link->length, link->port_a_id, link->port_b_id);

    char *zErrMsg = 0;

    
    int rc = sqlite3_exec(database, sql_update_link, NULL, 0, &zErrMsg);
    sdsfree(sql_update_link);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_update_link_by_port_ids - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int database_get_link_by_port_callback(void *link_ptr,  int argc, char **argv, char **col_name)
{
    database_link_t *link = (database_link_t *) link_ptr;

    link->id = strtol(argv[0], NULL, 10);
    link->port_a_id = strtol(argv[1], NULL, 10);
    link->port_b_id = strtol(argv[2], NULL, 10);
    link->link_type = strtol(argv[3], NULL, 10);
    link->speed = strtol(argv[4], NULL, 10);
    link->length = strtol(argv[5], NULL, 10);

    return 0;
}

/**
 * @brief Gets a link from a given port.
 * 
 * @param database open connection to a sqlite3 database.
 * @param port key of the search
 * @param link the found link, needs to be allocated before. id is set to -1 if no link has been found.
 * @return 0 on success, 1 on failure.
 */
int database_get_link_by_port(sqlite3 *database, database_port_t *port, database_link_t *link)
{
    sds sql_select_link = sdscatfmt(sdsempty(), "SELECT * FROM \"Links\" WHERE PortAId = %u OR PortBId = %u LIMIT 1;", port->id, port->id);

    char *zErrMsg = 0;

    link->id = -1;
    
    int rc = sqlite3_exec(database, sql_select_link, database_get_link_by_port_callback, link, &zErrMsg);
    sdsfree(sql_select_link);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_get_link_by_port - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Deletes a link.
 * 
 * @param database open connection to a sqlite3 database.
 * @param link the link that should be deleted.
 * @return 0 on success, 1 on failure.
 */
int database_delete_link(sqlite3 *database, database_link_t *link)
{
    sds sql_delete_link = sdscatfmt(sdsempty(), "DELETE FROM \"Links\" WHERE id = %i;", link->id);
    
    char *zErrMsg = 0;
    
    int rc = sqlite3_exec(database, sql_delete_link, NULL, 0, &zErrMsg);
    sdsfree(sql_delete_link);

    if( rc != SQLITE_OK )
    {
        printf(KRED"[ERROR] database_delete_link - SQL error: %d - %s\n"KNORMAL, rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}