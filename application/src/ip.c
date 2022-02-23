#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "ip.h"

/**
 * @brief print IPv4
 * 
 * The function prints IPv4 as a string and a unsigned int. Designed to be used with with gll_each.
 * 
 * @param ip The IPv4 as a void*
 */
void print_ipv4(void* ip)
{
    sds ip_str = str_from_ipv4(*((ipv4_t *) ip));
    printf("IPv4: %s - %u\n", ip_str, *((ipv4_t *) ip));
    sdsfree(ip_str);
}

/**
 * @brief Allocates IPv4
 * 
 * The functions allocates a IPv4 and returns it as a IPv4 pointer.
 * 
 * @param ip The IPv4 which will be allocated.
 * @return a pointer to a allocated IPv4.
 */
ipv4_t* malloc_ipv4(ipv4_t ip)
{
    ipv4_t* ip_ptr = (ipv4_t*)malloc(sizeof(ipv4_t));
    *ip_ptr = ip;
    return ip_ptr;
}

/**
 * @brief Free IPv4
 * 
 * The functions frees a IPv4 pointer and returns the IPv4.
 * 
 * @param ip_ptr The IPv4 pointer to be freed.
 * @return IPv4 which has been freed.
 */
ipv4_t free_ipv4(ipv4_t* ip_ptr)
{
    ipv4_t ip = *ip_ptr;
    free(ip_ptr);
    return ip;
}

/**
 * @brief Free IPv4
 * 
 * The functions frees a IPv4 pointer. It's designed to be used with gll_each.
 * 
 * @param ip_ptr The IPv4 pointer to be freed.
 */
void free_ipv4_void(void* ip_ptr)
{
    free((ipv4_t*) ip_ptr);
}

/**
 * @brief IPv4 Hexadecimal String to IPv4
 * 
 * Converts a IPv4 Hexadecimal string to a IPv4.
 * 
 * @param ip_hex_str The IPv4 hexadecimal string to be converted
 * @return The converted IPv4
 */
ipv4_t ipv4_from_hex_str(sds* ip_hex_str)
{
    ipv4_t ip;
    sds hex_without_spaces_str;

    // Found space in hex ip string
    if (strstr(*ip_hex_str, " ") != NULL)
    {
        hex_without_spaces_str = sdsempty();

        sds *tokens;
        int count, i;

        tokens = sdssplitlen(*ip_hex_str, sdslen(*ip_hex_str), " ", 1, &count);

        for (i = 0; i < count; i++)
        {
            hex_without_spaces_str = sdscatsds(hex_without_spaces_str, tokens[i]);
        }
        sdsfreesplitres(tokens,count);
    }
    else
    {
        hex_without_spaces_str = sdsnew(*ip_hex_str);
    }

    // Found "0x in string"
    if (hex_without_spaces_str[0] == '0' && hex_without_spaces_str[1] == '1')
    {
        ip = (int)strtol(hex_without_spaces_str, NULL, 0);
    }
    else
    {
        ip = (int)strtol(hex_without_spaces_str, NULL, 16);
    }  

    sdsfree(hex_without_spaces_str);

    return ip;
}

/**
 * @brief Converts IPv4 string to IPv4
 * 
 * This converts a IPv4 string to a IPv4
 * 
 * @param ip_str The IPv4 string to convert.
 * @return The converted IPv4
 */
ipv4_t ipv4_from_str(sds* ip_str)
{
    ipv4_t ip = 0;

    sds *tokens;
    int count;

    tokens = sdssplitlen(*ip_str, sdslen(*ip_str), ".", 1, &count);

    ip += ((int)strtol(tokens[3], NULL, 10) << 0);
    ip += ((int)strtol(tokens[2], NULL, 10) << 8);
    ip += ((int)strtol(tokens[1], NULL, 10) << 16);
    ip += ((int)strtol(tokens[0], NULL, 10) << 24);

    sdsfreesplitres(tokens,count);

    return ip;
}

/**
 * @brief Converts IPv4 to IPv4 string.
 * 
 * This converts a IPv4 to a IPv4 string.S
 * 
 * @param ipv4 The IPv4 to convert.
 * @return The converted IPv4 string.
 */
sds str_from_ipv4(ipv4_t ipv4)
{
    uint8_t ip[4] = {0};
    ip[0] = (char)((ipv4 >>  0) & 0xFF);
    ip[1] = (char)((ipv4 >>  8) & 0xFF);
    ip[2] = (char)((ipv4 >> 16) & 0xFF);
    ip[3] = (char)((ipv4 >> 24) & 0xFF);

    sds ip_str;
    ip_str = sdscatfmt(sdsempty(), "%u.%u.%u.%u", ip[3], ip[2], ip[1], ip[0]);
    ip_str = sdsRemoveFreeSpace(ip_str);

    return ip_str;
}