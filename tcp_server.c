#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <error.h> 

#define PORT 8085

void tcp_server();

void main()
{	int server_fd,client_fd;
	tcp_server(server_fd,client_fd);        
}

void tcp_server(int client_fd,int server_fd){
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    	error(1, errno, "error creating socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    	error(1, errno, "error binding to port 8085");

    int lis = listen(server_fd, 10);
    if (lis == -1)
        error(1, errno, "error listening on socket");
         
    int len=sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd == -1)
        error(1, errno,"error accepting connections");
    else
        printf("connection established succesfully\n");
    close(server_fd);            
}