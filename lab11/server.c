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

#ifdef __linux__
#define UDP_BUFFER_LEN 65507
#endif

int compare_ints(const void *a, const void *b) 
{
  return (*((int *)b) - *((int *)a));
}

int main(int argc, char *argv[])
{
  size_t maxlen = UDP_BUFFER_LEN;
  int sockfd;
  char line[maxlen];
  struct sockaddr_in servaddr, cliaddr; //структуры для адресов сервера и клиента

  bzero(&servaddr, sizeof(servaddr)); //обнуляем структуру адреса сервера(есть дополнительное ненужное поле)
  servaddr.sin_family = AF_INET;//заполняем структуру для адреса сервера
  servaddr.sin_port = htons(atoi(argv[1])); 
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //привязываем адрес к вычислтиельной системе в целом

  sockfd = socket(PF_INET, SOCK_DGRAM, 0); //создаём сокет
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) //настройка адреса сокета
  {
    servaddr.sin_port = 0;
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
      perror(NULL);
      close(sockfd); 
      exit(1);
    }
  }
  socklen_t servlen = sizeof(servaddr); // определяем размер переменной servaddr
  getsockname(sockfd, (struct sockaddr *)&servaddr, &servlen);//получаем имя 
  printf("Listening on port: %d\n", ntohs(servaddr.sin_port));//выводим порт сервера

  while (1)
  {
    socklen_t clilen = sizeof(cliaddr);
    int n = recvfrom(sockfd, line, maxlen, 0, (struct sockaddr *)&cliaddr, &clilen);//получаем сообщение из соккета и записываем в буфер line
    qsort(line, n * sizeof(char) / sizeof(int), sizeof(int), compare_ints);
    sendto(sockfd, line, n, 0, (struct sockaddr *)&cliaddr, clilen);//отправляем датаграмму
  }
}