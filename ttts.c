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
typedef struct game
{
  char **usernames, **messages, *sides, *board;
  int *sock, ucount;
} game;
char *board, *sides, **usernames, **messages;
int ucount, *sock;
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
char *combine(char *first, char *second)
{
  int i = 0;
  char *combination = (char *)malloc(sizeof(char) * (strlen(first) + strlen(second) + 1));
  int length = strlen(first) + strlen(second) + 1;
  for (i = 0; i < strlen(first); i++)
		combination[i] = first[i];
 int count = 0;
 while (count < strlen(second))
 {
		combination[i] = second[count];
		i++;
		count++;
 }
 combination[length - 1] = '\0';
 return combination;
}
void chooseSides()
{
 int r = rand() % 2;
 if(r == 1)
 {
  sides[0] = 'X';
  sides[1] = 'O';
 }
 else
 {
  sides[0] = 'O';
  sides[1] = 'X';
 }
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
int getPlayerFromToken(char player, int c)
{
  for (int i = 0; i < 2; i++)
  {
    if(sides[i] == player && c == 0)
      return i;
    if(sides[i] != player && c == 1)
      return i;
  }
}
char* intToString(int p)
{
  return NULL;
}
char* xHasResigned(int w)
{
  char *one = combine(usernames[w], " has resigned|");
  char *two = combine("W|", one);
  free(one);
  int l = strlen(two);
  char *three = intToString(l);
  char *four = combine(three, two);
  free(two);
  free(three);
  char *five = combine("OVER|", four);
  free(four);
  return five;
}
void *interpretCommand(char *command, char *text, char player)
{
  int w;
  if (strcmp("PLAY", command) == 0)
  {
    usernames[ucount] = text;
    ucount++;
    w = getPlayerFromToken(player, 0);
    messages[w] = "WAIT|0|";
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

      }
      else
      {

      }
    }
  }
  if (strcmp("RSGN", command) == 0)
  {
    w = getPlayerFromToken(player, 0);
    messages[w] = "OVER|21|L|You have resigned|";
    w = getPlayerFromToken(player, 1);
    messages[w] = xHasResigned(w);
  }
  if (strcmp("DRAW", command) == 0)
  {
    if(strcmp("S", text) == 0)
    {
      w = getPlayerFromToken(player, 1);
      messages[w] = "DRAW|2|S|";
    }
    if(strcmp("A", text) == 0)
    {
      messages[0] = "OVER|18|Draw agreed upon|";
      messages[1] = "OVER|18|Draw agreed upon|";
    }
    if(strcmp("R", text) == 0)
    {
      w = getPlayerFromToken(player, 1);
      messages[w] = "DRAW|2|R|";
    }
  }
}
char *formatMessage(char *string, int length, char player)
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
  free(number);
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
  return interpretCommand(command, text, player);
}
void playGame(int sock, int sock2, struct sockaddr *rem, socklen_t rem_len)
{
  char buf[BUFSIZE];
  int bytes;
  usernames = (char **)malloc(sizeof(char *) * 2);
  messages = (char **)malloc(sizeof(char *) * 2);
  sides = (char *)malloc(sizeof(char) * 2);
  chooseSides();
  while (active && (bytes = read(sock, buf, BUFSIZE)) > 0)
  {
    write(STDIN_FILENO, buf, bytes);
  }
  close(sock);
  close(sock2);
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
  sock = (int *)malloc(sizeof(int) * 2);
  ucount = 0;
  char *service = argc == 2 ? argv[1] : "15000";
  int listener = openListen(service, QUEUE_SIZE);
  if (listener < 0)
    exit(EXIT_FAILURE);
  puts("Listening for incoming connections");
  while (active)
  {
    remote_host_len = sizeof(remote_host);
    sock[0] = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
    sock[1] = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
    if (sock[0] < 0 || sock[1] < 0)
    {
      perror("accept");
      continue;
    }
    playGame(sock[0], sock[1], (struct sockaddr *)&remote_host, remote_host_len);
  }
  puts("Shutting down");
  close(listener);
  return EXIT_SUCCESS;
}
