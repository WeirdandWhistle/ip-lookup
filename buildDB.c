#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sqlite3.h"
#include "main.h"

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

int build_db(){
    sqlite3* db;
    // char* zErrorMsg = 0;
    int rc;

    rc = sqlite3_open(DATABASE_FILE, &db);
    if(rc){
        printf("Cant open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    
    // initlize table
    sqlite3_stmt* createTable;
    static char createTableSQL[] = "CREATE TABLE IF NOT EXISTS ip (ip_start INT, ip_end INT, country TEXT, state TEXT, city TEXT, latitude FLOAT, longitude FLOAT);";
    rc = sqlite3_prepare_v2(db, createTableSQL, sizeof(createTableSQL) / sizeof(char), &createTable, NULL);
    if(rc){
        printf("could not prepare the table SQL: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_step(createTable);
    if(rc != SQLITE_DONE){
        printf("could not execute the table SQL: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(createTable);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_finalize(createTable);

    sqlite3_stmt* csv_insert;
    static char csv_insert_SQL[] = "INSERT INTO ip (ip_start, ip_end, country, state, city, latitude, longitude) VALUES (?, ?, ?, ?, ?, ?, ?);";
    rc = sqlite3_prepare_v2(db, csv_insert_SQL, sizeof(csv_insert_SQL), &csv_insert, NULL);
    if(rc){
        printf("could not prepare the insert SQL: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", 0, 0, 0);
    sqlite3_exec(db, "PRAGMA journal_mode = MEMORY;", 0, 0, 0);
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // parse csv
    FILE *fptr = fopen("dbip-city-ipv4.csv", "r");
    if(fptr == NULL){
        sqlite3_close(db);
        printf("Failed to open file: %s\n", strerror(errno));
        return 1;
    }   

    int currentNumberOfRowsWritten = 0;
    static int maxRowsWritten = 5E7;
    static int sizeOfBuffer = 1000;
    char lineBuf[sizeOfBuffer] = {};
    while(fgets(lineBuf, sizeOfBuffer, fptr) != NULL && currentNumberOfRowsWritten < maxRowsWritten){
        currentNumberOfRowsWritten++;
        
        // printf("line #%3.0d: %s", currentNumberOfRowsWritten, lineBuf);
        // parse comma values and then write to DB
        int fullBufLength = strlen(lineBuf);
        for(int i = 0; i<fullBufLength;i++){
            if(lineBuf[i] == ',') lineBuf[i] = 0;
        }

        int offset = 0;
        long ip_start = ip_to_long(lineBuf);
        offset += strlen(lineBuf) + 1;

        long ip_end = ip_to_long(lineBuf+offset);
        offset += strlen(lineBuf+offset) + 1;

        int continent_len = 2;
        char continent[3] = {0};
        strcpy(continent, lineBuf+offset);
        offset += continent_len + 1;
        
        int country_len = strlen(lineBuf+offset);
        char* country = malloc(country_len+1);
        strcpy(country, lineBuf+offset);
        offset += country_len + 1;

        int state_len = strlen(lineBuf+offset);
        char* state = malloc(state_len+1);
        strcpy(state, lineBuf+offset);
        offset += state_len + 1;

        int city_len = strlen(lineBuf+offset);
        char* city = malloc(city_len+1);
        strcpy(city, lineBuf+offset);
        offset += city_len + 1;
        
        offset += 1; // theres just an extra comma in the source CSV

        float latitude = strtof(lineBuf+offset, NULL);
        offset += strlen(lineBuf+offset) + 1;

        float longitude = strtof(lineBuf+offset, NULL);
        offset += strlen(lineBuf+offset) + 1;

        // printf("'%ld' '%ld' '%s' '%s' '%s' '%s' '%f' '%f'\n",ip_start, ip_end, continent, country, state, city, latitude, longitude);

        // put into SQLite
        rc = sqlite3_bind_int64(csv_insert, 1, ip_start); if(rc != SQLITE_OK) goto clean_up_and_report_error;
        rc = sqlite3_bind_int64(csv_insert, 2, ip_end); if(rc != SQLITE_OK) goto clean_up_and_report_error;

        // rc = sqlite3_bind_text(csv_insert, 3, continent, continent_len, NULL); if(rc != SQLITE_OK) goto clean_up_and_report_error;
        rc = sqlite3_bind_text(csv_insert, 3, continent, continent_len, NULL); if(rc != SQLITE_OK) goto clean_up_and_report_error;
        rc = sqlite3_bind_text(csv_insert, 4, country, country_len, NULL); if(rc != SQLITE_OK) goto clean_up_and_report_error;
        rc = sqlite3_bind_text(csv_insert, 5, city, city_len, NULL); if(rc != SQLITE_OK) goto clean_up_and_report_error;

        rc = sqlite3_bind_double(csv_insert, 6, latitude); if(rc != SQLITE_OK) goto clean_up_and_report_error;
        rc = sqlite3_bind_double(csv_insert, 7, longitude); if(rc != SQLITE_OK) goto clean_up_and_report_error;

        rc = sqlite3_step(csv_insert);
        if(rc != SQLITE_DONE){
            printf("SQLite erroed while writing csv insert: %s\n", sqlite3_errmsg(db));
        }

        goto end_of_clean_up;
        clean_up_and_report_error:
        printf("SQLite errored while binding to csv INSERT stmt: %s\n", sqlite3_errmsg(db));
        end_of_clean_up:


        free(country);
        free(state);
        free(city);
        memset(lineBuf, 0, sizeOfBuffer);
        sqlite3_reset(csv_insert);
    }
    fclose(fptr);
    sqlite3_finalize(csv_insert);

    sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    sqlite3_exec(db, "CREATE INDEX ip_idx ON ip (ip_start, ip_end);", 0, 0, 0);
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", 0, 0, 0);

    sqlite3_close(db);
    return 0;
}