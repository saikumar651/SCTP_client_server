#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h> 
#include <inttypes.h>
#include <stdint.h>
#include <time.h>  

#define PORT 8085
#define BUFFER_SIZE  60               //Buffer to store SDU sent by PDCP layer
#define SIZE 15	                      //size of the each SDU in transmission_Buffer
#define SEGMENT_SIZE 7			      //segment size should be minimum of 4 bytes(it will be given by MAC layer) 

void tcp_server();
void Buffering();
void segmentation();
void not_segmented();
void first_segment();
void middle_segments();
void last_segment();
 
struct UM_PDU_NS{
	uint8_t SI_R;					  //segmentation info field of PDU
	uint8_t data[SEGMENT_SIZE-1];     //data field of PDU
};                                    //structure format for SDU which is not segmented

struct UM_PDU_S1{                     
    uint8_t SI_SN;                    //segmentation info and sequence no field of PDU
	uint8_t data[SEGMENT_SIZE-1];     //data field of PDU
};									  //structure format for 1st segment of SDU after segmentation

struct UM_PDU_S2{
	uint8_t SI_SN;					  //segmentation info and sequence no field of PDU
	uint8_t SO_1;                     //first 8 bits of segment offset of PDU
	uint8_t SO_2;                     //next  8 bits of segment offset of PDU
	uint8_t data[SEGMENT_SIZE-3];     //data field of PDU		
	     							  //strucute format for middle and last segments of SDU after segmentation
};

int server_fd;
int client_fd;
uint8_t transmission_Buffer[BUFFER_SIZE];
uint8_t retransmission_Buffer[BUFFER_SIZE];
int transmission_Buffer_index=0;
int retransmission_Buffer_index=0;
int SDU_SIZE;
int SDU_INDEX;
uint8_t SI;
uint8_t TX_Next=0;
uint16_t SO;
    
void main()
{
    tcp_server(); 
    Buffering();    
	segmentation();
	close(server_fd);
 //clock_t start=clock();	
 //	double time_taken=((double) (clock()-start))/CLOCKS_PER_SEC;
 //	printf("time taken:%lf\n",time_taken);
                
} 

void tcp_server(){
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
}

void Buffering(){
	//uint8_t transmission_Buffer[14]={0x33,0x63,0x27,0x53,0x93,0x22,0x26,0x53,0x83,0x25,0x63,0x24,0x67,0x52};
	int file_open=open("chapter-1.txt",O_RDONLY);
	if(file_open==-1)
	error(1,errno,"opening of file failed\n");
	
	int file_read=read(file_open,transmission_Buffer,sizeof(transmission_Buffer));
	if(file_read==-1)
		error(1,errno,"reading from file failed\n");
	else{ 
		printf("%d bytes are read from file into transmission_Bufferfer\n",file_read);
	//	sleep(2);
		for(int i=0;i<file_read;i++)
		putchar(transmission_Buffer[i]);
		printf("\n");
	}
}

void segmentation(){
	while(transmission_Buffer_index<sizeof(transmission_Buffer)){	
		SDU_SIZE=SIZE;
		SDU_INDEX=0;
		SI=0;
		SO=0;
		while(SDU_SIZE!=0){
			if(SEGMENT_SIZE>SDU_SIZE && (SI==0 && TX_Next<=63) && (SO<=65535))		//for no segmentation				   					    
				not_segmented();
	    
	    	if(SEGMENT_SIZE<(SDU_SIZE+1) && (SI==0 && TX_Next<=63)  && (SO<=65535))     //for 1st segment
				first_segment();
				
	   		if(SEGMENT_SIZE<(SDU_SIZE+3) && ((SI==1 || SI==3) && TX_Next<=63)  && (SO<=65535))  //for middle segments
				middle_segments();
	   
	   		if(SEGMENT_SIZE>=(SDU_SIZE+3) && ((SI==3 || SI==1) && TX_Next<=63)  && (SO<=65535))     //for last segment
	   			last_segment();							
		}
	}	
}

void not_segmented(){
    struct UM_PDU_NS *PDU=(struct UM_PDU_NS *) calloc(1,sizeof(struct UM_PDU_NS));
	PDU->SI_R=0;
	for(int i=transmission_Buffer_index;i<(transmission_Buffer_index+SDU_SIZE);i++)
		PDU->data[i-transmission_Buffer_index]=transmission_Buffer[i];
                
    int first_byte=(int) PDU->SI_R;
    int fb=send(client_fd,&first_byte,4,0);
    if(fb==-1)
		error(1,errno,"1st byte of NS PDU is not sent\n");
	else
		printf("%d bytes are sent to socket\nfirst_byte of NS PDU:%x bytes is sent succesfully\n",fb,first_byte);
				 
	int signal=0;
    int sg=recv(client_fd,&signal,sizeof(signal),0);
    if(sg==-1)
		error(1,errno,"signal is not sent\n");
	else
		printf("%d bytes are sent to socket\nsignal value:%d is sent succesfully\n",sg,signal);
								 
	if(signal==4){  		   
	   	int ret=send(client_fd,PDU,SDU_SIZE+1,0);
	   	if(ret==-1)
	   		error(1,errno,"reading from socket failed from NS PDU\n");
	   	else
	   		printf("%d bytes are read from socket from NS PDU\n",ret);
	   		    	
	   	printf("%x\n",PDU->SI_R);
	    for(int i=0;i<SDU_SIZE;i++)
	   		printf("%x\n",PDU->data[i]);
	   	printf("\n");
	   	SDU_SIZE=0;
	   	free(PDU);
	 // printf("%d %d\n",SDU_SIZE,transmission_Buffer_index);	   			
	}	
}

void first_segment(){
	struct UM_PDU_S1 *PDU=(struct UM_PDU_S1 *) calloc(1,sizeof(struct UM_PDU_S1));
	SI=1;
	PDU->SI_SN= (SI<<6) | TX_Next;
	for(int i=transmission_Buffer_index;i<(transmission_Buffer_index+(SEGMENT_SIZE-1));i++)
		PDU->data[i-transmission_Buffer_index]=transmission_Buffer[i];

    int first_byte=(int) PDU->SI_SN;
    int fb=send(client_fd, &first_byte,4,0);
    if(fb==-1)
		error(1,errno,"1st byte of PDU1 is not sent\n");
	else
		printf("%d bytes are sent to socket\nfirst_byte of PDU1:%x bytes is sent succesfully\n",fb,first_byte);
					    		
	int flag1=0;
    int fg=recv(client_fd, &flag1,sizeof(flag1),0);
    if(fg==-1)
		error(1,errno,"flag of PDU1 is not received\n");
	else
		printf("%d bytes are read from socket\nflag of PDU1:%d bytes are received succesfully\n",fg,flag1);
				
	if(flag1==1){					    		
	   	int ret=send(client_fd,PDU,SEGMENT_SIZE,0);
	   	if(ret==-1)
	   		error(1,errno,"writing into socket failed from PDU1\n");
	   	else
	   		printf("%d bytes are written into socket from PDU1\n",ret);
	   		    	    		
	   	SDU_SIZE=SDU_SIZE-(SEGMENT_SIZE-1);
		transmission_Buffer_index=transmission_Buffer_index+(SEGMENT_SIZE-1);
		SDU_INDEX=SDU_INDEX+(SEGMENT_SIZE-1);
				    
	   	printf("%x\n",PDU->SI_SN);
	   	for(int i=0;i<SEGMENT_SIZE-1;i++)
	   		printf("%x\n",PDU->data[i]);
	   	printf("\n");
	   	free(PDU);					
	  	//		printf("%d %d\n",SDU_SIZE,transmission_Buffer_index); 		    
	}						
}

void middle_segments(){
	struct UM_PDU_S2 *PDU=(struct UM_PDU_S2 *) calloc(1,sizeof(struct UM_PDU_S2));
	SI=3;
	PDU->SI_SN= (SI<<6) | TX_Next;
	SO=(uint16_t) SDU_INDEX;			
	PDU->SO_1= SO & 0xff;
	PDU->SO_2= (SO>>8) & 0xff;					
	for(int i=transmission_Buffer_index;i<(transmission_Buffer_index+(SEGMENT_SIZE-3));i++)
		PDU->data[i-transmission_Buffer_index]=transmission_Buffer[i];
                
    int first_byte=(int) PDU->SI_SN;
    int fb=send(client_fd, &first_byte,4,0);
    if(fb==-1)
		error(1,errno,"1st byte of PDU2 is not sent\n");
	else
		printf("%d bytes are sent to socket\nfirst_byte of PDU2:%x bytes is sent succesfully\n",fb,first_byte);
				
	int flag2=0;
    int fg=recv(client_fd, &flag2,sizeof(flag2),0);
    if(fg==-1)
		error(1,errno,"flag of PDU2 is not received\n");
	else
		printf("%d bytes are read from socket\nflag of PDU2:%d bytes are received succesfully\n",fg,flag2);
				
	if(flag2==2){					    		
	   	int ret=send(client_fd,PDU,SEGMENT_SIZE,0);
	   	if(ret==-1)
	   		error(1,errno,"writing into socket failed from PDU2\n");
	   	else
	   		printf("%d bytes are written into socket from PDU2\n",ret);
	   		    	    		
	    SDU_SIZE=SDU_SIZE-(SEGMENT_SIZE-3);
		transmission_Buffer_index=transmission_Buffer_index+(SEGMENT_SIZE-3);
		SDU_INDEX=SDU_INDEX+(SEGMENT_SIZE-3);
					
	   	printf("%x\n",PDU->SI_SN);
	   	printf("%x\n%x\n",PDU->SO_1,PDU->SO_2);
	   	for(int i=0;i<SEGMENT_SIZE-3;i++)
	   		printf("%x\n",PDU->data[i]);
	   	printf("\n");
	   	free(PDU);	
	   	//	printf("%d %d\n",SDU_SIZE,transmission_Buffer_index);  	
	}	
}

void last_segment(){
	struct UM_PDU_S2 *PDU=(struct UM_PDU_S2 *) calloc(1,sizeof(struct UM_PDU_S2));
	SI=2;
	PDU->SI_SN= (SI<<6) | TX_Next;
	SO=(uint16_t) SDU_INDEX;
	PDU->SO_1= SO & 0xff;
	PDU->SO_2= (SO>>8) & 0xff; 
	TX_Next++;
	SI=0;
	SO=0;
	for(int i=transmission_Buffer_index;i<(transmission_Buffer_index+SDU_SIZE);i++)
		PDU->data[i-transmission_Buffer_index]=transmission_Buffer[i];
		
	transmission_Buffer_index=transmission_Buffer_index+SDU_SIZE;
 	SDU_INDEX=SDU_INDEX+SDU_SIZE;
	   			
	int first_byte=(int) PDU->SI_SN;
    int fb=send(client_fd,&first_byte,4,0);
    if(fb==-1)
		error(1,errno,"1st byte of PDU3 is not sent\n");
	else
		printf("%d bytes are sent to socket\nfirst_byte of PDU3:%x bytes are sent succesfully\n",fb,first_byte);
				
	int flag3=0;
    int fg=recv(client_fd,&flag3,sizeof(flag3),0);
    if(fg==-1)
	error(1,errno,"flag of PDU3 is not received\n");
	else
	printf("%d bytes are read from socket\nflag of PDU3:%d bytes are received succesfully\n",fg,flag3);
				
	if(flag3==3){					   			
    	int size=send(client_fd,&SDU_SIZE,sizeof(SDU_SIZE),0);
    	if(size==-1)
			error(1,errno,"size of last segmented PDU is not sent\n");
		else
			printf("%d bytes are sent to socket\npayload size of last segmented PDU:%d bytes are sent succesfully\n",size,SDU_SIZE);	      		
	      			
	   	int ret=send(client_fd,PDU,SDU_SIZE+3,0);
	   	if(ret==-1)
	   		error(1,errno,"writing into socket failed from PDU3\n");
	   	else
	   		printf("%d bytes are written into socket from PDU3\n",ret);
	   		     			
	   	printf("%x\n",PDU->SI_SN);
	   	printf("%x\n%x\n",PDU->SO_1,PDU->SO_2);
	   	for(int i=0;i<SDU_SIZE;i++)
	   	printf("%x\n",PDU->data[i]);
	 	printf("\n");
	   	SDU_SIZE=0;
	   	free(PDU); 	
	  //printf("%d %d\n",SDU_SIZE,transmission_Buffer_index);		
	}
}
