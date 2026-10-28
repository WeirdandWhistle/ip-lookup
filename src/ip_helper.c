#include "ip_helper.h"
#include <stdio.h>
#include <string.h>

// must be null terminated!
long ip_to_long(char* arr){
    // Source - https://stackoverflow.com/a/1684635
    // Posted by Liran Orevi, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-10-24, License - CC BY-SA 2.5

    //No checking of the input    
    unsigned int c1,c2,c3,c4;
    int rc = sscanf(arr, "%u.%u.%u.%u",&c1,&c2,&c3,&c4);
    // must match all four to be valid
    if(rc != 4) return -1;

    unsigned long ip = (unsigned long) (c1 << 24) | (c2 << 16) | (c3 << 8) | (c4);
    return ip;
}

// out must have at least 40 bytes (ipv6 + null bytes)
void connection_to_ip_str(struct mg_connection* c, char* out){
    uint8_t* p = c->rem.addr.ip;
    if(c->rem.is_ip6){
        mg_snprintf(out, 40, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15]);
    } else {
        mg_snprintf(out, 40, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    }    
}