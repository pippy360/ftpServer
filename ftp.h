
void ftp_newClientState(ftpClientState_t *clientState, int command_fd, char *usernameBuffer, int usernameBufferLength);

void ftp_handleFtpRequest(redisContext *vfsContext, ftpParserState_t *parserState, ftpClientState_t *clientState);
