#include "api_log.h"
#include "mongoose.h"
#include "ip_helper.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

// csv file format:
// ip,response_time,response_code,path,timestamp(unix),user_agent,referer

void log_connection(int rc, double response_time, struct mg_connection* c, struct mg_http_message* hm){
    
    static int max_user_agent_length = 127;
    static int max_referer_length = 127;

    char ip[46] = {0};
    char* user_agent = NULL;
    int user_agent_len = 0;
    char* referer = NULL;
    int referer_len = 0;
    
    struct mg_str* user_agent_str =  mg_http_get_header(hm, "User-Agent");
    if(user_agent_str != NULL){
        user_agent_len = user_agent_str->len;
        if(user_agent_len > max_user_agent_length) user_agent_len = max_user_agent_length;
        user_agent = malloc(user_agent_len+1);
        if(user_agent != NULL){
            memcpy(user_agent, user_agent_str->buf, user_agent_len);
            user_agent[user_agent_len] = 0;
        }
    }
    
    
    struct mg_str* referer_str =  mg_http_get_header(hm, "Referer");
    if(referer_str != NULL){
        referer_len = referer_str->len;
        if(referer_len > max_referer_length) referer_len = max_referer_length;
        referer = malloc(referer_len+1);
        if(referer != NULL){
            memcpy(referer, referer_str->buf, referer_len);
            user_agent[referer_len] = 0;
        }
    }
    // printf("No CF-IP found for logging... using null\n");
    // strcpy(ip, "null");
    #ifdef CF_ON
        struct mg_str* s = mg_http_get_header(hm, "CF-Connecting-IP");
        if(s != NULL){
            int cp_len = s->len > (sizeof(ip)-1) ? sizeof(ip) - 1 : s->len;
            memcpy(ip, s->buf, cp_len);
        } else {
            connection_to_ip_str(c, ip);
        }
    #else
        // printf("Not logging via cloudflare!\n");
        connection_to_ip_str(c, ip);
    #endif

    int path_len = hm->uri.len;
    char* path = malloc(path_len);    
    if(path != NULL){
        memcpy(path, hm->uri.buf, path_len);
        path[path_len] = 0;
    } else {
        path_len = 0;
    }

    long timestamp = time(NULL);

    FILE* file = fopen(API_LOG_FILE, "a");
     
    //  ip,response_time,response_code,path,timestamp(unix),user_agent
    fprintf(file, "%s,%f,%d,%s,%ld,%s,%s,\n", ip, response_time, rc, path, timestamp, user_agent == NULL ? "" : user_agent, referer == NULL ? "" : referer);

    fclose(file);

    free(referer);
    free(user_agent);
    free(path);
}