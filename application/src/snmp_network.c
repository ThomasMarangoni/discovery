#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "debug.h"
#include "kcolor.h"
#include "snmp_network.h"
#include "network_tree_nodes.h"

#include "ip.h"

/**
 * @brief SNMP network scans
 * 
 * This functions scans the given network for SNMP devices on a specific community.
 * 
 * Limits of network scan
 * Max number of hosts :           65535
 * Max community length:           32
 * Max number of communities:      16384
 * 
 * @param exec_path_str The path fom the main applicatio
 * @param host_str The host or network IPv4 address with the subnet address as a sting (eg. 192.168.0.0/24)
 * @param community_str The SNMP community string to perform the scan on.
 * @param snmp_device_list This returns a list of IP4 devices that have been found.
 * @return Status Code (0 = SUCESS, 1 = FAILURE)
 */
int snmp_network_scan_run(sds* exec_path_str, sds* host_str, sds* community_str, gll_t** snmp_device_list)
{
    int pipefd[2];
    if(pipe(pipefd)== -1)
    {
        printf(KRED "[ERROR] Pipe error\n" KNORMAL);
    }

    pid_t pid = fork();
    

    if (pid == -1)
    {
        // error, failed to fork()
        return EXIT_FAILURE;
    } 
    else if (pid > 0)
    {
        // Parent:
        close(pipefd[PIPE_WRITE_END]);

        int cstatus;

        FILE *stream;
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        stream = fdopen(pipefd[PIPE_READ_END], "r");

        while ((nread = getline(&line, &len, stream)) != -1) 
        {
            sds line_str = sdsnewlen(line, nread);

            line_str = sdstrim(line_str, " \t");

            //parse ipv4 address:
            ipv4_t ip = ipv4_from_str(&line_str);
            ipv4_t *ip_ptr = malloc_ipv4(ip);
            gll_push(*snmp_device_list, ip_ptr);
            

            sdsfree(line_str);
        }

        if(line != NULL)
        {
            free(line);
        }
        fclose(stream);

        close(pipefd[PIPE_READ_END]);

        waitpid(pid, &cstatus, 0);

        return EXIT_SUCCESS;
    }
    else 
    {
        // Child:
        sds binary_path_str = sdsempty();
        binary_path_str = sdscatsds(binary_path_str, *exec_path_str);
        binary_path_str = sdscat(binary_path_str, "external/onesixtyone");
        binary_path_str = sdsRemoveFreeSpace(binary_path_str);

        close(pipefd[PIPE_READ_END]);

        if (dup2(pipefd[PIPE_WRITE_END], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            return EXIT_FAILURE;
        }

        close(pipefd[PIPE_WRITE_END]);

        if(access(binary_path_str, X_OK) != 0)
        {
            printf(KRED "[ERROR] Can't run file: %s" KNORMAL, binary_path_str);
            return EXIT_FAILURE;
        }

        /// String "fix" is needed after -s parameter because onesixtyone needs an argument for it.
        /// But the argument isn't used. -q is used to suppress sysDescr of device.
        execl(binary_path_str, binary_path_str, "-s", "fix", "-q", *host_str, *community_str, NULL);
        return EXIT_FAILURE;
    }
}

/**
 * @brief SNMP Walk for multiple OIDs on a single host
 * 
 * This function makes a snmp walk over multiple oids on a single host and returns the data as a string, which needs to be parsed.
 * 
 * @param exec_path_str The path fom the main application
 * @param community_str The SNMP community string used to make the SNMP walk.
 * @param host_ip The IPv4 address of the host, to make the SNMP walk on.
 * @param oid_list A string list which can contain multiple OIDs to perform the SNMP walk on.
 * @param return_str The output returned from multiple SNMP walks as a string.
 * @return Status Code (0 = SUCESS, 1 = FAILURE)
 */
int snmp_network_walk_batch_run_str(sds* exec_path_str, sds* community_str, ipv4_t host_ip,  gll_t** oid_list, sds* return_str)
{
    gll_t* oid_list_ptr = *oid_list;
    gll_node_t* current = oid_list_ptr->first;

    int status_code;

    while(current != NULL) {
        sds oid_str = (sds) current->data;
        
        status_code = snmp_network_walk_run_str(exec_path_str, community_str, host_ip, &oid_str, return_str);
        if(status_code != EXIT_SUCCESS)
            break;

        current = current->next;
    }

    return status_code;
}

/**
 * @brief SNMP walk for a single OID on a single host
 * 
 * This function makes a snmp walk over a single oid on a single host and returns the data as a string, which needs to be parsed.
 * 
 * @param exec_path_str The path from the main application
 * @param community_str The SNMP community string used to make the SNMP walk.
 * @param host_ip The IPv4 address of the host, to make the SNMP walk on.
 * @param oid_str The oid to start the walk on as a sds sting.
 * @param return_str The output returned from the SNMP walk as a string.
 * @return Status Code (0 = SUCESS, 1 = FAILURE)
 */

int snmp_network_walk_run_str(sds* exec_path_str, sds* community_str, ipv4_t host_ip,  sds* oid_str, sds* return_str)
{
    int pipefd[2];
    if(pipe(pipefd)== -1)
    {
        printf(KRED "[ERROR] Pipe error\n" KNORMAL);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        // error, failed to fork()
        return EXIT_FAILURE;
    } 
    else if (pid > 0)
    {
        // Parent:
        close(pipefd[PIPE_WRITE_END]);

        int cstatus;

        FILE *stream;
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        stream = fdopen(pipefd[PIPE_READ_END], "r");

        while ((nread = getline(&line, &len, stream)) != -1) 
        {
            sds line_str = sdsnewlen(line, nread);
            *return_str = sdscat(*return_str, line_str);       

            sdsfree(line_str);
        }

        if(line != NULL)
        {
            free(line);
        }

        fclose(stream);

        close(pipefd[PIPE_READ_END]);

        waitpid(pid, &cstatus, 0);

        return EXIT_SUCCESS;
    }
    else 
    {
        // Child:
        sds binary_path_str = sdsempty();
        binary_path_str = sdscatsds(binary_path_str, *exec_path_str);
        binary_path_str = sdscat(binary_path_str, "external/snmpwalk");
        binary_path_str = sdsRemoveFreeSpace(binary_path_str);

        close(pipefd[PIPE_READ_END]);

        if (dup2(pipefd[PIPE_WRITE_END], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            return EXIT_FAILURE;
        }

        if (dup2(pipefd[PIPE_WRITE_END], STDERR_FILENO) == -1)
        {
            perror("dup2");
            return EXIT_FAILURE;
        }

        close(pipefd[PIPE_WRITE_END]);

        if(access(binary_path_str, X_OK) != 0)
        {
            printf("[ERROR] Can't run file: %s", binary_path_str);
            return EXIT_FAILURE;
        }

        sds host_ip_str = str_from_ipv4(host_ip);
        execl(binary_path_str, binary_path_str, "-c", *community_str, "-v", "2c", "-One", host_ip_str, *oid_str, NULL);
        return EXIT_FAILURE;
    }
}

#ifdef DEBUG
/**
 * @brief prints snmp device ip from list
 * 
 * This function only works in debug mode, prints the ip address of a snmp_device_list element, designed to be used with gll_each.
 * 
 * @param element 
 */
void snmp_network_print_snmp_device_list_debug(void* element)
{
        ipv4_t *ip;
        ip = (ipv4_t*) element;
        sds ip_str = str_from_ipv4(*ip);
        PRINT_DEBUG("%s\n", ip_str);
        sdsfree(ip_str);
}
#endif