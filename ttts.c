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
typedef enum condition
{
  xwin,
  owin,
  draw,
  unfinished
} condition;
char *board, **usernames;
int ucount;
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
int makeMove(int row, int col, char player)
{
  row--;
  col--;
  if((row > 2) || (row < 0) || (col < 0) || (col > 2))
    return -1;
  int index = row * 3 + col;
  if(board[index] != '.')
    return -2;
  board[index] = player;
  condition a1 = checkStatus();
  if(a1 == unfinished)
    return 1;
  else
    return 2;
}
char *interpretCommand(char *command, char *text, char player)
{
  if (strcmp("PLAY", command) == 0)
  {
    usernames[ucount] = text;
    ucount++;
    return "WAIT|0|";
  }
  if (strcmp("MOVE", command) == 0)
  {
    if (strlen(text) < 3)
      return NULL;
    else
    {
      int row = text[0] - '0';
      int col = text[2] - '0';
      int success = makeMove(row, col, player);
      if(success == 1)
      {
        return 
      }
      else
      {

      }
    }
  }
  if (strcmp("RSGN", command) == 0)
  if (strcmp("DRAW", command) == 0)
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
  char *text = (char *)malloc(sizeof(char) * num);
  count = i + 1;
  for (i = count; string[i] != '|'; i++)
  {
    if (((i - count) >= num) || i >= length)
    {
      format = 0;
      break;
    }
    text[i - count] = string[i];
  }
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
condition checkWin2(char c)
{
  if (c == 'X')
    return xwin;
  else
    return owin;
}
condition checkStatus()
{
  short draw = 1;
  if ((board[0] == board[1]) && (board[1] == board[2]) && (board[0] != '.'))
    return checkWin2(board[0]);
  if ((board[3] == board[4]) && (board[4] == board[5]) && (board[3] != '.'))
    return checkWin2(board[3]);
  if ((board[6] == board[7]) && (board[7] == board[8]) && (board[6] != '.'))
    return checkWin2(board[6]);
  if ((board[0] == board[3]) && (board[3] == board[6]) && (board[0] != '.'))
    return checkWin2(board[0]);
  if ((board[1] == board[4]) && (board[4] == board[7]) && (board[1] != '.'))
    return checkWin2(board[1]);
  if ((board[2] == board[5]) && (board[5] == board[8]) && (board[2] != '.'))
    return checkWin2(board[2]);
  if ((board[0] == board[4]) && (board[4] == board[8]) && (board[0] != '.'))
    return checkWin2(board[0]);
  if ((board[2] == board[4]) && (board[4] == board[6]) && (board[2] != '.'))
    return checkWin2(board[2]);
  for (int i = 0; i < 9; i++)
    if (board[i] == '.')
      draw = 0;
  if (draw)
    return draw;
  return unfinished;
}
char *setBoard()
{
  board = (char *)malloc(sizeof(char) * 10);
  for (int i = 0; i < 9; i++)
  {
    board[i] = '.';
  }
  board[9] = '\0';
}
int main(int argc, char **argv)
{
  struct sockaddr_storage remote_host;
  socklen_t remote_host_len;
  usernames = (char **)malloc(sizeof(char *) * 2);
  ucount = 0;
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
