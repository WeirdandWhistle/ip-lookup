#ifndef IP_LOOKUP_IP_HELPER
#define IP_LOOKUP_IP_HELPER

    #include "mongoose.h"

    long ip_to_long(char* arr);
    void connection_to_ip_str(struct mg_connection* c, char* out);

#endif