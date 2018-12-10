//2017320229.c
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>

#define SERVER_PORT 47500
#define FLAG_HELLO ((unsigned char)(0x01<<7))
#define FLAG_INSTRUCTION ((unsigned char)(0x01<<6))
#define FLAG_RESPONSE ((unsigned char)(0x01<<5))
#define FLAG_TERMINATE ((unsigned char)(0x01<<4))

#define OP_ECHO ((unsigned char)(0x00))
#define OP_INCREMENT ((unsigned char)(0x01))
#define OP_DECREMENT ((unsigned char)(0x02))
#define OP_PUSH ((unsigned char)(0x03))
#define OP_DIGEST ((unsigned char)(0x04))

struct hw_packet{
	unsigned char flag; //HIRT-4bits, reserved-4bits
	unsigned char operation; //8-bits operation
	unsigned short data_len; //16 bits (2 bytes) data length
	unsigned int seq_num; //32 bits (4bytes) sequence number
	char data[1024]; //optional data
};

int main(){
	struct sockaddr_in sin;
	int s;
	//count received packet.
	int count=0;
	char new_data[1024];
	//HELLO packet
	unsigned int value;
	struct hw_packet buf_struct;
	struct hw_packet buf_struct_rcv;
	char file_rev[12288];
	buf_struct.flag=FLAG_HELLO;
	buf_struct.operation=OP_ECHO;
	buf_struct.data_len=4;
	buf_struct.seq_num=0;
	value=2017320229;
	memcpy(buf_struct.data, &value, sizeof(unsigned int));

	/* build address data structure */
	//bzero((char *)sin, sizeof(sin));
	memset(&sin, 0, sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_port=htons(SERVER_PORT);
	sin.sin_addr.s_addr=inet_addr("127.0.0.1");

	printf("*** starting ***\n\n");
	
	/* active open */
	/* socket for accessing server */
    if((s=socket(PF_INET,SOCK_STREAM,0)) < 0 ){
		perror("simplex-talk: socket");
		exit(1);
	}
	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("simplex-talk: connect");
		close(s);
		exit(1);
	}

	/*send HELLO packet.*/
	printf("sending first hello msg...\n");
	if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
		perror("simplex-talk: send HELLO packet");
		close(s);
		exit(1);
	}	
	++count;

	/*iterate until the server sends TERMINATION packet.*/
	while(1){
		/*receive packet.*/
		if(recv(s, &buf_struct_rcv, sizeof(buf_struct),0)<0){
			perror("simplex-talk: receive instruction packet");
			printf("%d",count);
			close(s);
			exit(1);
		}
		++count;
		/*parser received data.*/
		switch(buf_struct_rcv.flag){
			//parse HELLO packet data
			case FLAG_HELLO:
				printf("received hello message from the server\n");
				printf("waiting for the first instruction message...\n\n");
				break;
			
			/*parse instruction packet data*/
			case FLAG_INSTRUCTION:
				printf("\nreceived instruction message! received data_len : %u bytes\n", buf_struct_rcv.data_len);
				if(buf_struct_rcv.operation==OP_ECHO){
					printf("operation type is echo.\n");
					/*make packet be same with instruction packet for respond*/
					buf_struct.flag=FLAG_RESPONSE;
					buf_struct.operation=OP_ECHO;
					buf_struct.data_len=buf_struct_rcv.data_len;
					buf_struct.seq_num=buf_struct_rcv.seq_num;
					printf("str len : %u", buf_struct_rcv.data_len);
					//buf_struct.data[0]="\0";
					memcpy(buf_struct.data, buf_struct_rcv.data, buf_struct_rcv.data_len);
					if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
						perror("simplex-talk: send ECHO packet");
						close(s);
						exit(1);
					}	
					/*send echo packet*/
					printf("echo : %s\n", buf_struct.data);
					printf("sent response msg with seq.num. %u to server\n",buf_struct.seq_num);
				}
				else if(buf_struct_rcv.operation==OP_DECREMENT){
					printf("operation type is decrement.\n");
					buf_struct.flag=FLAG_RESPONSE;
					buf_struct.seq_num=buf_struct_rcv.seq_num;
					buf_struct.data_len=4;
					buf_struct.operation=OP_ECHO;
					//buf_struct.data[0]="\0";
					memcpy(&value, buf_struct_rcv.data, sizeof(unsigned int));
					--value;
					memcpy(buf_struct.data, &value, sizeof(unsigned int));
					if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
						perror("simplex-talk: send DECREMENT packet");
						close(s);
						exit(1);
					}
					printf("decrement : %u\n",value);
					printf("sent response msg with seq.num. %u to server\n\n", buf_struct.seq_num);
				}
				else if(buf_struct_rcv.operation==OP_INCREMENT){
					printf("operation type is increment.\n");
					buf_struct.flag=FLAG_RESPONSE;
					buf_struct.seq_num=buf_struct_rcv.seq_num;
					buf_struct.data_len=4;
					buf_struct.operation=OP_ECHO;
					//buf_struct.data[0]="\0";
					memcpy(&value, buf_struct_rcv.data, sizeof(unsigned int));
					++value;
					memcpy(buf_struct.data, &value, sizeof(unsigned int));
					if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
						perror("simplex-talk: send INCREMENT packet");
						close(s);
						exit(1);
					}
					printf("increment : %u\n",value);
					printf("sent response msg with seq.num. %u to server\n\n", buf_struct.seq_num);
					
				}
				else if(buf_struct_rcv.operation==OP_PUSH){
					//printf("operation type is PUSH and the first byte of data is [%u]",buf_struct_rcv.seq_num);
					printf("received push instruction!!\n");
					printf("received seq_num: %u\n", buf_struct_rcv.seq_num);
					printf("received data_len: %u\n", buf_struct_rcv.data_len);
					//printf("real file seq_num: %d\n", strlen(file_rev));
					printf("saved bytes from index %u to %u\n",strlen(file_rev),strlen(file_rev)+buf_struct_rcv.data_len-1);
					buf_struct.flag=FLAG_RESPONSE;
					buf_struct.seq_num=0;
					buf_struct.operation=OP_PUSH;
					buf_struct.data_len=0;
                    memmove(&value, &buf_struct_rcv.seq_num, sizeof(unsigned int));   //start address of data segment
                    memmove(file_rev+value, buf_struct_rcv.data, sizeof(char)*buf_struct_rcv.data_len);
                    //file_rev[strlen(file_rev)]='\0';
					//strcat(file_rev, buf_struct_rcv.data);
					if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
						perror("simplex-talk: send INCREMENT packet");
						close(s);
						exit(1);
					}
					file_rev[buf_struct_rcv.seq_num+buf_struct_rcv.data_len]='\0';
					printf("saved byte stream (character representation) : %s\n", file_rev+value);
					printf("current file size is : %d\n\n",strlen(file_rev));
				}
                else if(buf_struct_rcv.operation==OP_DIGEST){
					printf("received digest instruction!!\n");
					printf("********** calculated digest **********\n");
                    buf_struct.flag=FLAG_RESPONSE;
					buf_struct.seq_num=0;
					buf_struct.operation=OP_DIGEST;
                    SHA1(buf_struct.data, file_rev, strlen(file_rev));
					//printf("%s", buf_struct.data);
					printf("***************************************\n");
                    buf_struct.data_len=20;
                    if(send(s,&buf_struct,sizeof(buf_struct),0)<0){
						perror("simplex-talk: send INCREMENT packet");
						close(s);
						exit(1);
					}
					printf("sent digest message to server");
                }
				/*error*/
				else{
					perror("simplex-talk: parse received packet\n");
				}
				break;
			
			case FLAG_TERMINATE:
				printf("\nreceived terminate meg! terminating...\n");
				exit(1);
				break;

			default:
				perror("simplex-talk: parse received packe\n");
				break;
		}
	} 
	return 0;
}