/*
 *  otp_dec_d.c
 *
 *  Created on: Mar 5, 2016
 *  Author: Susie Kuretski
 *  Some of the following code is copied verbatim from:
 *  www.linuxhowtos.org/C_C++/socket.htm
 *
 *  This program is a simple implementation of decryption using
 *  one-time pad-like system utilizing sockets for inter-process
 *  communcation and multiple processes. Used in conjunction with
 *  otp_dec.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256

char int_to_char(int n);
int char_to_int(char c);
void decrypt(char *plainText, char *key, long length);
void rec_msg(int sockfd, char *output, long size);
long rec_num(int sockfd);
void send_num(int sockfd, long num);
void send_msg(int sockfd, char *input, long size);

int main(int argc, char *argv[])
{
     int sockfd, newsockfd;
     int optval;
     int portno;
	 int n;
     socklen_t clilen;
     char buffer[BUFFER_SIZE];
	 char tempBuffer[BUFFER_SIZE];
	 char *text, *key, *response;
	 long fileLength, keyLength;
     struct sockaddr_in serv_addr, cli_addr;
     int spawnpid, status;

     if (argc < 2) {
         fprintf(stderr,"Usage: %s [port number]\n", argv[0]);
         exit(1);
     }
	 //Create socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0){
        perror("socket");
     	exit(1);
     }
     if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
    	perror("setsockopt");
    	exit(1);
     }
	 //Clear server address
     bzero((char *) &serv_addr, sizeof(serv_addr));

     //Change argument of port number to int
	 portno = atoi(argv[1]);

     //Set server address fields
	 serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

	 //Bind socket
     if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    	 perror("bind fail");
     	 exit(1);
     }

	 //Listen for connections (queue max of 5)
     listen(sockfd,5);
     if(listen(sockfd, 5) == -1){
    	 perror("listen");
    	 exit(1);
     }
     while(1){
    	 clilen = sizeof(cli_addr);
		 //Accept connection and use new socket
		 newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
    	 if (newsockfd < 0){
    		 perror("accept");
    		 exit(1);
    	 }
		 //Create child process 
    	 spawnpid = fork();
    	 switch(spawnpid){
    	 case -1:
    		 close(newsockfd);
    		 perror("fork");
    		 exit(1);
    		 break;
		 //Child process
    	 case 0:
    		 close(sockfd);		//close socket that is listening
			 //Handshake client
			 bzero(buffer, sizeof(buffer));
    	     n = read(newsockfd,buffer,BUFFER_SIZE - 1);
    	     if (n < 0)
    	    	 perror("read");
			 if(strcmp(buffer, "dec_d") != 0){
		   	 	response = "invalid";
			 	write(newsockfd, response, sizeof(response));
				exit(1);
			 }else{
				response = "dec_d";
				n = write(newsockfd, response, sizeof(response));
			    if (n < 0)
    	    		perror("write");
			 }
			 //Receive input
			 bzero(buffer, sizeof(buffer));
			 fileLength = rec_num(newsockfd);
			 text = (char*)malloc(fileLength);
			 rec_msg(newsockfd, text, fileLength);
		
			 keyLength = rec_num(newsockfd);
			 key = (char*)malloc(keyLength);
			 rec_msg(newsockfd, key, keyLength);

		     //Decode
			 decrypt(text, key, fileLength);
	
			 //Send decoded message back
			 send_num(newsockfd, fileLength);
			 send_msg(newsockfd, text, fileLength);

			 //Cleanup
			 close(newsockfd);
			 free(key);
			 free(text);
		     exit(0);
    		 break;
    	 default:
			 close(newsockfd);
   			 wait(NULL);
    	 }
     }
	 close(sockfd);
     return 0;
}
/* Description: changes a char to an int; a space character is
 * 26
 * Parameters: char
 * Returns: int
 */
int char_to_int(char c){
	if(c == ' ')
		return 26;
	else
		return (c - 'A');
}
/* Description: changes an int to a char; a space character
 * is 26
 * Parameters: int
 * Returns: char
 */
char int_to_char(int n){
	if(n == 26)
		return ' ';
	else
		return ('A' + n);
}

/* Description: decodes text from key
 * Parameters: c-string of text to be decoded, c-string of key,
 * and long of c-string length
 * Post-condition: decoded text based on key; results will be in
 * text itself
 */
void decrypt(char *text, char *key, long length){
	char x;
	int i;
	for(i = 0; i < length-1; i++){
		x = char_to_int(text[i]) - char_to_int(key[i]);
		if(x < 0)
			x += 27;
		text[i] = int_to_char(x);
	}
}
/* Description: receives long int from socket and returns it
 * Parameters: socket descriptor
 * Returns: long int read from socket
 */
long rec_num(int sockfd){
	long num;
	int n;
	n = read(sockfd, &num, sizeof(num));
	if(n < 0) perror("num rec read");
	else
		return num;
}
/* Description: receives c-string from socket
 * Parameters: socket descriptor, c-string (output) to store read
 * data into, and long int of file/c-string size
 * Post-condition: receives c-string from socket and stores into
 * another c-string (output)
 */
void rec_msg(int sockfd, char *output, long size){
	char buffer[70000];
	long tempSize = size;
	long n, bytesRec = 0;
	while(bytesRec < size){
		n = read(sockfd, buffer+bytesRec, tempSize-bytesRec);
		bytesRec += n;
		if(n < 0) perror("string rec read");
		else if(n == 0)
			bytesRec = tempSize - bytesRec;
	}
	strncpy(output, buffer, size);
}
/* Description: sends a c-string of data
 * Parameters: socket descriptor, c-string, and c-string size
 * Post-condition: sends c-string utilizing socket and breaks up
 * data into packets to ensure whole c-string transmission
 */
void send_msg(int sockfd, char *input, long size){
	long n, tempSize, bytesSent;
	tempSize = size;
	bytesSent = 0;
	while(bytesSent < tempSize){
		n = write(sockfd, input + bytesSent, tempSize - bytesSent);
		bytesSent += n;
		if(n < 0) perror("send msg");
		else if(n == 0)
			bytesSent = tempSize - bytesSent;

	}
}
/* Description: sends a long int via socket
 * Parameters: socket descriptor, and long int to send
 * Post-condition: sends long int utilizing socket
 */
void send_num(int sockfd, long num){
	long tempNum;
	int n;
	tempNum = num;
	n = write(sockfd, &tempNum, sizeof(tempNum));
	if(n < 0) perror("send num");
}

