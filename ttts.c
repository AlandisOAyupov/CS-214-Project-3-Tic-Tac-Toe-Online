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
#include <time.h>
#define QUEUE_SIZE 8
#define BUFSIZE 1054
#define HOSTSIZE 100
#define PORTSIZE 10
typedef enum condition
{
  xwin,
  owin,
  draw,
  unfinished
} condition;
typedef struct game
{
  char **residue, **usernames, **messages, *sides, *board, turn;
  int *socks, *alloc, drawS, started, active, pcount;
} game;
game **arrgames;
int storedSock, storedNum, arrSize, ucount = 0;
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
int numFields(char *string)
{
  if(strcmp(string, "PLAY") == 0)
    return 3;
  if (strcmp(string, "MOVE") == 0)
    return 4;
  if (strcmp(string, "DRAW") == 0)
    return 3;
  if (strcmp(string, "RSGN") == 0)
    return 2;
  return -1;
}
char *combine(char *first, char *second)
{
  int i = 0;
  char *combination = (char *)malloc(sizeof(char) * (strlen(first) + strlen(second) + 1));
  int length = strlen(first) + strlen(second) + 1;
  for (i = 0; i < strlen(first); i++)
    combination[i] = first[i];
  int count = 0;
  while (i < length)
  {
    combination[i] = second[count];
    i++;
    count++;
  }
  combination[length - 1] = '\0';
  return combination;
}
int containsUser(char *string)
{
  for (int i = 0; i < ucount / 2; i++)
  {
    if(arrgames[i]->usernames[0] != NULL)
      if(strcmp(arrgames[i]->usernames[0], string) == 0)
        return 1;
    if(arrgames[i]->usernames[1] != NULL)
      if(strcmp(arrgames[i]->usernames[1], string) == 0)
        return 1;
  }
  return 0;
}
void chooseSides(game *g)
{
  srand(time(NULL));
  int r = rand() % 2;
  if (r == 1)
  {
    g->sides[0] = 'X';
    g->sides[1] = 'O';
    printf("X/O\n");
  }
  else
  {
    g->sides[0] = 'O';
    g->sides[1] = 'X';
    printf("O/X\n");
  }
}
condition checkWin(char c)
{
  if (c == 'X')
    return xwin;
  else
    return owin;
}
condition checkStatus(game *g)
{
  short draw = 1;
  if ((g->board[0] == g->board[1]) && (g->board[1] == g->board[2]) && (g->board[0] != '.'))
    return checkWin(g->board[0]);
  if ((g->board[3] == g->board[4]) && (g->board[4] == g->board[5]) && (g->board[3] != '.'))
    return checkWin(g->board[3]);
  if ((g->board[6] == g->board[7]) && (g->board[7] == g->board[8]) && (g->board[6] != '.'))
    return checkWin(g->board[6]);
  if ((g->board[0] == g->board[3]) && (g->board[3] == g->board[6]) && (g->board[0] != '.'))
    return checkWin(g->board[0]);
  if ((g->board[1] == g->board[4]) && (g->board[4] == g->board[7]) && (g->board[1] != '.'))
    return checkWin(g->board[1]);
  if ((g->board[2] == g->board[5]) && (g->board[5] == g->board[8]) && (g->board[2] != '.'))
    return checkWin(g->board[2]);
  if ((g->board[0] == g->board[4]) && (g->board[4] == g->board[8]) && (g->board[0] != '.'))
    return checkWin(g->board[0]);
  if ((g->board[2] == g->board[4]) && (g->board[4] == g->board[6]) && (g->board[2] != '.'))
    return checkWin(g->board[2]);
  for (int i = 0; i < 9; i++)
    if (g->board[i] == '.')
      draw = 0;
  if (draw)
    return draw;
  return unfinished;
}
int makeMove(game *g, int row, int col, char player)
{
  row--;
  col--;
  if ((row > 2) || (row < 0) || (col < 0) || (col > 2))
    return -1;
  int index = row * 3 + col;
  if (g->board[index] != '.')
    return -2;
  g->board[index] = player;
  condition a1 = checkStatus(g);
  if (a1 == unfinished)
    return 1;
  else
    return 2;
}
int getPlayerFromToken(game *g, char player, int c)
{
  for (int i = 0; i < 2; i++)
  {
    if (g->sides[i] == player && c == 0)
      return i;
    if (g->sides[i] != player && c == 1)
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
char *moved(game *g, int row, int col, int player)
{
  char *one = "MOVD|16|";
  char *two = coordsToString(row, col, player);
  char *three = combine(one, two);
  free(two);
  char *four = combine(three, g->board);
  free(three);
  char *five = combine(four, "|");
  free(four);
  return five;
}
char *xHasY(game *g, int w, char *A, char *B, char *C)
{
  char *one = combine(g->usernames[w], C);
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
short interpretCommand2(game *g, char *command, char *text, int num)
{
  short invl = 1;
  if (strcmp("PLAY", command) == 0)
  {
    if(containsUser(text) != 1)
    {
      char *name = (char*)malloc(sizeof(char) * strlen(text)+1);
      name = strcpy(name, text);
      g->usernames[g->pcount] = name;
      g->pcount++;
      g->messages[num] = "WAIT|0|";
      g->started++;
      g->active++;
      invl = 0;
    }
    else
      g->messages[num] = "INVL|23|Username already taken|";
  }
  else
    g->messages[num] = "INVL|16|Invalid command|";
  if (invl)
    return -1;
  else
    return 0;
}
short interpretCommand(game *g, char *command, char *text, char player)
{
  printf("%c:%c\n", player, g->turn);
  int c, o;
  short com = 1;
  short invl = 0;
  c = getPlayerFromToken(g, player, 0);
  o = getPlayerFromToken(g, player, 1);
  if (strcmp("MOVE", command) == 0)
  {
    if(player == g->turn)
    {
      if (strlen(text) < 5)
      {
        g->messages[c] = "INVL|20|Invalid coordinates|";
        invl = 1;
      }
      else
      {
        if(text[1] != '|' || text[3] != ',')
        {
          g->messages[c] = "INVL|16|Invalid command|";
          invl = 1;
        }
        else
        {
          int row = text[2] - '0';
          int col = text[4] - '0';
          int success = 0;
          if (player != text[0])
          {
            g->messages[c] = "INVL|11|Wrong Side|";
            invl = 1;
          }
          else
            success = makeMove(g, row, col, player);
          if (success == -2)
          {
            g->messages[c] = "INVL|23|Space already occupied|";
            invl = 1;
          }
          if (success == -1)
          {
            g->messages[c] = "INVL|20|Invalid coordinates|";
            invl = 1;
          }
          if (success == 1)
          {
            if(g->turn == 'X')
              g->turn = 'O';
            else
              g->turn = 'X';
            g->messages[0] = moved(g, row, col, player);
            g->messages[1] = moved(g, row, col, player);
            g->alloc[0] = 1;
            g->alloc[1] = 1;
          }
          if (success == 2)
          {
            g->messages[c] = "OVER|22|W|You have 3 in a row|";
            g->messages[o] = xHasY(g, c, "OVER|", "L|", " has 3 in a row|");
            g->alloc[o] = 1;
            g->active = -3;
          }
        }
      }
    }
    else
    {
      g->messages[c] = "INVL|14|Not your turn|";
      invl = 1;
    }
    com = 0;
  }
  if (strcmp("RSGN", command) == 0)
  {
    g->messages[c] = "OVER|20|L|You have resigned|";
    g->messages[o] = xHasY(g, c, "OVER|", "W|", " has resigned|");
    g->alloc[o] = 1;
    com = 0;
    g->active = -3;
  }
  if (strcmp("DRAW", command) == 0)
  {
    if (strcmp("S", text) == 0)
    {
      if(g->drawS >= 0)
      {
        g->messages[c] = "INVL|45|Draw request already sent by opposite player|";
        com = 0;
        invl = 1;
      }
      else
      {
        g->messages[o] = "DRAW|2|S|";
        g->drawS = c;
        com = 0;
      }
    }
    if (strcmp("A", text) == 0 && c != g->drawS && g->drawS >= 0)
    {
      g->messages[0] = "OVER|19|D|Draw agreed upon|";
      g->messages[1] = "OVER|19|D|Draw agreed upon|";
      g->active = -3;
      g->drawS = -1;
      com = 0;
    }
    if (strcmp("R", text) == 0 && c != g->drawS && g->drawS >= 0)
    {
      g->messages[o] = "DRAW|2|R|";
      g->drawS = -1;
      com = 0;
    }
  }
  if (com)
  {
    g->messages[c] = "INVL|16|Invalid command|";
    invl = 1;
  }
  if (invl)
    return -1;
  else
    return 1;
}
void cpyToResidue(game *g, char *string, int length, int num)
{
  g->residue[num] = (char *)malloc(sizeof(char) * (length + 1));
  strncpy(g->residue[num], string, length);
  g->residue[num][length] = '\0';
}
int playMessages(game *g)
{
  short success;
  if (g->active == 0)
  {
    chooseSides(g);
    g->turn = 'X';
    int x = getPlayerFromToken(g, 'X', 0);
    int o = getPlayerFromToken(g, 'O', 0);
    char *one, *two;
    char *temp = xHasY(g, o, "BEGN|", "X|", "|");
    if (g->messages[x] != NULL)
    {
      one = combine(g->messages[x], "\n");
      success = write(g->socks[x], one, strlen(one));
      if(success == -1)
        return -10;
      printf("%s", one);
      two = combine(temp, "\n");
      success = write(g->socks[x], two, strlen(two));
      if(success == -1)
        return -10;
      printf("%s", two);
      free(one);
      free(two);
    }
    else
    {
      one = combine(temp, "\n");
      success = write(g->socks[x], one, strlen(one));
      if(success == -1)
        return -10;
      printf("%s", one);
      free(one);
    }
    free(temp);
    temp = xHasY(g, x, "BEGN|", "O|", "|");
    if (g->messages[o] != NULL)
    {
      one = combine(g->messages[o], "\n");
      success = write(g->socks[o], one, strlen(one));
      if(success == -1)
        return -10;
      printf("%s", one);
      two = combine(temp, "\n");
      success = write(g->socks[o], two, strlen(two));
      if(success == -1)
        return -10;
      printf("%s", two);
      free(one);
      free(two);
    }
    else
    {
      one = combine(temp, "\n");
      success = write(g->socks[o], one, strlen(one));
      if(success == -1)
        return -10;
      printf("%s", one);
      free(one);
    }
    free(temp);
    g->active++;
    if (g->alloc[0])
      free(g->messages[0]);
    if (g->alloc[1])
      free(g->messages[1]);
    return 1;
  }
  if (g->messages[0] != NULL)
  {
    char *t1 = combine(g->messages[0], "\n");
    printf("%s", t1);
    success = write(g->socks[0], t1, strlen(t1));
    if(success == -1)
      return -10;
    free(t1);
    if (g->alloc[0])
      free(g->messages[0]);
  }
  if (!(g->messages[1] == NULL))
  {
    char *t2 = combine(g->messages[1], "\n");
    printf("%s", t2);
    success = write(g->socks[1], t2, strlen(t2));
    if(success == -1)
      return -10;
    free(t2);
    if (g->alloc[1])
      free(g->messages[1]);
  }
  g->messages[0] = NULL;
  g->messages[1] = NULL;
  g->alloc[0] = 0;
  g->alloc[1] = 0;
  return success;
}
short formatMessage(game *g, char *string, int length, char player, int num2)
{
  char *word = (char *)malloc(sizeof(char) * strlen(string) + 1);
  if(g->residue[num2] != NULL)
  {
    free(word);
    word = combine(g->residue[num2], string);
    free(g->residue[num2]);
    g->residue[num2] = NULL;
  }
  else
    strcpy(word, string);
  short success = -1, format = 1;
  int i = 0, fieldsC = 0;
  char *command = (char *)malloc(sizeof(char) * 5);
  for (i = 0; i < strlen(word); i++)
  {
    if(word[i] == '|')
    {
      fieldsC++;
      break;
    }
    if(i >= 4)
    {
      format = 0;
      break;
    }
    command[i] = word[i];
  }
  command[4] = '\0';
  if (fieldsC < 1)
  {
    cpyToResidue(g, string, length, num2);
    free(command);
    return -2;
  }
  char *number = (char *)malloc(sizeof(char) * 4);
  int fields = numFields(command);
  if(fields == -1)
    format = 0;
  int count = i + 1;
  for (i = i + 1; i < strlen(word); i++)
  {
    if(word[i] == '|')
    {
      fieldsC++;
      if(fieldsC == 2)
        break;
    }
    else
    {
      if (((i - count) >= 3) || word[i] < 48 || word[i] > 57)
      {
        format = 0;
        break;
      }
      number[i - count] = word[i];
    }
  }
  number[3] = '\0';
  int num = atoi(number);
  int index = i + num;
  if (index < strlen(word))
  {
    if (word[index] != '|')
      format = 0;
  }
  else
    format = 0;
  free(number);
  if(fieldsC < 2 && fields > 0)
  {
    cpyToResidue(g, string, length, num2);
    free(command);
    return -2;
  }
  if (num > 255 || num < 0)
    format = 0;
  char *text = (char *)malloc(sizeof(char) * num);
  count = i + 1;
  for (i = i + 1; i < strlen(word); i++)
  {
    if(fieldsC >= fields)
      break;
    if (word[i] == '|')
    {
      fieldsC++;
      if(fieldsC >= fields)
        break;
    }
    if((i - count) >= num)
    {
      format = 0;
      break;
    }
    text[i - count] = word[i];
  }
  if (num != 0)
  {
    text[num - 1] = '\0';
  }
  if(fieldsC < fields && fields > 0)
  {
    cpyToResidue(g, string, length, num2);
    free(command);
    free(text);
    return -2;
  }
  if (!format)
  {
    g->messages[num2] = "INVL|14|Invalid input|";
    playMessages(g);
    return -1;
  }
  if (g->started == 1)
    success = interpretCommand(g, command, text, player);
  else
    success = interpretCommand2(g, command, text, num2);
  success = playMessages(g);
  if(success == -10)
    return -10;
  if (i + 1 < strlen(word))
  {
    char *newString = (char*)malloc(sizeof(char) * (strlen(word)-i));
    int length2 = strlen(word) - i;
    num = 0;
    for (i = i + 1; i < strlen(word); i++)
    {
      newString[num] = word[i];
      num++;
    }
    newString[length2 - 1] = '\0';
    success = formatMessage(g, newString, length2, player, num2);
  }
  free(word);
  free(command);
  free(text);
  return success;
}
int readLine(game *g, int s, char player, int num)
{
  g->alloc[0] = 0;
  g->alloc[1] = 0;
  g->messages[0] = NULL;
  g->messages[1] = NULL;
  char *buf = (char *)malloc(sizeof(char) * BUFSIZE);
  int bytes;
  short success2;
  bytes = read(s, buf, BUFSIZE);
  buf[bytes] = '\0';
  if (bytes == 0)
  {
    free(buf);
    return -10;
  }
  success2 = formatMessage(g, buf, bytes, player, num);
  free(buf);
  if (success2 == -5)
    return -5;
  return success2;
}
void setBoard(game *g)
{
  g->board = (char *)malloc(sizeof(char) * 10);
  for (int i = 0; i < 9; i++)
    g->board[i] = '.';
  g->board[9] = '\0';
}
void clearGame(game *g)
{
  free(g->board);
  if(g->residue[0] != NULL)
    free(g->residue[0]);
  if(g->residue[1] != NULL)
    free(g->residue[1]);
  g->residue[0] = NULL;
  g->residue[1] = NULL;
  free(g->usernames[0]);
  free(g->usernames[1]);
  g->usernames[0] = NULL;
  g->usernames[1] = NULL;
  g->active = -4;
  g->drawS = -1;
  g->started = -1;
  g->pcount = 0;
  g->turn = '~';
  setBoard(g);
}
int playGame(int sock, fd_set csock)
{
  game *currGame = NULL;
  int playerNum;
  int connectionCheck = 0;
  for (int i = 0; i < ucount / 2; i++)
  {
    if(sock == arrgames[i]->socks[0])
    {
      currGame = arrgames[i];
      playerNum = 0;
    }
    if(sock == arrgames[i]->socks[1])
    {
      currGame = arrgames[i];
      playerNum = 1;
    }
  }
  if(currGame != NULL)
    connectionCheck = readLine(currGame, sock, currGame->sides[playerNum], playerNum);
  if(currGame->active == -3 || connectionCheck == -10)
  {
    int sock;
    int opp = 0;
    if (playerNum == 0)
      opp = 1;
    else
      opp = 0;
    sock = currGame->socks[opp];
    if(connectionCheck == -10)
    {
      printf("OVER|31|W|Other player lost connection|\n");
      write(sock, "OVER|31|W|Other player lost connection|\n", strlen("OVER|31|W|Other player lost connection|\n"));
    }
    clearGame(currGame);
    return sock;
  }
  else
    return -1;
}
void freeGame(game *g)
{
  free(g->board);
  if(g->residue[0] != NULL)
    free(g->residue[0]);
  if(g->residue[1] != NULL)
    free(g->residue[1]);
  free(g->usernames[0]);
  free(g->usernames[1]);
  free(g->sides);
  free(g->alloc);
  free(g->socks);
  free(g);
}
void freeArr()
{
  for(int i = 0; i < ucount/2; i++)
    free(arrgames[i]);
  free(arrgames);
}
void createGame(game *g)
{
  g = (game *)malloc(sizeof(game));
  g->residue = (char **)malloc(sizeof(char *) * 2);
  g->residue[0] = NULL;
  g->residue[1] = NULL;
  g->usernames = (char **)malloc(sizeof(char *) * 2);
  g->usernames[0] = NULL;
  g->usernames[1] = NULL;
  g->messages = (char **)malloc(sizeof(char *) * 2);
  g->sides = (char *)malloc(sizeof(char) * 2);
  g->alloc = (int *)malloc(sizeof(int) * 2);
  g->socks = (int *)malloc(sizeof(int) * 2);
  g->active = -2;
  g->drawS = -1;
  g->started = -1;
  g->pcount = 0;
  g->turn = '~';
  setBoard(g);
  arrgames[ucount / 2] = g;
}
int acceptConnection(int listener)
{
  struct sockaddr_storage remote_host;
  socklen_t remote_host_len;
  remote_host_len = sizeof(remote_host);
  int sock = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
  if(sock < 0)
    perror("accept");
  short assign = 0;
  if (storedNum != -1)
  {
    printf("%d\n", storedNum);
    if(storedNum == ucount/2)
    {
      createGame(arrgames[storedNum]);
      arrgames[storedNum]->socks[0] = storedSock;
      arrgames[storedNum]->socks[1] = sock;
      ucount++;
      storedSock = -1;
      storedNum = -1;
      assign = 1;
    }
    else
    {
      arrgames[storedNum]->socks[0] = storedSock;
      arrgames[storedNum]->socks[1] = sock;
      storedSock = -1;
      storedNum = -1;
      assign = 1;
    }
  }
  if(!assign)
  {
    for (int i = 0; i < ucount / 2; i++)
    {
      if(arrgames[i]->active == -4)
      {
        storedSock = sock;
        storedNum = i;
        printf("%d\n", i);
        arrgames[i]->active = -2;
        assign = 1;
      }
    }
  }
  if(!assign)
  {
    if(ucount % 2 == 0)
    {
      int index2 = ucount / 2;
      storedSock = sock;
      storedNum = ucount / 2;
      printf("%d\n", index2);
      ucount++;
    }
  }
  if((ucount+2)/2 > arrSize)
  {
    arrSize += 2;
    arrgames = realloc(arrgames, sizeof(game*) * arrSize);
  }
  return sock;
}
int main(int argc, char **argv)
{
  arrSize = 10;
  arrgames = (game**)malloc(sizeof(game*) * arrSize);
  storedSock = -1, storedNum = -1;
  char *service = argc == 2 ? argv[1] : "14000";
  fd_set csock, rsock;
  int listener = openListen(service, QUEUE_SIZE);
  if (listener < 0)
    exit(EXIT_FAILURE);
  FD_ZERO(&csock);
  FD_SET(listener, &csock);
  puts("Listening for incoming connections");
  while (1)
  {
    rsock = csock;
    if(select(FD_SETSIZE, &rsock, NULL, NULL, NULL) < 0)
      exit(EXIT_FAILURE);
    for (int i = 0; i < FD_SETSIZE; i++)
    {
      if(FD_ISSET(i, &rsock)) 
      {
        if(i == listener)
        {
          int cs = acceptConnection(listener);
          FD_SET(cs, &csock);
        }
        else
        {
          int end = playGame(i, csock);
          if(end != -1)
          {
            FD_CLR(i, &csock);
            FD_CLR(end, &csock);
          }
        }
      }
    }
  }
  puts("Shutting down");
  close(listener);
  freeArr();
  return EXIT_SUCCESS;
}
