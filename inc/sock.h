#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>


int socket_connect( char *host, int port );
int socket_recv( int sockfd, char *buf, int len);
int socket_send(int sockfd, char *buf);
