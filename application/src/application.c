#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "lib/gll.h"
#include "lib/sds.h"

#include "ip.h"
#include "debug.h"
#include "kcolor.h"
#include "snmp_network.h"
#include "snmp_trap.h"
#include "network_tree_nodes.h"
#include "database.h"

#include "snmp_oid.h"
#include "snmp_parse.h"

static sds exec_path_str;
static sds appl_name_str;
static sqlite3 *database = NULL;

/**
 * @brief Get the exec path and application name
 * 
 * Parses the path and the application name and saves them in static variables.
 * 
 * Example:
 * Input: /usr/bin/application
 * exec_path = /usr/bin/
 * appl_name = application
 * 
 * @param exec_path_argv_str argv[0] as sds string
 */
void get_exec_path_and_appl_name(sds* exec_path_argv_str)
{
    sds* token_str_arr;
    int count;

    token_str_arr = sdssplitlen(*exec_path_argv_str, sdslen(*exec_path_argv_str), "/", 1, &count);

    exec_path_str = sdsempty();

    for (int i = 0; i < count-1; i++)
    {
        exec_path_str = sdscatsds(exec_path_str, token_str_arr[i]);
        exec_path_str = sdscat(exec_path_str, "/");
    }
    exec_path_str = sdsRemoveFreeSpace(exec_path_str);
    
    appl_name_str = sdsnew(token_str_arr[count-1]);
    appl_name_str = sdsRemoveFreeSpace(appl_name_str);

    sdsfreesplitres(token_str_arr, count);
}

/**
 * @brief Cleanup and Exit
 * 
 * Cleanup used memory and exit program with given exit_code
 * 
 * @param exit_code code to exit the program
 */
void clean_exit(int exit_code)
{
    snmp_oid_free_oid_lists();

    snmp_trap_wait_for_thread();

    if(database != NULL)
        database_close(database);

    sdsfree(exec_path_str);
    sdsfree(appl_name_str);

    printf("Nothing to do anymore.\nBye o/\n");
    exit(exit_code);
}

static volatile bool run_loop = true;

static void signal_handler(int signo) {
    if(signo == SIGINT || signo == SIGTERM)
    {
        run_loop = false;
    }
}


int main(int argc, char* argv[])
{    
    if(geteuid() != 0)
    {
        printf(KRED"[ERROR] This application needs root privileges.\n"KNORMAL);

        return EXIT_FAILURE;
    }

    if(argc < 3)
    {
        printf(KRED"[ERROR] To few arguments.\n"KNORMAL);
        printf("[NOTICE] Usage: application <host> <community>\n");

        return EXIT_FAILURE;
    }

    sds exec_path_argv_str = sdsnew(argv[0]);
    get_exec_path_and_appl_name(&exec_path_argv_str);
    sdsfree(exec_path_argv_str);

    snmp_oid_generate_oid_lists();

    //TODO: Argument Checking
    sds host_str = sdsnew(argv[1]);
    sds community_str = sdsnew(argv[2]);

    /// Start SNMP network scan and put found devices into snmp_device_list;
    gll_t* snmp_device_list = gll_init();
    printf("Starting Network Scan\n");
    snmp_network_scan_run(&exec_path_str, &host_str, &community_str, &snmp_device_list);
    printf("Finished Network Scan, found %d SNMP devices with community \"%s\".\n", snmp_device_list->size , community_str);

    #ifdef DEBUG
    PRINT_DEBUG("Found Devices:\n");
    gll_each(snmp_device_list, &snmp_network_print_snmp_device_list_debug);
    #endif

    /// If no SNMP device have been found, free allocated memory and exit the programm
    if(gll_first(snmp_device_list) == NULL)
    {
        printf("No SNMP Device with community \"%s\" found.\n", community_str);

        gll_each(snmp_device_list, &free_ipv4_void);
        gll_destroy(snmp_device_list);
        sdsfree(community_str);
        sdsfree(host_str);

        clean_exit(EXIT_SUCCESS);
    }
    sdsfree(host_str);

    /// Initialize Database
    if(database_open("application.db", &database))
        clean_exit(EXIT_FAILURE);
    
    if(database_drop(database))
        clean_exit(EXIT_FAILURE);

    if(database_generate(database))
        clean_exit(EXIT_FAILURE);

    /// Get List for OIDs for init run
    gll_t *oid_init_list;
    oid_init_list = *snmp_oid_get_oid_init_list();

    gll_t* host_data_list;
    host_data_list = gll_init();

    /// Iterate over SNMP device list and get the init data from each device
    gll_node_t* current = snmp_device_list->first;
    while(current != NULL) {
        sds return_data_str = sdsempty();

        ipv4_t *host_ip_ptr;
        host_ip_ptr = (ipv4_t*)current->data;
        snmp_network_walk_batch_run_str(&exec_path_str, &community_str, *host_ip_ptr, &oid_init_list, &return_data_str);

        if(!snmp_parse_check_from_list(&return_data_str, &oid_init_list))
        {
            sds host_ip_str = str_from_ipv4(*host_ip_ptr);
            printf(KYELLOW "[WARNING] Not all needed OIDs are implemented on Host \"%s\" with community \"%s\".\n" KNORMAL, host_ip_str, community_str);
            sdsfree(host_ip_str);
        }

        gll_t* oid_string_tuple_list;
        oid_string_tuple_list = gll_init();

        snmp_parse_from_list(&return_data_str,*host_ip_ptr, &oid_init_list, &oid_string_tuple_list);
        host_data_pair_t *host_data_pair;
        host_data_pair = malloc(sizeof(host_data_pair_t));
        host_data_pair->host = *host_ip_ptr;
        host_data_pair->oid_string_tuple_list = oid_string_tuple_list;

        snmp_host_data_pair_to_database(host_data_pair, database);

        gll_push(host_data_list, host_data_pair);

        sdsfree(return_data_str);

        current = current->next;
    } 

    /// Setting up the SNMP Trap daemon
    snmp_trap_daemon_setup(&exec_path_str, &community_str);

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("[ERROR] snmp_trap_daemon_setup couldn't setup signal handling for SIGINT\n");
        return EXIT_FAILURE;
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        printf("[ERROR] snmp_trap_daemon_setup couldn't setup signal handling for SIGTERM\n");
        return EXIT_FAILURE;
    }

    /// Main Loop:
    /// Check if a SNMP Trap has been received, if a trap has been received renew data from connected devices.
    while(run_loop)
    {
        ipv4_t *ip_list = calloc(IP_BUFFER_SIZE, sizeof(ipv4_t));
        int ip_list_pos = 0;
        snmp_trap_read_data(&ip_list, &ip_list_pos);

        /// Update saved data 
        for(int i = 0; i < ip_list_pos; i++)
        {
            sds ip_str = str_from_ipv4(ip_list[i]);
            printf("[NOTICE] Received SNMP trap from %s\n", ip_str);
            sdsfree(ip_str);

            sds return_data_str = sdsempty();

            snmp_network_walk_batch_run_str(&exec_path_str, &community_str, ip_list[i], &oid_init_list, &return_data_str);

            if(!snmp_parse_check_from_list(&return_data_str, &oid_init_list))
            {
                sds host_ip_str = str_from_ipv4(ip_list[i]);
                printf(KYELLOW "[WARNING] Not all needed OIDs are implemented on Host \"%s\" with community \"%s\".\n" KNORMAL, host_ip_str, community_str);
                sdsfree(host_ip_str);
            }

            gll_t* oid_string_tuple_list;
            oid_string_tuple_list = gll_init();

            snmp_parse_from_list(&return_data_str, ip_list[i], &oid_init_list, &oid_string_tuple_list);
            host_data_pair_t *host_data_pair;
            host_data_pair = malloc(sizeof(host_data_pair_t));
            host_data_pair->host = ip_list[i];
            host_data_pair->oid_string_tuple_list = oid_string_tuple_list;

            /// Find node in list
            int found_index = 0;
            gll_node_t* found = NULL;
            gll_node_t* current = snmp_device_list->first;
            while(current != NULL) {
                host_data_pair_t* data_pair = (host_data_pair_t*)current->data;

                if(data_pair->host == ip_list[found_index])
                {
                    found = current;
                    break;
                }

                current = current->next;
                found_index++;
            } 

            if(found == NULL)
            {
                gll_push(host_data_list, host_data_pair);
            }
            else
            {
                gll_remove(host_data_list, found_index);

                host_data_pair_t* data_pair = (host_data_pair_t*)found->data;
                gll_each(data_pair->oid_string_tuple_list, snmp_parse_free_oid_string_tuple_t);
                gll_destroy(data_pair->oid_string_tuple_list);
                free(data_pair);

                gll_push(host_data_list, host_data_pair);
            }

            sdsfree(return_data_str);

        }
        
        free(ip_list);
        usleep(1000);
    }
    
    /// Cleanup
    gll_each(host_data_list, &snmp_parse_free_host_data_pair_t);

    sdsfree(community_str);

    gll_each(snmp_device_list, &free_ipv4_void);
    gll_destroy(snmp_device_list);

    clean_exit(EXIT_SUCCESS);
}