
#include "../global.h"



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>


#define error printf
#define CRLF "\x0d\x0a.\x0d\x0a"
#define CR   "\x0d\x0a"
#define	SA      struct sockaddr
#define	MAXLINE	8192
#define POP3_PORT    110



int checkConn( char * inServer, int port )
{
	//
	// Check that the username and password are valid
	// Display error message or that they have accessed correctly
	//
	int	sockfd;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons( port );
	inet_pton(AF_INET, inServer, &servaddr.sin_addr);

	if ( !connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) ) return sockfd;
	else return -1;
}



ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c;
	char *ptr;

	ptr = (char *)vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = recv(fd, &c, 1, 0)) == 1) {
			*ptr++ = c;
			if (c == '\n') break;
		} else if (rc == 0) {
			if (n == 1) return(0);	/* EOF, no data read */
			else break;		/* EOF, some data was read */
		} else return(-1);	/* error */
	}

	*ptr = 0;
	return(n);
}

ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t	n;

	if ( (n = readline(fd, ptr, maxlen)) == -1) error("readline error");
	return(n);
}


int checkUser( char * User, char * Pass, int sockfd)
{
	char recvline[MAXLINE], cmdUser [ 60 ];
	bzero(cmdUser,60);

	strcat( cmdUser, "user " );
	strcat( cmdUser, User );
	strcat( cmdUser, "\n" );

	char cmdPass [ 60 ];
	bzero(cmdPass,60);

	strcat( cmdPass, "pass " );
	strcat( cmdPass, Pass );
	strcat( cmdPass, "\n" );

	if (Readline(sockfd, recvline, MAXLINE) == 0)
		error("checkUser: server terminated prematurely");

	send(sockfd, cmdUser, strlen(cmdUser),0);

	if (Readline(sockfd, recvline, MAXLINE) == 0)
		error("checkUser: server terminated prematurely");

	send(sockfd, cmdPass, strlen(cmdPass), 0 );

	if (Readline(sockfd, recvline, MAXLINE) == 0)
		error("checkUser: server terminated prematurely");

	if ( recvline[ 0 ] == '-' ) {
		fputs ( "\nUsuario o Contraseña incorrectos\n\n", stdout );
		return 0;
	}
	return 1;
}


static char encode(unsigned char u)
{

	if(u < 26)  return 'A'+u;
	if(u < 52)  return 'a'+(u-26);
	if(u < 62)  return '0'+(u-52);
	if(u == 62) return '+';

	return '/';

}




void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}


void itoa(int n, char s[])
{
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;          /* make n positive */
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);     /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}
int getNumberOfRemoteMsgs( int sockfd )
{
	char recvline[ MAXLINE ];
	bzero( recvline, MAXLINE );
	char cmd[ MAXLINE ];
	bzero( cmd, MAXLINE );

	strcat( cmd, "stat\x0d\x0a" );

	send( sockfd, cmd, strlen( cmd ), 0 );

	if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
		error("getNumberOfMsgs: terminated prematurely");

	int i = 0, j = 0;

	char number[ 6 ];
	bzero( number, 6 );
	//recvline example:  "+OK 6 146378

	while ( recvline[ i++ ] != ' ' );

	while ( recvline[ i ] != ' ' )
		number[ j++ ] = recvline[ i++ ];

	return atoi( number );
}

void parseRemoteHeaders(int sockfd, int *remote_command, int *delete_index)
{
	const int NofMessages = getNumberOfRemoteMsgs( sockfd );
	if( ! NofMessages ) {
		printf( "\n\tNo new mail\n" );
		return;
	}

	char recvline[ MAXLINE ];
	bzero( recvline, MAXLINE );
	char cmd[ MAXLINE ];
	bzero( cmd, MAXLINE );

	char Subject[ MAXLINE ], Date[ MAXLINE ], From[ MAXLINE ];
	int index = 1;
	char number[ 6 ];

	int b = 1;


	while ( index != NofMessages + 1 ) {


		bzero( number, 6 );
		itoa( index, number );
		bzero( cmd, MAXLINE );
		strcat( cmd, "top " );
		strcat( cmd, number );
		strcat( cmd, " 0\x0d\x0a" );

		if ( b ) {
			send( sockfd, cmd, strlen( cmd ), 0 );
			b = 0;
		}

		if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
			error("getHeaders: server terminated prematurely");


		if (NULL != strstr(recvline, "Subject:"))
			if (strncmp(recvline, "Subject:", strlen("Subject:")) == 0) {
				//printf("%s", recvline);
				if (NULL != strstr(recvline, "video")) {
					printf("will del:%s", recvline);
					*remote_command = VIDEO;
					*delete_index = index;
				} else 	if (NULL != strstr(recvline, "image")) {
					printf("will del:%s", recvline);
					*remote_command = IMAGE;
					*delete_index = index;
				}


		}

		if (recvline[0] == '.') {
			index++;
			b = 1;
		}

	}

}



int delFromServer( int sockfd, int option )
{
	int N = getNumberOfRemoteMsgs( sockfd );

	if ( ! N ) {
		printf( "Warning: inbox is empty !!!" );
		return 0;
	}

	if ( N < option ) {
		printf( "There are just %d, Messages", N );
		return 0;
	}

	char cmd[ 15 ], number[ 6 ], recvline[ MAXLINE ];
	bzero( number, 6 );
	bzero( cmd, 15 );
	bzero( recvline, MAXLINE );
	itoa( option, number );
	strcat( cmd, "dele " );
	strcat( cmd, number );
	strcat( cmd, CR );

	send( sockfd, cmd, strlen( cmd ), 0 );

	if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
		error("delFromServer: terminated prematurely");

	if ( recvline[ 0 ] == '+' )
		printf ( "Message %d will be deleted when the session is ended\n", option );
	else
		printf( "There have been errors when trying to delete the message" );

	return 1;
}




int pop3_main(void (*event_handler)(int))
{
	printf( "Welcome to the pop3 client modified by tanhangbo\n\n" );
	struct hostent *h;
	int delete_index = 0;
	int remote_command = 0;
	char pop3_server[] = POP3_SERVER;
	//char smtp_server[] = SMTP_SERVER;
	char user_name[] = USER_NAME;
	char pass_word[] = MAIL_PASSWORD;


	char  inServer[3*4 + 4 +1];//111.111.111.111\0
	//char  outServer[3*4 + 4 + 1];
	h = gethostbyname(pop3_server);
	strcpy(inServer , inet_ntoa(*((struct in_addr *)h->h_addr)));
	//h = gethostbyname(smtp_server);
	//strcpy(outServer , inet_ntoa(*((struct in_addr *)h->h_addr)));
	printf("POP3 IP:%s\n", inServer);
	//printf("SMTP IP:%s\n", outServer);

	while(1) {

		delete_index = 0;
		remote_command = 0;


		int sockfd = checkConn( inServer, POP3_PORT );
		if( sockfd == -1 )
			return 0;
		if(!checkUser(user_name, pass_word, sockfd)) {
			printf("login fail!\n");
		}


		parseRemoteHeaders(sockfd, &remote_command, &delete_index);
		if (0 != remote_command)
			event_handler(remote_command);//handle here!!!!!!!
		if (0 != delete_index)
			delFromServer(sockfd, delete_index);


		send( sockfd, "QUIT\x0d\x0a", strlen( "QUIT\x0d\x0a" ), 0 );
		close(sockfd);
		sleep(1);

	}


	return 0;
}



