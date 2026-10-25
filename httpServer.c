#include <stdio.h>
#include <signal.h>
#include "sqlite3.h"
#include "mongoose.h"
#include "main.h"

static void ev_handler(struct mg_connection* c, int ev, void *ev_data);
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

    static char query_ip_SQL[] = "SELECT country, state, city, latitude, longitude FROM ip WHERE ? BETWEEN ip_start AND ip_end LIMIT 1;";
    rc = sqlite3_prepare_v2(db, query_ip_SQL, sizeof(query_ip_SQL), &query_ip_stmt, NULL);
    if(rc){
        printf("Failed to prepare the SQL statement that searches IP's: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    mg_log_set(MG_LL_VERBOSE);
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
    
    for(;;){
        if(close_program_soon) exit(0);
        mg_mgr_poll(&mgr, 1000);
    }

    sqlite3_close(db);

    return 0;
}

static void ev_handler(struct mg_connection* c, int ev, void *ev_data){
    if(ev == MG_EV_HTTP_MSG){
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        if(mg_match(hm->uri, mg_str("/api/hello"), NULL)){
            mg_http_reply(c, 200, "", "{%m:%d}", MG_ESC("status"), 1);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = ".", .fs = &mg_fs_posix};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}
static void signal_handler(int sig){
    printf("recived signal: %d\n", sig);
    sqlite3_close(db);
    close_program_soon = 1;
}