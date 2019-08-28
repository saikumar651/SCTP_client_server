#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <error.h> 

#define PORT 8085

void sctp_server();

void main()
{	
	sctp_server();        
}

void sctp_server(){
    int server_fd,client_fd;
    char *msg="hi from server!";
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (server_fd == -1)
    	error(1, errno, "error creating socket");

    bzero((void *)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_ret == -1){
        close(server_fd);
    	error(1, errno, "error binding to port\n");
    }

    int lis = listen(server_fd, 5);
    if (lis == -1)
        error(1, errno, "error listening on socket\n");
         
    int len=sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd == -1)
        error(1, errno,"error accepting connections\n");
    else
        printf("connection established succesfully\n");

    int send_ret = sctp_sendmsg(client_fd, msg,strlen(msg)+1, NULL, 0, 0, 0, 0, 0, 0 );
    if(send_ret == -1)
        error(1,errno,"message sending failed\n");
    else
        printf("%d bytes are succesfully sent\n",send_ret);

    close(server_fd);            
}