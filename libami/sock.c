
#include "sock.h"
#include "dbg.h"

/* Connect socket, set non blocking and return the descriptor */
int socket_connect( char *host, int port ) {

   int sockfd;
   struct sockaddr_in dest_addr;
   struct hostent *hp;

   memset(&dest_addr, 0, sizeof(struct sockaddr_in));

   if ((hp = gethostbyname(host)) == NULL) {
      return -1;
   }

   sockfd = socket(AF_INET, SOCK_STREAM, 0);


   /* fcntl(sockfd, F_SETFL, O_NONBLOCK); */

   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons(port);
   dest_addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

   /* this will usually return -1 on a non-blocking socket */
   connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

   return sockfd;
}

int socket_recv( int sockfd, char *buf, int len) {
	int r;
  	r = recv(sockfd, buf, len, 0);
	/* printf("socket_recv: %s\n", buf); */
	return r;
}

int socket_send(int sockfd, char *buf) {
	int r;
	r = send( sockfd, buf, strlen(buf), 0);
	/* printf("socket_send: %s\n", buf); */
	return r;
}
