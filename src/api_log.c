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
    
    for(size_t i = 0; i < (sizeof(hm->headers) / sizeof(struct mg_http_header)); i++){
        if(CF_ON){
            if(mg_match(hm->headers[i].name, mg_str("CF-Connecting-IP"), NULL)){
                struct mg_str s = hm->headers[i].value; 
                for(size_t j = 0; j < s.len; j++){
                    ip[j] = s.buf[j];
                }
            }
        }
        if(mg_strcasecmp(hm->headers[i].name, mg_str("User-Agent")) == 0){
            user_agent_len = hm->headers[i].value.len;
            if(user_agent_len > max_user_agent_length) user_agent_len = max_user_agent_length;
            user_agent = malloc(user_agent_len+1);
            if(user_agent == NULL) goto end_user_agent_block;
            
            memcpy(user_agent, hm->headers[i].value.buf, user_agent_len);
            user_agent[user_agent_len] = 0;
            end_user_agent_block:
        } else if(mg_strcasecmp(hm->headers[i].name, mg_str("Referer")) == 0){
            referer_len = hm->headers[i].value.len;
            if(referer_len > max_referer_length) referer_len = max_referer_length;
            user_agent = malloc(referer_len+1);
            if(referer == NULL) goto end_referer_block;
            
            memcpy(referer, hm->headers[i].value.buf, referer_len);
            user_agent[referer_len] = 0;
            end_referer_block:
        }
    }
    // printf("No CF-IP found for logging... using null\n");
    // strcpy(ip, "null");
    if(!CF_ON) {
        connection_to_ip_str(c, ip);
    }

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