#include <stdio.h>
#include <signal.h>
#include "sqlite3.h"
#include "mongoose.h"
#include "buildDB.h"
#include "main.h"
#include "ip_helper.h"
#include "api_log.h"
#include <time.h>

static void ev_handler(struct mg_connection* c, int ev, void *ev_data);
int handler_ip_lookup(struct mg_connection* c, struct mg_http_message* hm);

int lookup_ip_json(struct mg_str ip, struct mg_str* json);
static void signal_handler(int sig);

sqlite3* db;
sqlite3_stmt* query_ip_stmt;

int close_program_soon = 0;

int http_server_init(){
    signal(SIGTERM, signal_handler);

    int rc = sqlite3_open(DATABASE_FILE, &db);
    if(rc){
        printf("SQLite failed to open: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    static char query_ip_SQL[] = "SELECT country, state, city, latitude, longitude FROM ip WHERE ip_start <= ? AND ? <= ip_end ORDER BY ip_start DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(db, query_ip_SQL, sizeof(query_ip_SQL), &query_ip_stmt, NULL);
    if(rc){
        printf("Failed to prepare the SQL statement that searches IP's: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    mg_log_set(MG_LL_DEBUG);
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
    
    for(;;){
        if(close_program_soon) break;
        mg_mgr_poll(&mgr, 1000);
    }

    sqlite3_finalize(query_ip_stmt);
    sqlite3_close(db);

    return 0;
}

static void ev_handler(struct mg_connection* c, int ev, void *ev_data){
    if(ev == MG_EV_HTTP_MSG){
        int rc = -1;
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        clock_t start_time = clock();
        if(mg_match(hm->uri, mg_str("/ip#"), NULL)){
            rc = handler_ip_lookup(c, hm);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = ".", .fs = &mg_fs_posix};
            mg_http_serve_dir(c, hm, &opts);
        }
        if(rc != -1){
            double response_time = (clock() - start_time) / (CLOCKS_PER_SEC / 1000.0);
            log_connection(rc, response_time, c, hm);
        }
    }
}
int handler_ip_lookup(struct mg_connection* c, struct mg_http_message* hm){
    struct mg_str caps[2];
    int rc = 0;
    int has_ip_str = 0;
    struct mg_str ip_str = {0};
    struct mg_str json;
    json.len = 1024;
    json.buf = malloc(json.len);
    if(mg_match(hm->uri, mg_str("/ip/*"), caps)){
        
        if(json.buf == NULL){
            printf("Malloc failed for setting a string buffer.\n");
            mg_http_reply(c, 500, GLOBAL_HEADERS, "{\"ok\":false,\"error\":\"Malloc failed.\"}");
            return 500;
        }
        ip_str = caps[0];
        has_ip_str = 1;      
    } else if(mg_match(hm->uri, mg_str("/ip"), NULL)){
        int found_cf_ip = 0;
        for(size_t i = 0; i < (sizeof(hm->headers) / sizeof(struct mg_http_header)); i++){
            if(mg_match(hm->headers[i].name, mg_str("CF-Connecting-IP"), NULL)){
                found_cf_ip = 1;
                ip_str = hm->headers[i].value;
                break;
            }
        }
        if(!found_cf_ip){
            char gen_ip[18] = {0};
            mg_snprintf(gen_ip, sizeof(gen_ip), "%d.%d.%d.%d", c->rem.addr.ip[0], c->rem.addr.ip[1], c->rem.addr.ip[2], c->rem.addr.ip[3]);
            ip_str = mg_str(gen_ip);
            has_ip_str = 1;
        }
    }

    if(has_ip_str){
        // double start_ts = get_time_ms();
        rc = lookup_ip_json(ip_str, &json);
        // printf("cost: %f\n", get_time_ms() - start_ts);

        if(rc > 299){
            switch(rc){
                case 500:
                    mg_snprintf(json.buf, json.len, "SQLite debug error: %d.", sqlite3_errmsg(db));
                    break;
                case 400:
                    mg_snprintf(json.buf, json.len, "Malformed IP address.");
                    break;
                default:
                    mg_snprintf(json.buf, json.len, "How did you even manage...");
                    break;
            }

            mg_http_reply(c, rc, GLOBAL_HEADERS, "{\"ok\":false,\"error\":%m}", MG_ESC(json.buf));
            free(json.buf);
            return rc;
        }

        mg_http_reply(c, rc, GLOBAL_HEADERS CACHE_CONTROL, "{\"ok\":true,\"data\":%s}", json.buf);
    } else {
        mg_http_reply(c, 404, GLOBAL_HEADERS, "{\"ok\":false,\"error\":\"Not Found.\"}");
    }
    free(json.buf);
    return rc;
}
int lookup_ip_json(struct mg_str ip, struct mg_str* json){
    int return_code = 200;
    int ip_null_len = ip.len+1;
    char* ip_null = malloc(ip_null_len);
    if(ip_null == NULL){
        printf("Malloc failed in lookup_ip_json.\n");
        return_code = 1;
        goto cleanup;
    }
    memset(ip_null, 0, ip_null_len);
    memcpy(ip_null, ip.buf, ip.len);

    long ip_num = ip_to_long(ip_null);
    if(ip_num == -1) {
        return_code = 400;
        goto cleanup;
    }

    int rc = sqlite3_bind_int64(query_ip_stmt, 1, ip_num);
    if(rc != SQLITE_OK){
        printf("SQLite binding to searching ip failed: %s\n", sqlite3_errmsg(db));
        return_code = 500;
        goto cleanup;
    }
    rc = sqlite3_bind_int64(query_ip_stmt, 2, ip_num);
    if(rc != SQLITE_OK){
        printf("SQLite binding to searching ip failed: %s\n", sqlite3_errmsg(db));
        return_code = 500;
        goto cleanup;
    }

    rc = sqlite3_step(query_ip_stmt);
    if(rc != SQLITE_ROW){
        if(rc == SQLITE_DONE){
            return_code = 206;
            mg_snprintf(json->buf, json->len, "{\"ip\":%m,\"country\":null,\"state\":null,\"city\":null,\"latitude\":0.0,\"longitude\":0.0}", MG_ESC(ip_null));
            goto cleanup;
        }
        printf("SQLite steping did not return a row on the first run (it should): %s\n", sqlite3_errmsg(db));
        return_code = 500;
        goto cleanup;
    }

    const unsigned char* country = sqlite3_column_text(query_ip_stmt, 0);
    const unsigned char* state = sqlite3_column_text(query_ip_stmt, 1);
    const unsigned char* city = sqlite3_column_text(query_ip_stmt, 2);

    const float latitude = sqlite3_column_double(query_ip_stmt, 3);
    const float longitude = sqlite3_column_double(query_ip_stmt, 4);

    mg_snprintf(json->buf, json->len, "{\"ip\":%m,\"country\":%m,\"state\":%m,\"city\":%m,\"latitude\":%f,\"longitude\":%f}", MG_ESC(ip_null), MG_ESC(country), MG_ESC(state), MG_ESC(city), latitude, longitude);   
    json->len = strlen(json->buf);

    cleanup:
    free(ip_null);
    sqlite3_reset(query_ip_stmt);

    return return_code;
}
static void signal_handler(int sig){
    printf("recived signal: %d\n", sig);
    sqlite3_close(db);
    close_program_soon = 1;
}