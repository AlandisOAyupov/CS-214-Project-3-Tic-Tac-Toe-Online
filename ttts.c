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
#include <time.h>
#define QUEUE_SIZE 8
#define BUFSIZE 1054
#define HOSTSIZE 100
#define PORTSIZE 10
int active = -2;
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
char **usernames, **messages, *board, *sides;
int *sock, *alloc, ucount, started, drawS;
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
  srand(time(NULL));
  int r = rand() % 2;
  if (r == 3)
  {
    sides[0] = 'X';
    sides[1] = 'O';
    printf("X/O\n");
  }
  else
  {
    sides[0] = 'O';
    sides[1] = 'X';
    printf("O/X\n");
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
  if ((row > 2) || (row < 0) || (col < 0) || (col > 2))
    return -1;
  int index = row * 3 + col;
  if (board[index] != '.')
    return -2;
  board[index] = player;
  condition a1 = checkStatus();
  if (a1 == unfinished)
    return 1;
  else
    return 2;
}
int getPlayerFromToken(char player, int c)
{
  for (int i = 0; i < 2; i++)
  {
    if (sides[i] == player && c == 0)
      return i;
    if (sides[i] != player && c == 1)
      return i;
  }
  return -1;
}
char *intToString(int p)
{
  int copy = p;
  int numDigits = 0;
  while (copy > 0)
  {
    copy = copy / 10;
    numDigits++;
  }
  char *number = (char *)malloc(sizeof(char) * (numDigits + 2));
  int length = numDigits + 2;
  for (int i = numDigits - 1; i >= 0; i--)
  {
    copy = p % 10;
    number[i] = copy + '0';
    p = p / 10;
  }
  number[length - 2] = '|';
  number[length - 1] = '\0';
  return number;
}
char *coordsToString(int row, int col, char player)
{
  char *letter;
  if (player == 'X')
    letter = "X|";
  else
    letter = "O|";
  char srow = row + '0';
  char scol = col + '0';
  char *coords = (char *)malloc(sizeof(char) * 5);
  coords[0] = srow;
  coords[1] = ',';
  coords[2] = scol;
  coords[3] = '|';
  coords[4] = '\0';
  char *string = combine(letter, coords);
  free(coords);
  return string;
}
char *moved(int row, int col, int player)
{
  char *one = "MOVD|16|";
  char *two = coordsToString(row, col, player);
  char *three = combine(one, two);
  free(two);
  char *four = combine(three, board);
  free(three);
  char *five = combine(four, "|");
  free(four);
  return five;
}
char *xHasY(int w, char *A, char *B, char *C)
{
  char *one = combine(usernames[w], C);
  char *two = combine(B, one);
  free(one);
  int l = strlen(two);
  char *three = intToString(l);
  char *four = combine(three, two);
  free(two);
  free(three);
  char *five = combine(A, four);
  free(four);
  return five;
}
short interpretCommand3(char *command, char *text, char player)
{
  int c, o;
  short com = 1;
  c = getPlayerFromToken(player, 0);
  o = getPlayerFromToken(player, 1);
  if (strcmp("DRAW", command) == 0)
  {
    if (strcmp("A", text) == 0)
    {
      messages[0] = "OVER|18|Draw agreed upon|";
      messages[1] = "OVER|18|Draw agreed upon|";
      active = -2;
      drawS = 0;
      com = 0;
    }
    if (strcmp("R", text) == 0)
    {
      messages[o] = "DRAW|2|R|";
      drawS = 0;
      com = 0;
    }
  }
  if (strcmp("RSGN", command) == 0)
  {
    messages[c] = "OVER|21|L|You have resigned|";
    messages[o] = xHasY(c, "OVER|", "W|", " has resigned|");
    alloc[o] = 1;
    com = 0;
    active = -2;
  }
  if (com)
  {
    messages[c] = "INVL|17|Invalid command|";
    return -1;
  }
  return 0;
}
short interpretCommand2(char *command, char *text, int num)
{
  short invl = 1;
  if (strcmp("PLAY", command) == 0)
  {
    usernames[ucount] = text;
    ucount++;
    messages[num] = "WAIT|0|";
    started++;
    active++;
    invl = 0;
  }
  else
    messages[num] = "INVL|17|Invalid command|";
  if (invl)
    return -1;
  else
    return 0;
}
short interpretCommand(char *command, char *text, char player)
{
  int c, o;
  short com = 1;
  short invl = 0;
  c = getPlayerFromToken(player, 0);
  o = getPlayerFromToken(player, 1);
  if (strcmp("MOVE", command) == 0)
  {
    if (strlen(text) < 5)
    {
      messages[c] = "INVL|20|Invalid coordinates|";
      invl = 1;
    }
    else
    {
      if(text[1] != '|' || text[3] != ',')
      {
        messages[c] = "INVL|17|Invalid command|";
        invl = 1;
      }
      else
      {
        int row = text[2] - '0';
        int col = text[4] - '0';
        int success = 0;
        if (player != text[0])
        {
          messages[c] = "INVL|12|Wrong Side|";
          invl = 1;
        }
        else
          success = makeMove(row, col, player);
        if (success == -2)
        {
          messages[c] = "INVL|23|Space already occupied|";
          invl = 1;
        }
        if (success == -1)
        {
          messages[c] = "INVL|20|Invalid coordinates|";
          invl = 1;
        }
        if (success == 1)
        {
          messages[0] = moved(row, col, player);
          messages[1] = moved(row, col, player);
          alloc[0] = 1;
          alloc[1] = 1;
        }
        if (success == 2)
        {
          messages[c] = "OVER|22|W|You have 3 in a row|";
          messages[o] = xHasY(c, "OVER|", "L|", " has 3 in a row|");
          alloc[o] = 1;
          active = -2;
        }
      }
    }
    com = 0;
  }
  if (strcmp("RSGN", command) == 0)
  {
    messages[c] = "OVER|21|L|You have resigned|";
    messages[o] = xHasY(c, "OVER|", "W|", " has resigned|");
    alloc[o] = 1;
    com = 0;
    active = -2;
  }
  if (strcmp("DRAW", command) == 0)
  {
    if (strcmp("S", text) == 0)
    {
      messages[o] = "DRAW|2|S|";
      drawS = 1;
      com = 0;
    }
  }
  if (com)
  {
    messages[c] = "INVL|17|Invalid command|";
    invl = 1;
  }
  if (invl)
    return -1;
  else
    return 1;
}
short formatMessage(char *string, int length, char player, int num2)
{
  short success = -1;
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
  if((num + i + 1) != (length - 1))
    format = 0;
  if(num > 255 || num < 0)
    format = 0;
  char *text = (char *)malloc(sizeof(char) * num);
  count = i + 1;
  for (i = count; i < length; i++)
  {
    if ((i - count) >= num)
      break;
    text[i - count] = string[i];
  }
  if (num != 0)
    text[num - 1] = '\0';
  if (!format)
  {
    messages[num2] = "INVL|16|Invalid format|";
    return -1;
  }
  if (drawS == 1)
    success = interpretCommand3(command, text, player);
  if (started == 1)
    success = interpretCommand(command, text, player);
  else
    success = interpretCommand2(command, text, num2);
  return success;
}
int readLine(int s, char player, int num)
{
  alloc[0] = 0;
  alloc[1] = 0;
  messages[0] = NULL;
  messages[1] = NULL;
  char buf[BUFSIZE];
  int bytes;
  short success;
  bytes = read(s, buf, BUFSIZE);
  success = formatMessage(buf, bytes, player, num);
  if (active == 0)
  {
    chooseSides();
    int x = getPlayerFromToken('X', 0);
    int o = getPlayerFromToken('O', 0);
    char *one, *two;
    char *temp = xHasY(o, "BEGN|", "X|", "|");
    if (messages[x] != NULL)
    {
      one = combine(messages[x], "\n");
      write(sock[x], one, strlen(one));
      printf("%s", one);
      two = combine(temp, "\n");
      sleep(0.5);
      write(sock[x], two, strlen(two));
      printf("%s", two);
      free(one);
      free(two);
    }
    else
    {
      one = combine(temp, "\n");
      write(sock[x], one, strlen(one));
      printf("%s", one);
      free(one);
    }
    free(temp);
    temp = xHasY(x, "BEGN|", "O|", "|");
    if (messages[o] != NULL)
    {
      one = combine(messages[o], "\n");
      write(sock[o], one, strlen(one));
      printf("%s", one);
      two = combine(temp, "\n");
      sleep(0.5);
      write(sock[o], two, strlen(two));
      printf("%s", two);
      free(one);
      free(two);
    }
    else
    {
      one = combine(temp, "\n");
      write(sock[o], one, strlen(one));
      printf("%s", one);
      free(one);
    }
    free(temp);
    active++;
    if (alloc[0])
      free(messages[0]);
    if (alloc[1])
      free(messages[1]);
    return 1;
  }
  if (!(messages[0] == NULL))
  {
    char *t1 = combine(messages[0], "\n");
    printf("%s", t1);
    write(sock[0], t1, strlen(t1));
    free(t1);
    if (alloc[0])
      free(messages[0]);
  }
  if (!(messages[1] == NULL))
  {
    char *t2 = combine(messages[1], "\n");
    printf("%s", t2);
    write(sock[1], t2, strlen(t2));
    free(t2);
    if (alloc[1])
      free(messages[1]);
  }
  return success;
}
void setBoard()
{
  board = (char *)malloc(sizeof(char) * 10);
  for (int i = 0; i < 9; i++)
  {
    board[i] = '.';
  }
  board[9] = '\0';
}
void playGame(int *sock)
{
  int x, o, success;
  drawS = 0;
  started = -1;
  usernames = (char **)malloc(sizeof(char *) * 2);
  messages = (char **)malloc(sizeof(char *) * 2);
  sides = (char *)malloc(sizeof(char) * 2);
  alloc = (int *)malloc(sizeof(int) * 2);
  setBoard();
  while (started != 1)
  {
    success = -1;
    while (success == -1)
      success = readLine(sock[0], 'N', 0);
    success = -1;
    while (success == -1)
      success = readLine(sock[1], 'N', 1);
  }
  x = getPlayerFromToken('X', 0);
  o = getPlayerFromToken('O', 0);
  printf("Started\n");
  while (active > 0)
  {
    success = -1;
    while (success == -1 && active > 0)
      success = readLine(sock[x], 'X', x);
    if (active < 0)
      break;
    success = -1;
    while (success == -1 && active > 0)
      success = readLine(sock[o], 'O', o);
  }
  printf("Game Over\n");
  free(usernames);
  free(messages);
  free(sides);
  free(alloc);
  free(board);
  close(sock[0]);
  close(sock[1]);
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
    playGame(sock);
    puts("Waiting for a new game...");
  }
  puts("Shutting down");
  close(listener);
  return EXIT_SUCCESS;
}
