#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include "lib/sds.h"
#include "lib/gll.h"
#include "stdint.h"

typedef struct
{
    uint32_t port_num;
    uint32_t port_id_subtype;
    sds* port_id;
} lldp_loc_list_element_t;

typedef struct
{
    sds* sys_name;
    uint32_t sys_cap_supported;
    uint32_t sys_cap_enabled;
    // List with lldp_loc_list_element_t
    gll_t* lldp_loc_list;
} lldp_loc_t;

typedef struct
{
    uint32_t index;
    uint32_t type;
    uint32_t speed;
    sds* phys_address;
    uint32_t oper_status;
    //sds* name;
} if_list_element_t;

typedef struct
{
    // List with lldp_loc_list_element_t
    gll_t* if_list;
} if_t;

typedef struct
{
    uint32_t man_addr_subtype;
    sds* man_addr;
    uint32_t chassis_id_subtype;
    sds* chassis_id;
    uint32_t port_id_subtype;
    sds* port_id;
    sds* sys_name;
    uint32_t sys_cap_supported;
    uint32_t sys_cap_enabled;
} lldp_rem_list_elem_t;

typedef struct
{
    // List with lldp_rem_list_elem_t
    gll_t* lldp_rem_list;
} lldp_rem_t;

typedef struct
{
    lldp_loc_t* lldp_local_data;
    if_t* interface_data;
    lldp_rem_t* lldp_remote_data;
} device_info_t;


#endif