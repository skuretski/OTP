/*  keygen.c
 *
 *  Created on: Mar 5, 2016
 *  Author: Susie Kuretski
 *
 *  This program creates a key of random letters and space char(s).
 *  Output is printed to stdout.
 *  Command line usage: [program name] [specified length of key]
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char *argv[]){
	if(argc != 2){
		printf("%s%s%s\n", "Usage: ", argv[0], " length");
		exit(0);
	}
	srand(time(NULL));
	int i, length;
	char key[length+1];
	char randLetter;
	length = atoi(argv[1]);
	for(i = 0; i < length; i++){
		//Credit: http://stackoverflow.com/questions/19724346/generate-random-characters-in-c
		randLetter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "[rand() % 27];
		key[i] = randLetter;
		printf("%c", key[i]);
	}
	printf("\n");
	fflush(stdout);
	return 0;
}
