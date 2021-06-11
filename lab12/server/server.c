#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_FILENAME_SIZE 256

long fsize(FILE *fp)// определяем размер файла
{
  long prev = ftell(fp);
  fseek(fp, 0L, SEEK_END);
  long sz = ftell(fp);
  fseek(fp, prev, SEEK_SET);
  return sz;
}

int main(int argc, char *argv[]) 
{
  int sockfd; 
  struct sockaddr_in servaddr; //структура данных адреса сервера

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;//заполнение структуры адреса сервера
  servaddr.sin_port = htons(atoi(argv[1]));
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)//настройка адреса сокета
  {
    servaddr.sin_port = 0;
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
      perror(NULL);
      close(sockfd);
      exit(1);
    }
  }
  socklen_t servlen = sizeof(servaddr); //размер структуры адреса сервера
  listen(sockfd, 5); // установление связи через виртуальное соединение,превод сокета в пассивный режим, установление глубины очереди для соединений
  getsockname(sockfd, (struct sockaddr *)&servaddr, &servlen); // получаем имя сокета
  printf("Listening on port: %d\n", ntohs(servaddr.sin_port));

  if (fork() == 0) // создаем новый процесс(обрабатываем запросы клиента)
  {
    while (1)
    {
      struct sockaddr_in cliaddr;
      socklen_t clilen = sizeof(cliaddr);
      int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen); //ожидаем нового клиента (получаем информацию о полностью устанволенных соединениях)
      if (fork() == 0) // создаем новый процесс
        continue;
      while (1) // обрабатываем запросы нового пользователя
      {
        char filename[MAX_FILENAME_SIZE];
        int n = read(newsockfd, filename, MAX_FILENAME_SIZE);// читаем из сокета имя файла
        {
          close(newsockfd);
          exit(0);
        }
        FILE *fin = fopen(filename, "r");
        if (fin == NULL)
        {
          long statusmsg = -1;
          write(newsockfd, &statusmsg, sizeof(statusmsg));
        }
        else
        {
          long filesize = fsize(fin);
          write(newsockfd, &filesize, sizeof(filesize)); 

          char msg[filesize];
          fread(msg, sizeof(char), filesize, fin);
          fclose(fin);
          write(newsockfd, msg, sizeof(msg));
        }
      }
    }
  }
  else
  {
    printf("Ready to recive commands\n");
    char command[MAX_FILENAME_SIZE];
    while (1)
    {
      scanf("%s", command);

      if (strcmp(command, "exit") == 0) 
      {
        exit(0);
      }
      else if (strcmp(command, "help") == 0)
      {
        printf("Avalible commands:\n");
        printf("exit - closes app\n");
        printf("help - shows avalible commands\n");
      }
      else
      {
        printf("Unknown command, please use help to get list of avalible commands\n");
      }
    }
  }
}