/*
 *
 *
 *
 * a parserState must have a valid request and parameter before it can be passed into any of these functions
 *
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
#include "hiredis/hiredis.h"
#include "vfs.h"

//#define SEVER_IP_FTP "178,62,20,142"//FIXME: this is a fucking horrible hack
#define SEVER_IP_FTP "127,0,0,1"//FIXME: this is a fucking horrible hack



#define EXAMPLE_DIR "-rw-rw-r--. 1 lilo lilo 100 Feb 26 07:08 file1\r\n"


//FIXME:
int isValidLogin(char *username, char *password){
	if (strcmp(username, "tom") == 0 && strcmp(password, "wootwoot") == 0)
		return 1;
	else
		return 0;
}

int sendFtpResponse(ftpClientState_t *clientState, char *contents){
	printf("response: %s\n", contents);
	return send(clientState->command_fd, contents, strlen(contents), 0);
}

void ftp_newClientState(ftpClientState_t *clientState, int command_fd, 
							char *usernameBuffer, int usernameBufferLength){

	clientState->command_fd 			= command_fd;
	clientState->usernameBuffer 		= usernameBuffer;
	clientState->usernameBufferLength 	= usernameBufferLength;
	clientState->transferType = FTP_TRANSFER_TYPE_I;
	clientState->loggedIn				= 0;
	clientState->data_fd				= -1;
	clientState->data_fd2				= -1;
	clientState->isDataConnectionOpen 	= 0;
	clientState->cwdId					= 0;
}

void openDataConnection(ftpClientState_t *clientState){
	char strBuf1[1000];//FIXME: hardcoded
	int tempSock = get_listening_socket("5000");//FIXME: WHAT DO I DO WITH TEMPSOCK ?????
	clientState->data_fd2 = tempSock;
	int port = getPort(tempSock);
	sprintf(strBuf1, "227 Entering Passive Mode (%s,%d,%d).\r\n", SEVER_IP_FTP, port >> 8, port&255);
	sendFtpResponse(clientState, strBuf1);
	socklen_t sin_size;
    struct sockaddr_storage their_addr;
	sin_size = sizeof their_addr;
    clientState->data_fd = accept(tempSock, (struct sockaddr *)&their_addr, &sin_size);
    if (clientState->data_fd == -1) {
        perror("accept");
    }
	clientState->isDataConnectionOpen = 1;
}

void sendFile(ftpClientState_t *clientState){
	FILE *ifp, *ofp;
	char *mode = "r";
	char outputFilename[] = "out.list";

	ifp = fopen("output.txt", mode);

	if (ifp == NULL) {
	  fprintf(stderr, "Can't open input file in.list!\n");
	  exit(1);
	}
	char buffer[100000];
	fread ( buffer, 10000, 1, ifp);
	send(clientState->data_fd, buffer, strlen(buffer), 0);
	close(clientState->data_fd);
}

void ftp_handleFtpRequest(redisContext *vfsContext, ftpParserState_t *parserState, ftpClientState_t *clientState){

	char strBuf1[1000];//FIXME: hardcoded
	char strBuf2[1000];//FIXME: hardcoded
	long id, fileSize;

	switch(parserState->type){
		case REQUEST_USER:
			strcpy(clientState->usernameBuffer, parserState->paramBuffer);
			sendFtpResponse(clientState, "331 User name okay, need password.\r\n");
			clientState->loggedIn = 0;
			break;
		case REQUEST_PASS:
			if( isValidLogin(clientState->usernameBuffer, parserState->paramBuffer) ){
				sendFtpResponse(clientState, "230 User logged in, proceed.\r\n");
				clientState->loggedIn = 1;
			}else{
				sendFtpResponse(clientState, "430 Invalid username or password\r\n");
			}
			break;
		case REQUEST_SYST:
			sendFtpResponse(clientState, "215 UNIX Type: L8\r\n");
			break;
		case REQUEST_PWD:
			vfs_getFolderPathFromId(vfsContext, clientState->cwdId, strBuf2, 1000);
			sprintf(strBuf1, "257 %s\r\n", strBuf2);
			sendFtpResponse(clientState, strBuf1);
			break;
		case REQUEST_TYPE:
			if(strcmp(parserState->paramBuffer, "I") == 0){
				clientState->transferType = FTP_TRANSFER_TYPE_I;
				sendFtpResponse(clientState, "200 Switching to Binary mode..\r\n");				
			}else{
				sendFtpResponse(clientState, "504 Command not implemented for that parameter.\r\n");				
			}
			break;
		case REQUEST_PASV:
			if (clientState->isDataConnectionOpen == 1){
				printf("there was an open connection, it's closed\n");
				close(clientState->data_fd);
				close(clientState->data_fd2);
				clientState->isDataConnectionOpen = 0;
			}
			openDataConnection(clientState);
			break;
		case REQUEST_SIZE:
			id = vfs_getIdFromPath(vfsContext, parserState->paramBuffer);
			if ((fileSize = vfs_getFileSizeFromId(vfsContext, id)) == -1){
				sendFtpResponse(clientState, "550 Could not get file size.\r\n");
			}else{
				sprintf(strBuf1, "213 %ld\r\n", fileSize);
				sendFtpResponse(clientState, strBuf1);//TODO:
			}
			break;
		case REQUEST_LIST:
			sendFtpResponse(clientState, "150 Here comes the directory listing.\r\n");
			
			send(clientState->data_fd, EXAMPLE_DIR, strlen(EXAMPLE_DIR), 0);
			//sendFile(clientState);
			if(close(clientState->data_fd) != 0){
				perror("close:");
			}
			if(close(clientState->data_fd2) != 0){
				perror("close2:");
			}
			sendFtpResponse(clientState, "226 Directory send OK.\r\n");
			break;
		case REQUEST_CWD:
			if((id = vfs_getIdFromPath(vfsContext, parserState->paramBuffer)) == -1){
				sendFtpResponse(clientState, "550 Failed to change directory.\r\n");
			}else{
				clientState->cwdId = id;
				sendFtpResponse(clientState, "250 Directory successfully changed.\r\n");//success
			}
			break;
		default:
			sendFtpResponse(clientState, "502 Command not implemented\r\n");
			break;
	}
}







