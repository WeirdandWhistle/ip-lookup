#include <stdio.h>
#include <signal.h>
#include "sqlite3.h"
#include "mongoose.h"
#include "buildDB.h"
#include "main.h"
#include <time.h>

static void ev_handler(struct mg_connection* c, int ev, void *ev_data);
static void handler_ip_lookup(struct mg_connection* c, struct mg_http_message* hm);

static int lookup_ip_json(struct mg_str ip, struct mg_str* json);
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
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        if(mg_match(hm->uri, mg_str("/api/hello"), NULL)){
            mg_http_reply(c, 200, "", "{%m:%d}", MG_ESC("status"), 1);
        } else if(mg_match(hm->uri, mg_str("/ip#"), NULL)){
            handler_ip_lookup(c, hm);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = ".", .fs = &mg_fs_posix};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}
double get_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
}
static void handler_ip_lookup(struct mg_connection* c, struct mg_http_message* hm){
    struct mg_str caps[2];
    int rc = 0;
    if(mg_match(hm->uri, mg_str("/ip/*"), caps)){
        struct mg_str json;
        json.len = 1024;
        json.buf = malloc(json.len);
        if(json.buf == NULL){
            printf("Malloc failed for setting a string buffer.\n");
            mg_http_reply(c, 500, GLOBAL_HEADERS, "{\"ok\":false,\"error\":\"Malloc failed.\"}");
            return;
        }
        // double start_ts = get_time_ms();
        rc = lookup_ip_json(caps[0], &json);
        // printf("cost: %f\n", get_time_ms() - start_ts);

        if(rc){
            switch(rc){
                case 500:
                    mg_snprintf(json.buf, json.len, "SQLite debug error: %d.", sqlite3_errmsg(db));
                    break;
                case 424:
                    mg_snprintf(json.buf, json.len, "Request proccess but no data was found... SQlite failed me.");
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
            return;
        }

        mg_http_reply(c, 200, GLOBAL_HEADERS CACHE_CONTROL, "{\"ok\":true,\"data\":%s}", json.buf);

        free(json.buf);
        return;
    }

    mg_http_reply(c, 404, GLOBAL_HEADERS, "{\"ok\":false,\"error\":\"Not Found.\"}");
}
static int lookup_ip_json(struct mg_str ip, struct mg_str* json){
    int return_code = 0;
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
            return_code = 424;
            goto cleanup;
        }
        printf("SQLite steping did not return a row on the first run (it should): %s\n", sqlite3_errmsg(db));
        return_code = 1;
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