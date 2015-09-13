#include	"unp.h"

#define DPISERVER_PORT  22222
typedef enum {
    MSGTYPE_PACKET = 1,
    MSGTYPE_FILE,
    MSGTYPE_CONFIG
}dpimsg_type_e;

typedef struct msgheader {
    unsigned short type;
    unsigned short len;
    char* data[0];
}msgheader;

int capnum = 0;
void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

void dump_buffer(unsigned char *buffer, int buflen)
{
    char print_buf[512];
    int len = 0;
    unsigned int i;
    for(i = 0; i < buflen; ++i)
    {
        len += snprintf(print_buf + len, 512 - len, " %02x", buffer[i]);
        if( (i + 1) % 16 == 0 )
        {
            printf("%s\n", print_buf);
            len = 0;
        }
    }
    printf("\n"); 
}

void handle_packet(int sockfd)
{
	ssize_t			n;
    msgheader  header;
    unsigned char buffer[BUFSIZ];
    int recvlen =0;
    /* int capnum = 0; */
	for ( ; ; ) {
		if ( (n = Readn(sockfd, &header, sizeof(header))) == 0) {
            printf("child %d capture %d packets.\n", getpid(), capnum);
			return;		/* connection closed by other end */
        }
        printf("\nType: %d\n", header.type);
        printf("Len: %d\n", header.len);
        recvlen = header.len < BUFSIZ ? header.len : BUFSIZ;
		if ( (n = Readn(sockfd, &buffer, recvlen)) == 0)  {
            printf("child %d capture %d packets.\n", getpid(), capnum);
			return;		/* connection closed by other end */
        }
        dump_buffer(buffer, n);
        capnum++;
    }
}

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	void				sig_chld(int);

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(DPISERVER_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);
    printf("Server started. Listen port %d\n", DPISERVER_PORT);
	Signal(SIGCHLD, sig_chld);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}
        printf("receive a new connectionn\n");
		if ( (childpid = Fork()) == 0) {	/* child process */
			Close(listenfd);	/* close listening socket */
			handle_packet(connfd);	/* process the request */
			exit(0);
		}
		Close(connfd);			/* parent closes connected socket */
	}
}
