#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
pthread_t t2;

static int clientSocket, portNum, nBytes;
static char buffer[BUFSIZE];
struct sockaddr_in serverAddr;
socklen_t addr_size;
void UDP_send_message(char *buf) {
	int bufSendSize = strlen(buf);
	sendto(clientSocket,buf,bufSendSize,0,(struct sockaddr *)&serverAddr,addr_size);
}


void UDP_client() {
  printf("Starting UDP client\n");

  /*Create UDP socket*/
  clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

  /*Configure settings in address struct*/
  const char* server_name = "169.254.0.2";
  portNum = 12346;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(portNum);
  inet_pton(AF_INET, server_name, &serverAddr.sin_addr);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*Initialize size variable to be used later on*/
  addr_size = sizeof serverAddr;

  while(1){
    printf("Type a sentence to send to server:\n");
    fgets(buffer,1024,stdin);
    printf("You typed: %s",buffer);

    nBytes = strlen(buffer) + 1;
    
    /*Send message to server*/
    sendto(clientSocket,buffer,nBytes,0,(struct sockaddr *)&serverAddr,addr_size);

    /*Receive message from server*/
                nBytes = recvfrom(clientSocket,buffer,1024,0,NULL, NULL);

    printf("Received from server: %s\n",buffer);

  }
}

void UDP_client_init() {
    int ret3 = pthread_create(&t2, NULL, (void *) &UDP_client, NULL);
    if(ret3) {
        fprintf(stderr, "Error - pthread_create() return code: %d\n", ret3);
    }
}
