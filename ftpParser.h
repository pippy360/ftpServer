void ftp_newParserState(ftpParserState_t *state, char *paramBuffer, int paramBufferLength);

int ftp_parsePacket(char *packet, int packetLength, ftpParserState_t *parserState, ftpClientState_t *clientState);
