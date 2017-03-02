/*
 *  otp_enc.c
 *
 *  Created on: Mar 5, 2016
 *  Author: Susie Kuretski
 *
 *  Implementation of a simple decoding/encoding program using one-time pad encryption.
 *  See otp_dec.c for decoding. Relies upon otp_enc_d.c as a "daemon" which
 *  does the actual encryption.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


#define BUFFER_SIZE 1000
#define MAX_SIZE 70000

long getFileSize(FILE *fp);
void readFile(char *output, FILE *fp, long length);
void sendString(int sockfd, char *fileString, long fileSize);
void sendNum(int sockfd, long num);
void rec_msg(int sockfd, char *output, long size);
long rec_num(int sockfd);

int main(int argc, char *argv[])
{
    int sockfd, portno, n, c;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char *handshake = "enc_d";
	FILE *fp_text, *fp_key;
	long keyLength, plainLength, cipherLength;
    char *key, *plain, *cipher;
	char buffer[BUFFER_SIZE];
    if (argc < 4) {
       printf("%s%s%s\n", "Usage: ", argv[0], "[plain text file] [encryption key] [port number]");
       exit(1);
    }

	//Change 4th argument to port number
    portno = atoi(argv[3]);
	
	//Open files for reading
	fp_text = fopen(argv[1], "r");
	fp_key = fopen(argv[2], "r");
	if(fp_text == NULL || fp_key == NULL){
		perror("open");
		exit(1);
	}
	//Get file sizes
	keyLength = getFileSize(fp_key);
	plainLength = getFileSize(fp_text);
	if(keyLength < plainLength){
		fprintf(stderr, "Error: key is shorter than the text\n");
		exit(1);
	}

	//Allocate memory for file -> C-string arrays
	key = (char*)malloc(keyLength);
	plain = (char*)malloc(plainLength);

	//Check for bad input
    while((c = fgetc(fp_text)) != EOF){
		if((c < 'A' || c > 'Z') && c != ' '){
			if (c != '\n'){
				fprintf(stderr, "Error: input contains bad characters\n");
				exit(1);
				break;
			}
		}	
	}
	
	//Read file into char array
	fseek(fp_text, 0, SEEK_SET);	
	readFile(key, fp_key, keyLength);
	readFile(plain, fp_text, plainLength);

	//Strip newline
	plain[plainLength] = '\0';
	
	//Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("socket");

	//Set server information
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }
	//Clear server address and server address fields
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    //Connect to server
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        perror("connect");
	//Handshake with server
    bzero(buffer, sizeof(buffer));
	n = write(sockfd, handshake, sizeof(handshake));
	if(n < 0) perror("write");
	n = read(sockfd, buffer, BUFFER_SIZE - 1);
	if(n < 0)
		perror("read");

	//Send file contents	
	sendNum(sockfd, plainLength);
	sendString(sockfd, plain, plainLength);
	sendNum(sockfd, keyLength);
	sendString(sockfd, key, keyLength);
    
	//Receive cipher back
	cipherLength = rec_num(sockfd); 
	cipher = (char*)malloc(cipherLength);
	rec_msg(sockfd, cipher, cipherLength);
	long i;
	for(i = 0; i < cipherLength; i++){
		printf("%c", cipher[i]);
	}

	//Cleanup
	close(sockfd);
	fclose(fp_text);
	fclose(fp_key);
	free(key);
	free(plain);
	free(cipher);
    return 0;
}
//******************************   FUNCTIONS **************************************
/* Description: returns file size and rewinds to beginning of file
 * Parameters: file pointer
 * Returns: long int of file size
 */
long getFileSize(FILE *fp){
	long size;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return size;
}
/* Description: sends a c-string of data
 * Parameters: socket descriptor, c-string, and c-string size
 * Post-condition: sends c-string utilizing socket and breaks up
 * data into packets to ensure whole c-string transmission
 */
void sendString(int sockfd, char *fileString, long fileSize){
	long bytesSent = 0;
	int n;
	while(bytesSent < fileSize){
		n = write(sockfd, fileString + bytesSent, fileSize - bytesSent);
		bytesSent += n;
		if(n < 0) perror("write");
		else if(n == 0)
			bytesSent = fileSize - bytesSent;
	}				
}
/* Description: sends a long int via socket
 * Parameters: socket descriptor, and long int to send
 * Post-condition: sends long int utilizing socket
 */
void sendNum(int sockfd, long num){
	long tempNum = num;
	int n;
	n = write(sockfd, &tempNum, sizeof(long));
	if(n < 0) perror("write");
}
/* Description: reads a file in and puts it into a c-string
 * Parameters: c-string (output), file descriptor, long of file length
 * Post-condition: reads in file into c-string called output
 */
void readFile(char *output, FILE *fp, long length){
	long numCopy;
	numCopy = fread(output, 1, length, fp); 
	if(numCopy != length){
		perror("file read in");
	}
}
/* Description: receives long int from socket and returns it
 * Parameters: socket descriptor
 * Returns: long int read from socket
 */
long rec_num(int sockfd){
	long n, num;
	n = read(sockfd, &num, sizeof(num));
	if(n < 0) perror("read num");
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
	long n, bytesRec;
	bytesRec = 0;
	while(bytesRec < size){
		n = read(sockfd, buffer + bytesRec, size - bytesRec);
		bytesRec += n;
		if(n < 0) perror("write msg");
		else if(n == 0)
			bytesRec = size - bytesRec;
	}
	strncpy(output, buffer, size);
}


