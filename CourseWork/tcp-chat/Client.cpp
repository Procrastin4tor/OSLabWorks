#include "Packets.cpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <limits>
#include <signal.h>
#include <iostream>

using namespace std;
pthread_t pack_thr;
bool connected_to_serv = false;
int sock;
void* recieve_packets(void* arg);

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

bool get_host_by_ip(string ipstr, hostent*& server)
{
    in_addr ip;
    hostent *hp;
    if (!inet_aton(ipstr.c_str(), &ip))
    { 
        printf("Can't parse IP address!\n");
        return false;
    }
    if ((hp = gethostbyaddr((const void*)&ip, sizeof(ip), AF_INET)) == NULL)
    {
        printf("Не найден сервер по ip %s\n", ipstr.c_str());
        return false;
    } 
    server = hp;
    return true;
}

bool split_adress(string adress, in_port_t& port, string& ip_str)
{
    size_t split_index = adress.find(':');
    if (split_index == string::npos) return false;

    ip_str = string(adress.begin(), adress.begin() + split_index);
    string port_str(adress.begin() + split_index + 1, adress.end());
    port = stoi(port_str);
    return true;
}

bool server_connect(in_port_t port, int sockfd, hostent* server)
{
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) return false;
    return true;
}

void destroy_connection()
{
    char p = 'p';
    SendPacket(&p, sock, sizeof(char), PacketType::Disconnection);
    close(sock);
    connected_to_serv = false;
    printf("Соединение разорвано!\n");
}

void notify_and_exit(int signum)
{
    destroy_connection();
    exit(0);
}

void handle_server_disconnection()
{
    connected_to_serv = false;
    destroy_connection();
}

void start_chat()
{
    printf("Успешное подключение к чату! Введите ваш никнейм:");
    char nickname[64];
    scanf("%s", nickname);
    cin.clear();
    cin.sync();

    SendPacket(nickname, sock, strlen(nickname) * sizeof(char), PacketType::Connection);

    pthread_create(&pack_thr, 0, recieve_packets, 0);
}

void* recieve_packets(void* arg)
{
    while (true)
    {
        bool status;
        PacketType pack_type;
        size_t pack_size;
        
        char* packet = (char*)RecvPacket(status, sock, pack_size, pack_type);
        if (!status)
        {
            handle_server_disconnection();
            pthread_exit(0);
        }

        switch (pack_type)
        {
        case PacketType::Disconnection:
            handle_server_disconnection();
            break;

        case PacketType::ToPrint:
            printf("%s\n", packet);
            break;
        }
    }
}

void* handle_cmds(void* arg)
{
    cin.clear();
    cin.sync();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    string command;
    while (true)
    {
        std::getline(std::cin, command);
        if (command == "") continue;
        if (command[0] == '/')
        {
            if (command.find("/tell") != string::npos)
            {
                vector<string> strings;
                char delim = ' ';
                split(command, strings, delim);
                string actual_message = "";
                for (int i = 2; i < strings.size(); i++)
                {
                    actual_message += strings[i] + ' ';
                }
                
                char send_buf[strings[1].size() + 1 + actual_message.size()]; // name + message
                sprintf(send_buf, "%s %s", strings[1].c_str(), actual_message.c_str());
                SendPacket(send_buf, sock, strlen(send_buf) * sizeof(char), PacketType::PrivateMsg);
            }
            else if (command.find("/connect") != string::npos)
            {
                if (connected_to_serv)
                {
                    printf("Вы уже подключены к серверу!\n");
                    continue;
                }
                vector<string> strings;
                split(command, strings, ' ');
                
                if (strings.size() != 2)
                {
                    printf("Ошибка команды!\n");
                    continue;
                }

                sock = create_socket();

                in_port_t port;
                string ip_str;
                split_adress(strings[1], port, ip_str);

                hostent* server;
                if (get_host_by_ip(ip_str, server))
                {
                    if (server_connect(port, sock, server))
                    {
                        connected_to_serv = true;
                        start_chat();
                    }
                    else
                    {
                        printf("Не удалось подключиться к серверу! Попробуйте другой адрес, используя /connect *адрес*\n");
                    }
                }
            }
            else if (command.find("/disconnect") != string::npos)
            {
                if (!connected_to_serv)
                {
                    printf("Вы и так не подключены к серверу!\n");
                }
                else
                {
                    destroy_connection();
                }
                
            }
            else if (command.find("/help") != string::npos)
            {
                printf("\n/tell *Имя пользователя* - Отправить пользователю приватное сообщение\n/connect *Адрес сервера* - подключиться к серверу\n/disconnect - отключиться от сервера\n/help - показать помощь\n\n");
            }
            else
            {
                printf("Команда не найдена! Используйте /help для вывода помощи.\n");
            }
        }
        else
        {
            if (!connected_to_serv)
            {
                printf("Сервер отключён! Ваши сообщение не отправлено. Команды работают.\n");
                continue;
            }
            char msg[command.size()];
            strcpy(msg, command.c_str());

            SendPacket(msg, sock, strlen(msg) * sizeof(char), PacketType::ChatMessage);
        }
    }
    
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");

    if (argc >= 2)
    {
        string adress = argv[1];
        in_port_t port; 
        string ip_str;
        if (!split_adress(adress, port, ip_str))
        {
            printf("Некорректный формат адреса! Используйте адрес:порт!\n");
        }
        else
        {
            sock = create_socket();

            hostent* server;
            if (get_host_by_ip(ip_str, server))
            {
                if (server_connect(port, sock, server))
                {
                    connected_to_serv = true;
                    start_chat();
                }
                else
                {
                    printf("Не удалось подключиться к серверу! Попробуйте другой адрес, используя /connect *адрес*\n");
                    close(sock);
                }
            }
        }
    }

    struct sigaction sa;
    sa.sa_handler = notify_and_exit;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pthread_t msg_thr;
    pthread_create(&msg_thr, 0, handle_cmds, 0);
    
    while (true);
}