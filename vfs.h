redisReply *vfs_getFileName(redisContext *context, long id);

redisReply *vfs_getFolderName(redisContext *context, long id);

long vfs_createFile(redisContext *context, long parentId, char *name, long size);

long vfs_mkdir(redisContext *context, long parentId, char *name);

void vfs_pwd(redisContext *context, long dirId);

long vfs_getFileIdByName(redisContext *context, long parentFolderId, char *inputName);

long vfs_getFolderIdByName(redisContext *context, long parentFolderId, char *inputName);

void vfs_getFolderPathFromId(redisContext *context, long inputId, char *outputBuffer, int outputBufferLength);

long vfs_getIdFromPath(redisContext *context, char *path);

long vfs_getIdByName(redisContext *context, long parentFolderId, char *inputName);

void vfs_pwd(redisContext *context, long dirId){

