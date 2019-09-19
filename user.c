#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char * argv[]) {
    char buffer[1024];
    char * token;

    //get input
    
    fgets(buffer, 1024, stdin);
    if ((token = strchr(buffer, '\n')) != NULL)
        *token = '\0'; //remove last \n if it exists


    token = strtok(buffer, " ");
    
    if (strcmp(token, "reg") == 0 || strcmp(token, "register") == 0) {
        token = strtok(NULL, " ");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " ");
        }
    }
}