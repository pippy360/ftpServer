all: server.out

#todo: hack here, "./hiredis/*.o" find out how to actually use makefiles !!!
server.out: ftp.o vfs.o ftpParser.o server.o networking.o
	gcc ftp.o vfs.o ftpParser.o server.o networking.o ./hiredis/*.o -lssl -lcrypto -g -o server.out

server.o: server.c
	gcc -c server.c

vfs.o: vfs.c
	gcc -c vfs.c

ftp.o: ftp.c
	gcc -c ftp.c

ftpParser.o: ftpParser.c
	gcc -c ftpParser.c

networking.o: net/networking.c
	gcc -c net/networking.c
