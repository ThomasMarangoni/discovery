#include <stdlib.h>

#include "snmp_oid.h"
#include "lib/sds.h"

/// This List contains OID strings for the init and the periodic phase.
static gll_t* oid_init_list;

/// This list contains OID strings for the peridoic phase.
static gll_t* oid_periodic_list;

/**
 * @brief Generates OID lists
 * 
 * Generates two list with OIDs, that are used to perform a SNMP Walk and parse the data.
 */
void snmp_oid_generate_oid_lists(void)
{
    oid_init_list = gll_init();
    oid_periodic_list = gll_init();

    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpLocSysName));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpLocSysCapSupported));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpLocSysCapEnabled));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpLocPortIdSubtype));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpLocPortId));
    gll_push(oid_init_list, sdsnew(IFMIB_ifIndex));
    gll_push(oid_init_list, sdsnew(IFMIB_ifType));
    gll_push(oid_init_list, sdsnew(IFMIB_ifSpeed));
    gll_push(oid_init_list, sdsnew(IFMIB_ifPhysAddress));
    gll_push(oid_init_list, sdsnew(IFMIB_ifOperStatus));
    gll_push(oid_init_list, sdsnew(IFMIB_ifName));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemChassisIdSubtype));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemChassisId));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemPortIdSubtype));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemPortId));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemSysName));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemSysCapSupported));
    gll_push(oid_init_list, sdsnew(LLDPMIB_lldpRemSysCapEnabled));

    gll_push(oid_periodic_list, sdsnew(IFMIB_ifIndex));
    gll_push(oid_periodic_list, sdsnew(IFMIB_ifType));
    gll_push(oid_periodic_list, sdsnew(IFMIB_ifSpeed));
    gll_push(oid_periodic_list, sdsnew(IFMIB_ifPhysAddress));
    gll_push(oid_periodic_list, sdsnew(IFMIB_ifOperStatus));
    gll_push(oid_periodic_list, sdsnew(IFMIB_ifName));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemChassisIdSubtype));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemChassisId));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemPortIdSubtype));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemPortId));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemSysName));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemSysCapSupported));
    gll_push(oid_periodic_list, sdsnew(LLDPMIB_lldpRemSysCapEnabled));
}

void free_sds_list_element(void* sds_ptr)
{
    sds string = (sds)sds_ptr;
    sdsfree(string);
}

/**
 * @brief Cleanup of OID Lists
 * 
 * This frees the memory used by the init and periodic list.
 */
void snmp_oid_free_oid_lists(void)
{
    gll_each(oid_init_list, &free_sds_list_element);
    gll_destroy(oid_init_list);

    gll_each(oid_periodic_list, &free_sds_list_element);
    gll_destroy(oid_periodic_list);
}

/**
 * @brief Get list of OIDs for init phase.
 * 
 * @return List with OIDs for init phase.
 */
gll_t** snmp_oid_get_oid_init_list(void)
{
    return &oid_init_list;
}

/**
 * @brief Get list of OIDs for periodic phase.
 * 
 * @return List with OIDs for init phase.
 */
gll_t** snmp_oid_get_oid_periodic_list(void)
{
    return &oid_periodic_list;
}