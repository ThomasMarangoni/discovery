#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "kcolor.h"
#include "snmp_oid.h"
#include "snmp_parse.h"

/**
 * @brief Precheck SNMP data with a OID list.
 * 
 * This function will check if the given OIDs from a list, are included in the given string.
 * 
 * @param line_str A string with lines containing OIDs.
 * @param oid_list A list with OID strings to check.
 * @return Status (true = Success, false = Failure)
 */
bool snmp_parse_check_from_list(sds *line_str, gll_t** oid_list)
{
    gll_t* oid_list_ptr = *oid_list;
    gll_node_t* current = oid_list_ptr->first;

    while(current != NULL)
    {
        sds oid_str = (sds) current->data;
        
        sds oid_search_dot_str = sdsdup(oid_str);
        oid_search_dot_str = sdscat(oid_search_dot_str, ".");

        sds oid_search_space_str = sdsdup(oid_str);
        oid_search_space_str = sdscat(oid_search_space_str, " ");

        if (!STR_CONTAINS(*line_str, oid_search_dot_str) && !STR_CONTAINS(*line_str, oid_search_space_str))
        {
            PRINT_DEBUG("Does not contain %s, line: %s\n", oid_str, *line_str);

            sdsfree(oid_search_space_str);
            sdsfree(oid_search_dot_str);

            return false;
        }

        sdsfree(oid_search_space_str);
        sdsfree(oid_search_dot_str);

        current = current->next;
    }

    return true;
}

/**
 * @brief Checks if a line contain a OID from the list
 * 
 * This function checks id a line contains a OID from a list with OID strings, if a OID has been found it will be returned via parameter.
 * 
 * @param line_str A string which should be checked for a OID.
 * @param oid_list A list of OID strings to compare with.
 * @param found_oid_str The OID which has been found, returns NULL if non has been found.
 * @return Status (true = Found a OID, false = Didn't found a OID)
 */
bool str_contains_oid_from_list(sds *line_str, gll_t** oid_list, sds* found_oid_str)
{
    int contains = 0;

    gll_t* oid_list_ptr = *oid_list;
    gll_node_t* current = oid_list_ptr->first;

    while(current != NULL)
    {
        sds oid_str = (sds) current->data;
        if (STR_CONTAINS(*line_str, oid_str))
        {
            sds tmp_line_str = *line_str;
            if(tmp_line_str[strlen(oid_str)] == '.' || tmp_line_str[strlen(oid_str)] == ' ')
            {
                if(contains == 0)
                {
                    *found_oid_str = sdsdup(oid_str);
                }
                contains++;
            }
        }
        current = current->next;
    }

    if(contains > 1)
    {
        printf(KYELLOW "[WARNING] Line with multiple OIDs found!\n" KNORMAL);
    }

    return (contains > 0);
}

/**
 * @brief Parse OIDs to Tuples with a OID List
 * 
 * This functions converts a line from a SNMP walk, which contains a OID from the given OID list,
 * to a Tuple (oid_string_tuple_t) and adds it to a list. The tuple contains following data:
 * OID, OID Sub ID, Data Type, Type.
 * 
 * @param line_str The output of a SNMP walk.
 * @param host_ip The IPv4 of the host, that was the target of the SNMP walk.
 * @param oid_list The list of OIDs to find in the line_str.
 * @param oid_string_tuple_list Outputs a gll containing the found OID in a Tuple.
 */
void snmp_parse_from_list(sds* line_str, ipv4_t host_ip, gll_t** oid_list, gll_t** oid_string_tuple_list)
{
    gll_t* string_tupel_list = *oid_string_tuple_list;

    sds *lines;
    int count, i;
    lines = sdssplitlen(*line_str, sdslen(*line_str), "\n", 1, &count);

    for (i = 0; i < count; i++)
    {
        sds line_str = lines[i];
        sds found_oid_str = NULL;
        if(str_contains_oid_from_list(&line_str, oid_list, &found_oid_str))
        {
            sds oid_str = sdsdup(found_oid_str);
            sds oid_id_str;
            sds data_type_str;
            sds data_str;

            char* type_start_ptr = strchr(line_str, '=');
            char* data_start_ptr = strchr(type_start_ptr, ':');
            char* oid_id_start_ptr = line_str + sdslen(found_oid_str);

            char* oid_id_dot_ptr = strchr(oid_id_start_ptr, '.');
            if(oid_id_dot_ptr != NULL)
            {
                oid_id_dot_ptr = strchr(oid_id_dot_ptr+1, '.');
            }

            //TODO: remove this if unused
            //char* oid_id_space_ptr = strchr(oid_id_start_ptr, ' ');
            

            if(data_start_ptr == NULL || type_start_ptr == NULL )
            {
                sds host_ip_str = str_from_ipv4(host_ip);
                printf(KYELLOW "[WARNING][%s] Line doesn't contain expected format: \"%s\".\n" KNORMAL, host_ip_str, line_str);

                sdsfree(host_ip_str);
                sdsfree(oid_str);

                if(found_oid_str != NULL)
                    sdsfree(found_oid_str);

                continue;
            }

            size_t oid_len = sdslen(found_oid_str);
            size_t data_len = strlen(data_start_ptr);
            size_t type_len = strlen(type_start_ptr) - data_len;
            size_t oid_id_len = strlen(line_str) - oid_len - data_len - type_len;

            oid_id_str = sdsnewlen(oid_id_start_ptr, oid_id_len);
            data_type_str = sdsnewlen(type_start_ptr, type_len);
            data_str = sdsnewlen(data_start_ptr, data_len);

            oid_id_str = sdstrim(oid_id_str, ". ");
            data_type_str = sdstrim(data_type_str, "= ");
            data_str = sdstrim(data_str, ": \"");

            sdsfree(found_oid_str);

            oid_string_tuple_t* oid_struct = malloc(sizeof(oid_string_tuple_t));
            
            oid_struct->oid_str_ptr = oid_str;
            oid_struct->oid_id_str_ptr = oid_id_str;
            oid_struct->data_type_str_ptr = data_type_str;
            oid_struct->data_str_ptr = data_str;

            gll_push(string_tupel_list, oid_struct);
        }
    }
    
    sdsfreesplitres(lines,count);
}

/**
 * @brief Cleanup oid_string_tuple_t
 * 
 * This function can be used to free a allocated oid_string_tuple_t, can be used with gll_each.
 * 
 * @param element a oid_string_tuple_t that has been allocated before.
 */
void snmp_parse_free_oid_string_tuple_t(void *oid_string_tuple)
{

    oid_string_tuple_t *tuple;
    tuple = (oid_string_tuple_t*)oid_string_tuple;
    
    sdsfree(tuple->data_str_ptr);
    sdsfree(tuple->data_type_str_ptr);
    sdsfree(tuple->oid_id_str_ptr);
    sdsfree(tuple->oid_str_ptr);

    free(oid_string_tuple);
}

/**
 * @brief Cleanup host_data_pair_t
 * 
 * This function can be used to free a allocated host_data_pair_t, can be used with gll_each.
 * 
 * @param host_data_pair a host_data_pair_t that has been allocated before.
 */
void snmp_parse_free_host_data_pair_t(void *host_data_pair)
{
    host_data_pair_t *pair_ptr;
    pair_ptr = (host_data_pair_t*)host_data_pair;

    gll_each(pair_ptr->oid_string_tuple_list, &snmp_parse_free_oid_string_tuple_t);
    gll_destroy(pair_ptr->oid_string_tuple_list);

    free(host_data_pair);
}

/**
 * @brief search for a oid and sub oid in a oid tuple list
 * 
 * @param oid oid to search for
 * @param oid_id oid sub id to search for
 * @param oid_string_tuple_list list of tuples to search in
 * @param tuple found tuple
 * @return true, if tuple found
 * @return false, if tuple not found
 */
static bool snmp_get_oid_string_tuple(const char *oid, const char* oid_id, gll_t *oid_string_tuple_list, oid_string_tuple_t *tuple)
{
    tuple->oid_str_ptr = sdsempty();
    tuple->oid_id_str_ptr = sdsempty();
    tuple->data_type_str_ptr = sdsempty();
    tuple->data_str_ptr = sdsempty();

    gll_node_t *current = oid_string_tuple_list->first;

    for(int i = 0; i < oid_string_tuple_list->size; i++)
    {
        oid_string_tuple_t* data = (oid_string_tuple_t*)current->data;

        if(STR_EQUAL(oid, data->oid_str_ptr) && STR_EQUAL(oid_id, data->oid_id_str_ptr))
        {
            
            tuple->oid_str_ptr = sdsdup(data->oid_str_ptr);
            tuple->oid_id_str_ptr = sdsdup(data->oid_id_str_ptr);
            tuple->data_type_str_ptr = sdsdup(data->data_type_str_ptr);
            tuple->data_str_ptr = sdsdup(data->data_str_ptr);

            break;
        }

        current = current->next;
    }

    return sdslen(tuple->oid_str_ptr) > 0;
}

/**
 * @brief converts mac addresses from hex string to string
 * 
 * @param mac_address hex string mac address to convert.
 * @return converted mac address as string
 */
static sds snmp_fix_hex_mac_address(sds mac_address)
{
    sds* token_str_arr;
    int count;

    sdstoupper(mac_address);

    /// If MAC-Address segment doesn't contain leading zero, the zero will get added.
    token_str_arr = sdssplitlen(mac_address, sdslen(mac_address), " ", 1, &count);   
    sdsclear(mac_address);

    for (int i = 0; i < count-1; i++)
    {
        if(sdslen(token_str_arr[i]) == 1)
            mac_address = sdscatfmt(mac_address, "0%S:", token_str_arr[i]);
        else
            mac_address = sdscatfmt(mac_address, "%S:", token_str_arr[i]);
    }

    if(sdslen(token_str_arr[count-1]) == 1)
        mac_address = sdscatfmt(mac_address, "0%S", token_str_arr[count-1]);
    else
        mac_address = sdscatfmt(mac_address, "%S", token_str_arr[count-1]);

    sdsfreesplitres(token_str_arr, count);

    return mac_address;
}

/**
 * @brief parses the interface number from a oid sub id
 * 
 * @param oid_id_str the oid sub id to parse from
 * @return interface number as string
 */
static sds parse_port_number_from_oid_id(sds oid_id_str)
{
    sds* token_str_arr;
    int count;

    /// If MAC-Address segment doesn't contain leading zero, the zero will get added.
    token_str_arr = sdssplitlen(oid_id_str, sdslen(oid_id_str), ".", 1, &count);   
    sdsclear(oid_id_str);

    if(count == 3)
    {
        oid_id_str = sdscatfmt(oid_id_str, "%s", token_str_arr[1]);
    }

    sdsfreesplitres(token_str_arr, count);

    return oid_id_str;
}

/**
 * @brief returns a remote port by the local interface number
 * 
 * @param remote_ports_list list of remote ports
 * @param local_interface_id local interface number to search for
 * @param remote_port found remote port, needs to be malloced before
 * @return true, if remote port was found
 * @return false, if remote port wasn't found
 */
static bool snmp_get_remote_port_from_list(gll_t* remote_ports_list, int local_interface_id, database_port_t* remote_port)
{

    gll_node_t *current = remote_ports_list->first;
    for(int i = 0; i < remote_ports_list->size; i++)
    {
        database_port_t *port = (database_port_t*)current->data;

        if(port->interface_id == local_interface_id)
        {
            remote_port->mac_address = sdscat(remote_port->mac_address, port->mac_address);
            return true;
        }

        current = current->next;
    }

   return false; 
}

/**
 * @brief fixes format of mac address from a string mac address
 * 
 * @param mac_address the mac address to fix
 * @return fixed mac address
 */
static sds snmp_fix_mac_address(sds mac_address)
{
    sds* token_str_arr;
    int count;

    sdstoupper(mac_address);

    /// If MAC-Address segment doesn't contain leading zero, the zero will get added.
    token_str_arr = sdssplitlen(mac_address, sdslen(mac_address), ":", 1, &count);   
    sdsclear(mac_address);

    for (int i = 0; i < count-1; i++)
    {
        if(sdslen(token_str_arr[i]) == 1)
            mac_address = sdscatfmt(mac_address, "0%S:", token_str_arr[i]);
        else
            mac_address = sdscatfmt(mac_address, "%S:", token_str_arr[i]);
    }

    if(sdslen(token_str_arr[count-1]) == 1)
        mac_address = sdscatfmt(mac_address, "0%S", token_str_arr[count-1]);
    else
        mac_address = sdscatfmt(mac_address, "%S", token_str_arr[count-1]);

    sdsfreesplitres(token_str_arr, count);

    return mac_address;
}

/**
 * @brief parses the given host data pair and saves it into database
 * 
 * @param host_data_pair the host data pair to parse
 * @param database the database were the data gets saved
 */
void snmp_host_data_pair_to_database(host_data_pair_t *host_data_pair, sqlite3 *database)
{
    sds host_ip_str = sdsdup(str_from_ipv4(host_data_pair->host));

    database_device_t *device = malloc(sizeof(database_device_t));
    device->id = -1;
    device->management_address = host_data_pair->host;
    device->capabilities_supported = -1;
    device->capabilities_enabled = -1;
    device->system_name = sdsnew("Unknown");

    gll_t *ports_list = gll_init();
    gll_t *remote_ports_list = gll_init();


    gll_t *oid_string_tuple_list = host_data_pair->oid_string_tuple_list;
    gll_node_t *current = oid_string_tuple_list->first;
    for(int i = 0; i < oid_string_tuple_list->size; i++)
    {
        oid_string_tuple_t *data = (oid_string_tuple_t*)current->data;

        /// Parse LLDPMIB_lldpLocSysCapSupported
        if(STR_EQUAL(data->oid_str_ptr, LLDPMIB_lldpLocSysCapSupported))
        {
            if(STR_EQUAL(data->data_type_str_ptr, "Hex-STRING"))
            {
                /// only read first hex byte, because second one isn't needed
                device->capabilities_supported = (int)strtol(data->data_str_ptr, NULL, 16);
            }
            else if(STR_EQUAL(data->data_type_str_ptr, "STRING"))
            {
                /// convert char to byte
                device->capabilities_supported = (uint8_t)data->data_str_ptr[0];
            }
            else
            {
                printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL,host_ip_str , data->oid_str_ptr, data->data_type_str_ptr);
            }
        }

        /// Parse LLDPMIB_lldpLocSysCapEnabled
        if(STR_EQUAL(data->oid_str_ptr, LLDPMIB_lldpLocSysCapEnabled))
        {
            if(STR_EQUAL(data->data_type_str_ptr, "Hex-STRING"))
            {
                /// only read first hex byte, because second one isn't needed
                device->capabilities_enabled = (int)strtol(data->data_str_ptr, NULL, 16);
            }
            else if(STR_EQUAL(data->data_type_str_ptr, "STRING"))
            {
                /// convert char to byte
                device->capabilities_enabled= (uint8_t)data->data_str_ptr[0];
            }
            else
            {
                printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, data->oid_str_ptr, data->data_type_str_ptr);
            }
        }

        /// Parse LLDPMIB_lldpLocSysName
        if(STR_EQUAL(data->oid_str_ptr, LLDPMIB_lldpLocSysName))
        {
            if(STR_EQUAL(data->data_type_str_ptr, "STRING"))
            {
                sdsfree(device->system_name);
                device->system_name = sdsnew(data->data_str_ptr);
            }
            else
            {
                printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, data->oid_str_ptr, data->data_type_str_ptr);
            }
        }

        if(STR_EQUAL(data->oid_str_ptr, IFMIB_ifIndex))
        {
            database_port_t *port = (database_port_t*)malloc(sizeof(database_port_t));
            port->device_id = -1;
            port->interface_id = -1;
            port->mac_address = sdsempty();
            port->max_speed = 0;
            port->operating_status = -1;
            port->name = sdsempty();

            int if_index = -1;
            if(STR_EQUAL(data->data_type_str_ptr, "INTEGER"))
            {
                if_index = strtol(data->data_str_ptr, NULL, 10);
            }
            else
            {
                printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, IFMIB_ifIndex, data->data_type_str_ptr);
            }

            if(if_index != -1)
            {
                port->interface_id = if_index;
                sds if_index_str = sdscatfmt(sdsempty(), "%i", if_index);

                oid_string_tuple_t address_tuple;
                if(snmp_get_oid_string_tuple(IFMIB_ifPhysAddress, if_index_str, oid_string_tuple_list, &address_tuple))
                {
                    if(STR_EQUAL(address_tuple.data_type_str_ptr, "STRING"))
                    {
                        if(sdslen(address_tuple.data_str_ptr) > 0)
                        {
                            sds mac_address = sdsdup(address_tuple.data_str_ptr);
                            sdsfree(port->mac_address);
                            port->mac_address = snmp_fix_mac_address(mac_address);
                        }
                        else
                        {
                            sdsfree(port->mac_address);
                            port->mac_address = sdsdup(address_tuple.data_str_ptr);
                        }
                    }
                    else
                    {
                        printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, IFMIB_ifPhysAddress, address_tuple.data_type_str_ptr);
                    }
                }
                else
                {
                    printf(KYELLOW"[WARNING][%s] Couldn't find %s.%s - Not found\n"KNORMAL, host_ip_str, IFMIB_ifPhysAddress, if_index_str);
                }

                sdsfree(address_tuple.oid_str_ptr);
                sdsfree(address_tuple.oid_id_str_ptr);
                sdsfree(address_tuple.data_type_str_ptr);
                sdsfree(address_tuple.data_str_ptr);

                oid_string_tuple_t oper_tuple;
                if(snmp_get_oid_string_tuple(IFMIB_ifOperStatus, if_index_str, oid_string_tuple_list, &oper_tuple))
                {
                    if(STR_EQUAL(oper_tuple.data_type_str_ptr, "INTEGER"))
                    {
                        port->operating_status = strtol(oper_tuple.data_str_ptr, NULL, 10);
                    }
                    else
                    {
                        printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, IFMIB_ifOperStatus, oper_tuple.data_type_str_ptr);
                    }
                }
                else
                {
                    printf(KYELLOW"[WARNING][%s] Couldn't find %s.%s - Not found\n"KNORMAL, host_ip_str, IFMIB_ifOperStatus, if_index_str);
                }

                sdsfree(oper_tuple.oid_str_ptr);
                sdsfree(oper_tuple.oid_id_str_ptr);
                sdsfree(oper_tuple.data_type_str_ptr);
                sdsfree(oper_tuple.data_str_ptr);

                oid_string_tuple_t speed_tuple;
                if(snmp_get_oid_string_tuple(IFMIB_ifSpeed, if_index_str, oid_string_tuple_list, &speed_tuple))
                {
                    if(STR_EQUAL(speed_tuple.data_type_str_ptr, "Gauge32"))
                    {
                        port->max_speed = strtol(speed_tuple.data_str_ptr, NULL, 10);
                    }
                    else
                    {
                        printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, IFMIB_ifSpeed, speed_tuple.data_type_str_ptr);
                    }
                }
                else
                {
                    printf(KYELLOW"[WARNING][%s] Couldn't find %s.%s - Not found\n"KNORMAL, host_ip_str, IFMIB_ifSpeed, if_index_str);
                }

                sdsfree(speed_tuple.oid_str_ptr);
                sdsfree(speed_tuple.oid_id_str_ptr);
                sdsfree(speed_tuple.data_type_str_ptr);
                sdsfree(speed_tuple.data_str_ptr);

                oid_string_tuple_t name_tuple;
                if(snmp_get_oid_string_tuple(IFMIB_ifName, if_index_str, oid_string_tuple_list, &name_tuple))
                {
                    if(STR_EQUAL(name_tuple.data_type_str_ptr, "STRING"))
                    {
                        sdsfree(port->name);
                        port->name = sdsdup(name_tuple.data_str_ptr);
                    }
                    else
                    {
                        printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, IFMIB_ifName, name_tuple.data_type_str_ptr);
                    }
                }
                else
                {
                    printf(KYELLOW"[WARNING][%s] Couldn't find %s.%s - Not found\n"KNORMAL, host_ip_str, IFMIB_ifName, if_index_str);
                }

                sdsfree(name_tuple.oid_str_ptr);
                sdsfree(name_tuple.oid_id_str_ptr);
                sdsfree(name_tuple.data_type_str_ptr);
                sdsfree(name_tuple.data_str_ptr);

                sdsfree(if_index_str);
            }

            if(sdslen(port->mac_address) > 0)
            {
                gll_pushBack(ports_list, port);
                
            }
        }

        if(STR_EQUAL(data->oid_str_ptr, LLDPMIB_lldpRemChassisIdSubtype))
        {
            if(STR_EQUAL(data->data_type_str_ptr, "INTEGER"))
            {
                /*
                 * chassisComponent (1),	 
                 * interfaceAlias   (2),	 
                 * portComponent    (3),	 
                 * macAddress       (4),	 
                 * networkAddress   (5),	 
                 * interfaceName    (6),	 
                 * local            (7)	 
                 */

                /// Parsing MAC-Address
                if(STR_EQUAL(data->data_str_ptr, "4"))
                {
                    database_port_t *remote_port = (database_port_t*)malloc(sizeof(database_port_t));
                    remote_port->device_id = -1;
                    remote_port->interface_id = -1;
                    remote_port->mac_address = sdsempty();
                    remote_port->max_speed = 0;
                    remote_port->operating_status = -1;
                    remote_port->name = sdsempty();

                    oid_string_tuple_t chassis_id_tuple;
                    if(snmp_get_oid_string_tuple(LLDPMIB_lldpRemChassisId, data->oid_id_str_ptr, oid_string_tuple_list, &chassis_id_tuple))
                    {
                        if(STR_EQUAL(chassis_id_tuple.data_type_str_ptr, "STRING"))
                        {
                            sds mac_address = sdsdup(chassis_id_tuple.data_str_ptr);
                            sdsfree(remote_port->mac_address);
                            remote_port->mac_address = snmp_fix_mac_address(mac_address);
                            /// save local interface id:
                            remote_port->interface_id = strtol(parse_port_number_from_oid_id( data->oid_id_str_ptr), NULL, 10);
                        }
                        else if(STR_EQUAL(chassis_id_tuple.data_type_str_ptr, "Hex-STRING"))
                        {
                            sds mac_address = sdsdup(chassis_id_tuple.data_str_ptr);
                            sdsfree(remote_port->mac_address);
                            remote_port->mac_address = snmp_fix_hex_mac_address(mac_address);
                            /// save local interface id:
                            remote_port->interface_id = strtol(parse_port_number_from_oid_id(data->oid_id_str_ptr), NULL, 10);
                        }
                        else
                        {
                            printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, LLDPMIB_lldpRemChassisId, chassis_id_tuple.data_type_str_ptr);
                        }
                    }
                    else
                    {
                        printf(KYELLOW"[WARNING][%s] Couldn't find %s.%s - Not found\n"KNORMAL, host_ip_str, LLDPMIB_lldpRemChassisId, data->oid_id_str_ptr);
                    }

                    if(remote_port->interface_id != -1)
                        gll_pushBack(remote_ports_list, remote_port);
                    
                    sdsfree(chassis_id_tuple.oid_str_ptr);
                    sdsfree(chassis_id_tuple.oid_id_str_ptr);
                    sdsfree(chassis_id_tuple.data_type_str_ptr);
                    sdsfree(chassis_id_tuple.data_str_ptr);
                }
                else
                {
                    printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, LLDPMIB_lldpRemChassisIdSubtype, data->data_str_ptr);
                }
            }
            else
            {
                printf(KYELLOW"[WARNING][%s] Couldn't parse %s of type %s - Not Implemented\n"KNORMAL, host_ip_str, LLDPMIB_lldpRemChassisIdSubtype, data->data_type_str_ptr);
            }
        }
            
        current = current->next;
    }

    /// Saving parsed data to database
    if(database_does_device_exist(database, device))
        database_update_device_by_management_address(database, device);
    else
        database_insert_device(database, device);

    
    current = ports_list->first;
    for(int i = 0; i < ports_list->size; i++)
    {
        database_port_t *port = (database_port_t*)current->data;

        database_port_t *real_remote_port = (database_port_t *)malloc(sizeof(database_port_t));
        real_remote_port->id = -1;
        real_remote_port->device_id = -1;
        real_remote_port->interface_id = -1;
        real_remote_port->mac_address = sdsempty();
        real_remote_port->max_speed = 0;
        real_remote_port->name = sdsempty();
        real_remote_port->operating_status = -1;

        database_link_t *link = (database_link_t*)malloc(sizeof(database_link_t));
        link->id = -1;
        link->length = 0;
        link->link_type = 0;
        link->port_a_id = port->id;
        link->port_b_id = -1;
        link->speed = 0;

        port->device_id = device->id;

        if(database_does_port_exist(database, port))
            database_update_port_by_mac_address(database, port);
        else
            database_insert_port(database, port);

        if(snmp_get_remote_port_from_list(remote_ports_list, port->interface_id, real_remote_port))
        {
            // searches for remote port and updates it's data
            if(database_does_port_exist(database, real_remote_port))
            {
                if(real_remote_port->max_speed > port->max_speed)
                    link->speed = port->max_speed;
                else
                    link->speed = real_remote_port->max_speed;

                link->port_b_id = real_remote_port->id;

                
                
                if(database_does_link_exist(database, link))
                    database_update_link_by_port_ids(database, link);
                else
                    database_insert_links(database, link);
            }
            else
            {
                if(database_does_link_exist(database, link))
                    database_delete_link(database, link);
            }
        }
        else
        {
            if(database_does_link_exist(database, link))
                database_delete_link(database, link);
        }

        database_free_link(link);
        database_free_port(port);
        database_free_port(real_remote_port);

        current = current->next;
    }

    current = remote_ports_list->first;
    for(int i = 0; i < remote_ports_list->size; i++)
    {
        database_port_t *port = (database_port_t *)current->data;
        database_free_port(port);
        current = current->next;
    }

    gll_destroy(remote_ports_list);
    gll_destroy(ports_list);

    database_free_device(device);

    sdsfree(host_ip_str);
}