#ifndef MAIN_H
#define MAIN_H

#define DATABASE_FILE "database.db"
#define API_LOG_FILE "api_logs.csv" // API logs meaning a csv of every visit to the API with stuff like (user-aganet, ip, return_code, path, etc...)
// dont define CF_ON to turn it off
#define CF_ON // is behind cloudflare. just change where to look for ip in logs
#define CORS "Access-Control-Allow-Origin: *\r\n"
#define CONTENT_JSON "Content-Type: application/json\r\n"
#define SERVER "Server: whynojava RPI w2 zero\r\n"
#define GLOBAL_HEADERS SERVER CORS CONTENT_JSON 

// #define CACHE_CONTROL "Cache-Control:public, max-age=3600\r\n"
#define CACHE_CONTROL "Cache-Control:none, max-age=0\r\n"

#endif