//todo: handle cases where send doesn't send all the bytes we asked it to
//todo: write packet logger !!!
//TODO: who closes the connection ?
//TODO: learn error handling
//"Note that the major and minor numbers MUST be treated as"
//"separate integers and that each MAY be incremented higher than a single digit."
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "net/networking.h"
#include "ftpCommon.h"
#include "ftp.h"
#include "ftpParser.h"

#define MAX_PACKET_SIZE 1600
#define SERVER_LISTEN_PORT "25001"
#define FILE_SERVER_URL "localhost"
#define VALID_GREETING "220 fuck yeah you've connected ! what are you looking for...?\r\n"

void handle_client( int client_fd ){

    char buffer[MAX_PACKET_SIZE], pBuffer[MAX_PACKET_SIZE], usernameBuffer[100];
    int sent, recieved;
    ftpParserState_t parserState;
    ftpClientState_t clientState;
    ftp_newParserState(&parserState, pBuffer, MAX_PACKET_SIZE);
    ftp_newClientState(&clientState, client_fd, usernameBuffer, 100);
    
    sent = send(client_fd, VALID_GREETING, strlen(VALID_GREETING), 0);
    
    while(1){
        recieved = recv(client_fd, buffer, MAX_PACKET_SIZE, 0);
        printf("buffer: %.*s--\n", recieved, buffer);
        ftp_parsePacket(buffer, recieved, &parserState, &clientState);
        //ftp_handleResponse();
        if(parserState.type == REQUEST_QUIT){
            printf("got a quit\n");
            break;
        }else{
            //handle a bad packet
        }
        //handle a good packet
        ftp_handleFtpRequest(&parserState, &clientState);
    }

    close(client_fd);
    printf("connection closed\n");
}

int main(int argc, char *argv[])
{

    int sockfd = get_listening_socket(SERVER_LISTEN_PORT);
    int client_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            
            handle_client( client_fd );
            
            close(client_fd);
           //recv from client, now get a file from google

            exit(0);
        }
        close(client_fd);  // parent doesn't need this
    }


    return 0;
}
