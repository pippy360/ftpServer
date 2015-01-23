#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./hiredis/hiredis.h"
#define MAX_FILENAME_SIZE 10000

void init(){

}

long getNewId(redisContext *context){
    redisReply *reply;
    reply = redisCommand(context, "GET current_id_count");
    long result = strtol(reply->str, NULL, 10);
    freeReplyObject(reply);
    reply = redisCommand(context, "INCR current_id_count");
    freeReplyObject(reply);
    return result;
}

void __mkdir(redisContext *context, long parentId, long id, char *name){
    redisReply *reply;
    reply = redisCommand(context, "HMSET FOLDER_%lu_info  parent \"%lu\" name \"%s\" createdData \"%s\"", 
                        id, parentId, name, "march 7th");
    freeReplyObject(reply);
}

//id 0 == root
void vfs_buildDatabase(redisContext *context){
	//wipe it and set the first id to 0 and crate a root folder
    redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
    freeReplyObject(reply);
	__mkdir(context, 0, 0, "root");
	reply = redisCommand(context, "SET current_id_count 0");
    freeReplyObject(reply);
}

//returns dir id
long vfs_mkdir(redisContext *context, long parentId, char *name){
	long id = getNewId(context);
    redisReply *reply;
    reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", parentId, id);
    freeReplyObject(reply);
    __mkdir(context, parentId, id, name);
    return id;
}

//returns the reply for the name resquest
//so use freeReplayObject() to free 
redisReply *vfs_getFileName(redisContext *context, long id){
    redisReply *reply;
    reply = redisCommand(context, "HGET FILE_%lu_info name", id);
    return reply;
}

//^see get file name
redisReply *vfs_getFolderName(redisContext *context, long id){
    redisReply *reply;
    reply = redisCommand(context, "HGET FOLDER_%lu_info name", id);
    return reply;
}

void __createFile(redisContext *context, long id, char *name, long size){
    redisReply *reply;
    reply = redisCommand(context, "HMSET FILE_%lu_info name \"%s\" size \"%lu\" createdData \"%s\" ", 
                            id, name, size, "march 7th");
    freeReplyObject(reply);
}

long vfs_createFile(redisContext *context, long parentId, char *name, long size){
	//add it to the file list of the dir
	long id = getNewId(context);
    redisReply *reply;
    reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", parentId, id);
    freeReplyObject(reply);
    __createFile(context, id, name, size);
    return id;
}

//return part id
void vfs_createPart(redisContext *context, long fileId, char *partUrl, long rangeStart, long rangeEnd){
	//add it to the list
}

void __createPart(){
	//creates the info part
}

void vfs_pwd(redisContext *context, long dirId){
    int j = 0;
    long id;
    redisReply *reply, *name;
    reply = redisCommand(context,"LRANGE FOLDER_%lu_folders 0 -1", dirId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            id = strtol(reply->element[j]->str, NULL, 10);
            name = vfs_getFolderName(context, id);
            printf("%s\n", name->str);
            freeReplyObject(name);
        }
    }
    freeReplyObject(reply);
    reply = redisCommand(context,"LRANGE FOLDER_%lu_files   0 -1", dirId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            long id = strtol(reply->element[j]->str, NULL, 10);
            name = vfs_getFileName(context, id);
            printf("%s\n", name->str);
            freeReplyObject(name);
        }
    }
    freeReplyObject(reply);
}

void vfs_rmDir(){

}

void vfs_rmFile(){
	
}

//-1 if not found, id otherwise
long vfs_getFileIdByName(redisContext *context, long parentFolderId, char *inputName){
    int j = 0;
    long id, result = -1;
    redisReply *reply, *name;
    reply = redisCommand(context,"LRANGE FOLDER_%lu_files 0 -1", parentFolderId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            id = strtol(reply->element[j]->str, NULL, 10);
            name = vfs_getFolderName(context, id);
            if(strcmp(name->str, inputName) == 0){
                result = id;
            }
            freeReplyObject(name);
        }
    }
    freeReplyObject(reply);
    return result;
}

//-1 if not found, id otherwise
long vfs_getFolderIdByName(redisContext *context, long parentFolderId, char *inputName){
    int j = 0;
    long id, result = -1;
    redisReply *reply, *name;
    reply = redisCommand(context,"LRANGE FOLDER_%lu_folders 0 -1", parentFolderId);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            id = strtol(reply->element[j]->str, NULL, 10);
            name = vfs_getFolderName(context, id);
            if(strcmp(name->str, inputName) == 0){
                result = id;
            }
            freeReplyObject(name);
        }
    }
    freeReplyObject(reply);
    return result;
}

long vfs_getIdByName(redisContext *context, long parentFolderId, char *inputName){
    long id;
    if ((id = vfs_getFileIdByName(context, parentFolderId, inputName)) != -1){
        return id;
    }else if((id = vfs_getFolderIdByName(context, parentFolderId, inputName)) != -1){
        return id;
    }
    return -1;
}

//path must start with '/'
long vfs_getIdFromPath(redisContext *context, char *path){
    if (path[0] != '/')
        return -1;

    char *ptr = path + 1;
    long cwdId = 0;
    long resultId = 0;
    char buffer[MAX_FILENAME_SIZE];

    while( *ptr ){
        //get the name and search it
        int i;
        for (i = 0; *ptr && *ptr != '/'; i++, ptr++){
            if (isprint(*ptr)){
                return -1;
            }
            buffer[i] = *ptr;
        }
        buffer[i] = '\0';

        //if the name isn't found return -1
        if((resultId = vfs_getIdByName(context, resultId, buffer)) == -1)
            return -1;

        //jump the '/'
        if (*ptr == '/')
            ptr++;
    }
    return resultId;
}

void vfs_getFolderPathFromId(redisContext *context, long inputId, char *outputBuffer, int outputBufferLength){
    long currentId = inputId;
    redisReply *parentIdReply, *nameReply;
    char buffer[outputBufferLength];

    outputBuffer[0] = '\0';
    while(currentId != 0){
        //get the name of the current id
        nameReply = vfs_getFolderName(context, currentId);

        //FIXME: check if it'll fit !
        //if(strlen(outputBuffer) > outputBufferLength){
        //}
        sprintf(buffer, "/%s%s", nameReply->str, outputBuffer);
        strcpy(outputBuffer, buffer);
        freeReplyObject(nameReply);
        
        //get it's parent's id
        parentIdReply = redisCommand(context, "HGET FOLDER_%lu_info parent", currentId);
        currentId = strtol( parentIdReply->str, NULL, 10);
        freeReplyObject(parentIdReply);
    }
}

/*
int main(int argc, char const *argv[])
{
    unsigned int j;
    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    vfs_buildDatabase(c);
    long newDirId = vfs_mkdir(c, 0, "new folder");
    long newFileId = vfs_createFile(c, newDirId, "a_new_file.webm", 1000);
    newDirId = vfs_mkdir(c, newDirId, "foldhere");
    newFileId = vfs_createFile(c, newDirId, "far_down_file.webm", 1000);

    vfs_pwd(c, 0);
    printf("%lu\n", vfs_getIdFromPath(c, "/new folder/foldhere"));
    //pretty print the files and folders
    //list the parts !!

    return 0;
}
*/

