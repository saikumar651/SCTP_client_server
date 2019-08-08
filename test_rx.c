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
#define SIZE 15
#define SEGMENT_SIZE 7
#define reassembly 3

void tcp_client();
int receive_PDUs();
int check();
void receive_NS_PDU();
void receive_first_PDU();
void receive_middle_PDUs();
void receive_last_PDU(int);
void reassembling();
uint8_t highest_SN(uint8_t);
uint16_t min_offset(uint16_t);
int reassemble_first_segment(int,int);
int reassemble_middle_segments(int,int);
int reassemble_last_segment(int,int);
 
struct PDU{
	uint8_t data[SEGMENT_SIZE];
};

int client_fd;
struct PDU Reception_Buffer[1024];
int RB_index=0;
int SDU_SIZE=SIZE;
int last_segment[1024]={0};
uint8_t RX_Next_Highest=0;
uint8_t RX_Next_Reassembly=0;

void main()
{
	tcp_client();
    receive_PDUs();   
    reassembling();     
    close(client_fd);        
}

void tcp_client(){

    struct sockaddr_in server_addr;
    int ret;
 
    client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    	error(1, errno, "error opening socket");

   	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    	error(1, errno, "error connecting to server");
}

int receive_PDUs(){
while(1){ 
    
    int first_byte=check();
	if(first_byte==-1)
		return 0;
		
	else{				
		if(first_byte==0)
			receive_NS_PDU();     
	
    	else if((first_byte>=0x40) && (first_byte<=0x7f))      //for 1st segment
	    	receive_first_PDU();
 
		else if((first_byte>=0xc0) && (first_byte<=0xff))          //for middile segments
			receive_middle_PDUs();
		
		else if((first_byte>=0x80) && (first_byte<=0xbf))          //for last segment 
   			receive_last_PDU(first_byte);
		}
	}		
}

int check(){
    int first_byte=0xffffffff; 
	int fb=recv(client_fd,&first_byte,4,0);
	
	if(first_byte==0xffffffff)
		return -1;
	
	else{
		if(fb==-1)
    		error(1,errno,"1st byte of PDU is not received\n");
		else{
			printf("%d bytes are received from socket\nfirst_byte of PDU:%x is received succesfully\n",fb,first_byte);
			return (first_byte);	
		}		
	}	
}

void receive_NS_PDU(){
	int signal=4;
    int sg=send(client_fd,&signal,sizeof(signal),0);
    if(sg==-1)
	    error(1,errno,"signal is not sent\n");
	else
	    printf("%d bytes are sent to socket\nsignal value:%d is sent succesfully\n",sg,signal);
					
	struct PDU NS;
	int ret=recv(client_fd,&NS,SDU_SIZE+1,0);
	if(ret==-1)
		error(1,errno,"reading NS PDU from socket failed\n");
	else
		printf("%d bytes of NS PDU are read from the socket\n",ret);
	
	for(int i=1;i<SDU_SIZE+1;i++)
	printf("%x\n",NS.data[i]);
	printf("\n");
}

void receive_first_PDU(){
 	int flag1=1;
 	int fg=send(client_fd,&flag1,sizeof(flag1),0);
   	if(fg==-1)
		error(1,errno,"flag of PDU1 is not sent\n");
	else
		printf("%d bytes are sent to socket\nflag of PDU1:%d is sent succesfully\n",fg,flag1);
					
	int ret=recv(client_fd,&Reception_Buffer[RB_index],SEGMENT_SIZE,0);
	if(ret==-1)
		error(1,errno,"reading PDU1 from socket failed\n");
	else
		printf("%d bytes of PDU1 are read from the socket into reception buffer of index %d\n",ret,RB_index);
	
	for(int i=0;i<SEGMENT_SIZE;i++)
		printf("%x\n",Reception_Buffer[RB_index].data[i]);
		
    RB_index++;
  //sleep(5);
	printf("\n");
}

void receive_middle_PDUs(){
	int flag2=2;
 	int fg=send(client_fd, &flag2, sizeof(flag2),0);
   	if(fg==-1)
		error(1,errno,"flag of PDU2 is not sent\n");
	else
		printf("%d bytes are sent to socket\nflag of PDU2:%d is sent succesfully\n",fg,flag2);
		
	int ret=recv(client_fd,&Reception_Buffer[RB_index],SEGMENT_SIZE,0);
	if(ret==-1)
		error(1,errno,"reading PDU2 from socket failed\n");
	else
		printf("%d bytes of PDU2 are read from the socket into reception buffer of index %d\n",ret,RB_index);
	
	for(int i=0;i<SEGMENT_SIZE;i++)
		printf("%x\n",Reception_Buffer[RB_index].data[i]);
			
    RB_index++;
  //sleep(5);
	printf("\n");
}

void receive_last_PDU(int first_byte){
	int last_segment_index=first_byte & 0x0000003f;	
	int flag3=3;
    int fg=send(client_fd,&flag3,sizeof(flag3),0);
    if(fg==-1)
		error(1,errno,"flag of PDU3 is not sent\n");
	else
		printf("%d bytes are sent to socket\nflag of PDU3:%d is sent succesfully\n",fg,flag3);

    int LAST_SEGMENT_SIZE=-1;
    int size=recv(client_fd,&LAST_SEGMENT_SIZE,sizeof(LAST_SEGMENT_SIZE),0);
    if(size==-1)
		error(1,errno,"size of last segmented PDU is not received\n");
	else
		printf("%d bytes are received from socket\npayload size of last segmented PDU:%d is received succesfully\n",size,LAST_SEGMENT_SIZE);				
     
    last_segment[last_segment_index]=LAST_SEGMENT_SIZE;
    
	int ret=recv(client_fd,&Reception_Buffer[RB_index],LAST_SEGMENT_SIZE+3,0);
	if(ret==-1)
		error(1,errno,"reading PDU1 from socket failed\n");
	else
		printf("%d bytes of PDU1 are read from the socket into reception buffer of index %d\n",ret,RB_index);
	
	for(int i=0;i<LAST_SEGMENT_SIZE+3;i++)
		printf("%x\n",Reception_Buffer[RB_index].data[i]);
		
	RB_index++;
  //sleep(5);
	printf("\n");
}

void reassembling(){
	uint16_t base_offset=0xffff;
    int flag=0;
    
	RX_Next_Highest=highest_SN(RX_Next_Highest);
//	for(int i=0;i<10;i++)
//	printf("%d ",last_segment[i]);
//	printf("\n");
    base_offset=min_offset(base_offset);
		
	while(base_offset<SIZE){		
		for(int i=0;i<=RB_index;i++){
		//printf("%d\nflag:%d\n",i,flag);
			if((Reception_Buffer[i].data[0] & 0x3f)==RX_Next_Reassembly && (Reception_Buffer[i].data[0]>=0x40) && (Reception_Buffer[i].data[0]<=0x7f) && Reception_Buffer[i].data[0]!=0){
				flag=reassemble_first_segment(i,flag);
				break;
			//	printf("first\n");	
			}

			else if((Reception_Buffer[i].data[0] & 0x3f)==RX_Next_Reassembly && (Reception_Buffer[i].data[0]>=0xc0) && (Reception_Buffer[i].data[0]<=0xff) && Reception_Buffer[i].data[0]!=0 && flag==1){
				base_offset=reassemble_middle_segments(i,base_offset);
				break;
				//printf("middle\n");
			}

			else if((Reception_Buffer[i].data[0] & 0x3f)==RX_Next_Reassembly && (Reception_Buffer[i].data[0]>=0x80) && (Reception_Buffer[i].data[0]<=0xbf) && Reception_Buffer[i].data[0]!=0 && flag==1){
				base_offset=reassemble_last_segment(i,base_offset);
		 		break;
				//printf("last\n");	
			}
		}						
	}  
}

uint8_t highest_SN(uint8_t RX_Next_Highest){
	for(int i=0;i<=RB_index;i++){
		if((Reception_Buffer[i].data[0] & 0x3f)>RX_Next_Highest)
			RX_Next_Highest=(Reception_Buffer[i].data[0] & 0x3f);					
	}
	return RX_Next_Highest;
}

uint16_t min_offset(uint16_t base_offset){
	for(int i=0;i<=RB_index;i++){
		if((Reception_Buffer[i].data[0] & 0x3f)==RX_Next_Reassembly && ((Reception_Buffer[i].data[0]>=0xc0) && (Reception_Buffer[i].data[0]<=0xff)) || 				((Reception_Buffer[i].data[0]>=0x80) && (Reception_Buffer[i].data[0]<=0xbf))){
			int SO;	
			SO=Reception_Buffer[i].data[2];
			SO=(SO << 8) | Reception_Buffer[i].data[1];
			if(SO<=base_offset)
				base_offset=SO;	
		}
	}
	return base_offset;
}
int reassemble_first_segment(int i,int flag){
	printf("reassembled packet with SN %d\n",RX_Next_Reassembly);
	for(int j=1;j<SEGMENT_SIZE;j++)
		printf("%x\n",Reception_Buffer[i].data[j]);
	Reception_Buffer[i].data[0]=0;
	flag=1;
	return flag;
}

int reassemble_middle_segments(int i,int base_offset){
	int SO=0;
	SO=Reception_Buffer[i].data[2];
	SO=(SO << 8) | Reception_Buffer[i].data[1];
//	printf("base_offset : %x\nso : %x\n",base_offset,SO);			
	if(SO<=base_offset){
		for(int j=3;j<SEGMENT_SIZE;j++)
			printf("%x\n",Reception_Buffer[i].data[j]);
		base_offset=base_offset+(SEGMENT_SIZE-3);
		Reception_Buffer[i].data[0]=0;
	}
	return base_offset;
}

int reassemble_last_segment(int i,int base_offset){
	int SO=0;
	SO=Reception_Buffer[i].data[2];
	SO=(SO << 8) | Reception_Buffer[i].data[1];
//	printf("base_offset : %x\nso : %x\n",base_offset,SO);				
	if(SO<=base_offset){
		for(int j=3;j<last_segment[RX_Next_Reassembly]+3;j++)
			printf("%x\n",Reception_Buffer[i].data[j]);
		base_offset=base_offset+last_segment[RX_Next_Reassembly];
		Reception_Buffer[i].data[0]=0;
	}
	return base_offset;		    
}
