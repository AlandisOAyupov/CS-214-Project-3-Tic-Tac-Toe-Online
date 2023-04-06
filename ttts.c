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
#define BUFSIZE 256
#define HOSTSIZE 100
#define PORTSIZE 10
volatile int active = 1;
int openListen(char *service, int queueSize)
{
 struct addrinfo hint, *info_list, *info;
 int error, sock;
 memset(&hint, 0, sizeof(struct addrinfo));
 hint.ai_family = AF_UNSPEC;
 hint.ai_socktype = SOCK_STREAM;
 hint.ai_flags = AI_PASSIVE;
 error = getaddrinfo(NULL, service, &hint, &info_list);
 if (error)
 {
  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
  return -1;
 }
 for (info = info_list; info != NULL; info = info->ai_next)
 {
  sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sock == -1)
   continue;
  error = bind(sock, info->ai_addr, info->ai_addrlen);
  if (error)
  {
   close(sock);
   continue;
  }
  error = listen(sock, queueSize);
  if (error)
  {
   close(sock);
   continue;
  }
  break;
 }
 freeaddrinfo(info_list);
 if (info == NULL)
 {
  fprintf(stderr, "Could not bind\n");
  return -1;
 }
 return sock;
}
char *formatMessage(char *string, int length)
{
 short format = 1;
 int i = 0;
 char *command = (char *)malloc(sizeof(char) * 5);
 for (i = 0; string[i] != '|'; i++)
 {
  if (i >= 4 || i >= length)
  {
   format = 0;
   break;
  }
  command[i] = string[i];
 }
 command[4] = '\0';
 char *number = (char *)malloc(sizeof(char) * 4);
 int count = i + 1;
 for (i = count; string[i] != '|'; i++)
 {
  if (((i - count) >= 3) || (i >= length) || string[i] < 48 || string[i] > 57)
  {
   format = 0;
   break;
  }
  number[i - count] = string[i];
 }
 number[3] = '\0';
 int num = atoi(number);
}
void playGame(int sock, int sock2, struct sockaddr *rem, socklen_t rem_len)
{
 char buf[BUFSIZE];
 int bytes;
 while (active && (bytes = read(sock, buf, BUFSIZE)) > 0)
 {
  write(STDIN_FILENO, buf, bytes);
 }
 close(sock);
 close(sock2);
}
int main(int argc, char **argv)
{
 struct sockaddr_storage remote_host;
 socklen_t remote_host_len;
 char *service = argc == 2 ? argv[1] : "15000";
 int listener = openListen(service, QUEUE_SIZE);
 if (listener < 0)
  exit(EXIT_FAILURE);
 puts("Listening for incoming connections");
 while (active)
 {
  remote_host_len = sizeof(remote_host);
  int sock = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
  int sock2 = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
  if (sock < 0 || sock2 < 0)
  {
   perror("accept");
   continue;
  }
  playGame(sock, sock2, (struct sockaddr *)&remote_host, remote_host_len);
 }
 puts("Shutting down");
 close(listener);
 return EXIT_SUCCESS;
}
