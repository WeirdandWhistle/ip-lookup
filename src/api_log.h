#ifndef IP_LOOKUP_API_LOG
#define IP_LOOKUP_API_LOG

    #include "mongoose.h"

    void log_connection(int rc, double response_time, struct mg_connection* c, struct mg_http_message* hm);

#endif