#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


#define UDP_BUFFER_LEN 65507


void fill_random_nums(int *nums, int n, int min, int max)
{
  srand((unsigned)(time(0)));
  for (int i = 0; i < n; i++)
  {
    nums[i] = min + rand() % (max - min + 1);
  }
}

void print_nums(int *nums, int c)
{
  for (int i = 0; i < c; i++)
  {
    printf("%d ", nums[i]);
  }
  printf("\n");
}

long timedifference(struct timeval t0, struct timeval t1)
{
  return (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_usec - t0.tv_usec);
}

int main(int argc, char *argv[])
{
  size_t max_n = UDP_BUFFER_LEN / sizeof(int);
  int n = 0, min, max;
  while (n <= 0 || n > max_n)
  {
    printf("Enter array length(<=%ld): ", max_n);
    scanf("%d", &n);
  }
  printf("Enter minimum: ");
  scanf("%d", &min);
  printf("Enter maximum: ");
  scanf("%d", &max);

  int sockfd;
  char sendline[n * sizeof(int) / sizeof(char)], recvline[n * sizeof(int) / sizeof(char)];// буфер
  struct sockaddr_in servaddr, cliaddr;//структуры для адресов сервера и клиента

  sockfd = socket(PF_INET, SOCK_DGRAM, 0);// создаём сокет

  bzero(&cliaddr, sizeof(cliaddr));//обнуляем структуру адреса клиента(есть дополнительное ненужное поле)
  cliaddr.sin_family = AF_INET;//заполняем структуру для адреса клиента
  cliaddr.sin_port = htons(0);
  cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); //настройка адреса сокета

  char *sep = strchr(argv[1], ':');//получаем порт 
  bzero(&servaddr, sizeof(servaddr));//обнуляем структуру адреса сервера(есть дополнительное ненужное поле)
  servaddr.sin_family = AF_INET;//заполняем структуру для адреса сервера
  servaddr.sin_port = htons(atoi(sep + 1));
  sep[0] = 0;
  inet_aton(argv[1], &servaddr.sin_addr);// приводим ip в нужный формат 
  fill_random_nums((int *)sendline, n, min, max);
  print_nums((int *)sendline, n);

  struct timeval start, end;
  int s = sendto(sockfd, sendline, sizeof(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));// отправляем массив(отсылаем датаграмму)
  gettimeofday(&start, 0);
  recvfrom(sockfd, recvline, sizeof(recvline), 0, (struct sockaddr *)NULL, NULL);//получаем обработанный массив
  gettimeofday(&end, 0);
  print_nums((int *)recvline, n);
  printf("Response time: %ldns\n", timedifference(start, end));
  close(sockfd);
}