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
#include "ftpCommon.h"

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
	clientState->loggedIn				= 0;
	clientState->data_fd				= -1;
	clientState->isDataConnectionOpen 	= 0;
}

void ftp_handleFtpRequest(ftpParserState_t *parserState, ftpClientState_t *clientState){

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
			//this is where clientState comes in
			break;
		default:
			sendFtpResponse(clientState, "202 Command not implemented\r\n");
			break;
	}
}









