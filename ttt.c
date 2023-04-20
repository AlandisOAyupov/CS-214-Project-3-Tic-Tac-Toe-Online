#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#define BUFLEN 1054
int sock;
int active = 1;
int connectSock(char *host, char *service)
{
  struct addrinfo hints, *info_list, *info;
  int sock, error;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  error = getaddrinfo(host, service, &hints, &info_list);
  if (error)
  {
    fprintf(stderr, "Error looking up %s:%s: %s\n", host, service, gai_strerror(error));
    return -1;
  }
  for (info = info_list; info != NULL; info = info->ai_next)
  {
    sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock < 0)
      continue;
    error = connect(sock, info->ai_addr, info->ai_addrlen);
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
    fprintf(stderr, "Unable to connect to %s:%s\n", host, service);
    return -1;
  }
  return sock;
}
char side(char *buf)
{
  int i = 0;
  while(buf[i] != '|')
    i++;
  i++;
  while(buf[i] != '|')
    i++;
  i++;
  return buf[i];
}
int cond(char *buf)
{
  if (buf[0] == 'O' && buf[1] == 'V' && buf[2] == 'E' && buf[3] == 'R')
    return 3;
  if (buf[0] == 'W' && buf[1] == 'A' && buf[2] == 'I' && buf[3] == 'T')
    return 2;
  if (buf[0] == 'M' && buf[1] == 'O' && buf[2] == 'V' && buf[3] == 'D')
    return 1;
  return 0;
}
void* writef()
{
  int bytes;
  char buf[BUFLEN];
  while ((bytes = read(STDIN_FILENO, buf, BUFLEN)) > 0)
  {
    buf[bytes] = '\0';
    write(sock, buf, bytes);
    if(active == 0)
      break;
  }
  return NULL;
}
void* readf()
{
  int bytes;
  char buf[BUFLEN];
  while ((bytes = read(sock, buf, BUFLEN)) > 0)
  {
    write(STDOUT_FILENO, buf, bytes);
    if (cond(buf) == 3)
    {
      active = 0;
      break;
    }
  }
  return NULL;
}
int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Host/Servive Required\n");
    exit(EXIT_FAILURE);
  }
  sock = connectSock(argv[1], argv[2]);
  if (sock < 0)
    exit(EXIT_FAILURE);
  pthread_t t1, t2;
  pthread_create(&t1, NULL, &readf, NULL);
  pthread_create(&t2, NULL, &writef, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  close(sock);
  return EXIT_SUCCESS;
}
