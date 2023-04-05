#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#define QUEUE_SIZE 8
int openListen(char* service, int queueSize)
{

}
int main(int argc, char **argv)
{
 struct sockaddr_storage remote_host;
 socklen_t remote_host_len;
 if (argc != 2)
 {
  printf("Port Number Required\n");
  exit(EXIT_FAILURE);
 }
 char *service = argv[1];
 int listern = openListen(service, QUEUE_SIZE);
}
