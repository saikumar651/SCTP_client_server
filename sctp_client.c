#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <error.h> 

#define PORT 8085

void sctp_client();

void main()
{	
	sctp_client();        
}

void sctp_client(){
    int client_fd;
    char buffer[1024];
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (client_fd == -1)
    	error(1, errno, "error opening socket");

    bzero((void *)&server_addr,sizeof(server_addr));
   	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    	error(1, errno, "error connecting to server");

    int recv_ret = sctp_recvmsg(client_fd, buffer, sizeof(buffer), NULL, 0, 0, 0);
    if(recv_ret == -1)
        error(1,errno,"message receiving failed\n");
    else
        printf("%d bytes are received\n",recv_ret);

    for(int i=0;i<recv_ret;i++)
        printf("%c",buffer[i]);

    close(client_fd);
}