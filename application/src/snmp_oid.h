#ifndef SNMP_OID_H
#define SNMP_OID_H

#include "lib/gll.h"

#define LLDPMIB_lldpLoc ".1.0.8802.1.1.2.1.3"
#define LLDPMIB_lldpLocSysName ".1.0.8802.1.1.2.1.3.3"
#define LLDPMIB_lldpLocSysCapSupported ".1.0.8802.1.1.2.1.3.5"
#define LLDPMIB_lldpLocSysCapEnabled ".1.0.8802.1.1.2.1.3.6"
//#define LLDPMIB_lldpLocPortNum ".1.0.8802.1.1.2.1.3.7.1.1"
#define LLDPMIB_lldpLocPortIdSubtype ".1.0.8802.1.1.2.1.3.7.1.2"
#define LLDPMIB_lldpLocPortId ".1.0.8802.1.1.2.1.3.7.1.3"

#define IFMIB_if ".1.3.6.1.2.1"
#define IFMIB_ifIndex ".1.3.6.1.2.1.2.2.1.1"
#define IFMIB_ifType ".1.3.6.1.2.1.2.2.1.3"
#define IFMIB_ifSpeed ".1.3.6.1.2.1.2.2.1.5"
#define IFMIB_ifPhysAddress ".1.3.6.1.2.1.2.2.1.6"
#define IFMIB_ifOperStatus ".1.3.6.1.2.1.2.2.1.8"
#define IFMIB_ifName ".1.3.6.1.2.1.31.1.1.1.1"

#define LLDPMIB_lldpRem ".1.0.8802.1.1.2.1.4"
//#define LLDPMIB_lldpRemLocalPortNum ".1.0.8802.1.1.2.1.4.1.1.2"
//#define LLDPMIB_lldpRemManAddrSubtype ".1.0.8802.1.1.2.1.4.2.1.1"
//#define LLDPMIB_lldpRemManAddr ".1.0.8802.1.1.2.1.4.2.1.2"
#define LLDPMIB_lldpRemChassisIdSubtype ".1.0.8802.1.1.2.1.4.1.1.4"
#define LLDPMIB_lldpRemChassisId ".1.0.8802.1.1.2.1.4.1.1.5"
#define LLDPMIB_lldpRemPortIdSubtype ".1.0.8802.1.1.2.1.4.1.1.6"
#define LLDPMIB_lldpRemPortId ".1.0.8802.1.1.2.1.4.1.1.7"
#define LLDPMIB_lldpRemSysName ".1.0.8802.1.1.2.1.4.1.1.9"
#define LLDPMIB_lldpRemSysCapSupported ".1.0.8802.1.1.2.1.4.1.1.11"
#define LLDPMIB_lldpRemSysCapEnabled ".1.0.8802.1.1.2.1.4.1.1.12"

void snmp_oid_generate_oid_lists(void);
void snmp_oid_free_oid_lists(void);

gll_t** snmp_oid_get_oid_init_list(void);
gll_t** snmp_oid_get_oid_periodic_list(void);

#endif