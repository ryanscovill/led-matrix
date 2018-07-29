#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include "udpserver.h"
#include <math.h>

#define BUFSIZE 1024
#define SHORT_MSG_BUFFER 40
#define NUM_ARGS 2
#define BITS_FOR_ARR_TO_STR 15 //32-bit INT = 12 bits + 2 bits for ',' & ' ' + 1 

_Bool keepRunFlag = true;
pthread_t t2;
static int sockfd;
static int portno;
socklen_t addr_size;
struct sockaddr_in serveraddr;
struct sockaddr_in clientaddr;
static char *buf;
static char *bufCpy;
static char *bufArr[NUM_ARGS];
static int reset;
static int n;
static int argStr2Int;
static char *ptr;
static int playerID;
pthread_mutex_t copyLock;

int sendDatagram(char *inputStr) {
    int inputLen = strlen(inputStr) + 1; //+1 for additional null terminating character(s)
    int q = sendto(sockfd, inputStr, inputLen, 0,
                (struct sockaddr *)&clientaddr, addr_size);
    if (q < 0) { perror("ERROR in sendto"); }
    return q;
}


void appToEnd(char *inputStr, char lineBreak) {
    int inputLen = strlen(inputStr);
    inputStr[inputLen] = lineBreak;
    inputStr[inputLen+1] = '\0';
}

void UDP_stop() {
    pthread_join(t2, NULL);
    keepRunFlag = false;
}

void UDP_server() {
	if (playerID == 1) {
		portno = 12345;
	} else {
		portno = 12346;
	}

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR OPENING SOCKET!\n");
        exit(1);
    }

    //Reset port & server when killing program
    reset = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reset, sizeof(int));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);
    addr_size = sizeof(clientaddr);

    if(bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("ERROR BINDING PORT\n");
        exit(1);
    }

    char *help = "I am a help message";
    char *invalidMsg = "I am an invalid message";


    //------------------------------------------------------
    buf = malloc((BUFSIZE+1) * sizeof(char)); // +1 for additioinal terminating characters 
    bufCpy = malloc((BUFSIZE+1) * sizeof(char));
    	while (keepRunFlag) {
		memset(buf, '\0', BUFSIZE); //Clear buffer / fill it with null terminating characters
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			     (struct sockaddr *)&clientaddr, &addr_size);
		if (n < 0) { perror("ERROR in recvfrom"); }

		printf("server received %d bytes\n", n);

        //Parse buffer
        pthread_mutex_lock(&copyLock);
        strcpy(bufCpy, buf);
        pthread_mutex_unlock(&copyLock);
        char *p = strtok (bufCpy, " ");       
        bufArr[0] = p;
        p = strtok (NULL, " ");
        bufArr[1] = p;

        // printf("Tokenizer output:\n");
        // printf("[0] = %s\n", bufArr[0]);
        // printf("[1] = %s\n", bufArr[1]);

        //Conversion of output
        if(bufArr[1] != NULL) {
            argStr2Int = (int) strtol(bufArr[1], &ptr, 10);
        }

        if(strcmp(buf, "help\n") == 0) {
            //printf("Help command recognized!\n");
            n = sendDatagram(help);
        } else {
            //printf("Invalid command!\n");
            n = sendDatagram(invalidMsg);
        }
	}
    free(bufCpy);
    free(buf);
}

char* UDP_get_buff() {
    return bufCpy;
}


void UDP_init(int player) {
	playerID = player;
    if (pthread_mutex_init(&copyLock, NULL) != 0) {
        printf("Init lock has failed\n");
    }
    int ret2 = pthread_create(&t2, NULL, (void *) &UDP_server, NULL);
    if(ret2) {
        fprintf(stderr, "Error - pthread_create() return code: %d\n", ret2);
    }
}