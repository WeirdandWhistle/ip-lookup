#include "buildDB.h"
#include "httpServer.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "main.h"

int main(int argc, char* argv[]){
    printf("Program stated with PID %d.\n", getpid());
    printf("size; float %d, double %d.\n", sizeof(float), sizeof(double));
    if(argc < 2){
        printf("No argument specifed.\n");
        return 1;
    } else if(strcmp("build", argv[1]) == 0){
        return build_db();
    } else if(strcmp("serve", argv[1]) == 0){
        return http_server_init();
    }

    printf("Unknown argument: %s\n", argv[1]);
    return 1;
}