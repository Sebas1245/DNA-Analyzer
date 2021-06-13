#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8888

int main(int argc, char const *argv[]) {
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client";
  char clientReply[1024] = {0};
  char serverMsg[100024] = {0};
  char fileName[100];
  int iOpt;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  printf("Connection established\n");

  printf("Welcome to DNAAnalyzer!\n");
  while (1) {
    memset(serverMsg, 0, sizeof(serverMsg));
    printf(
        "What would you like to do? \n 1) Upload a reference \n 2) Upload a "
        "sequence \n Type '1' or '2'\n");
    scanf("%d", &iOpt);

    printf("Name of the file? (Include extension) ");
    scanf("%s", fileName);
    char filepath[1000] = "test_files/";
    strcat(filepath, fileName);

    if (access(filepath, F_OK) != 0) {
      printf("File doesn't exist\n");
      continue;
    }

    if (iOpt == 1) {
      strcat(filepath, "R");
    } else if (iOpt == 2) {
      strcat(filepath, "S");
    }

    send(sock, filepath, sizeof(fileName), 0);
    if (recv(sock, serverMsg, sizeof(serverMsg), 0) > 0) {
      printf("Server: %s\n", serverMsg);
    }
  }

  return 0;
}