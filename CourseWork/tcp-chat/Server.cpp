#include "Packets.cpp"
#include <vector>
#include <bits/stdc++.h>
#include <signal.h>

#define MAX_USERS 5

using namespace std;
int sockfd;

struct User
{
    int sockfd;
    string nickname;

    User(int sock) : sockfd(sock) {};
};

vector<User*> users;

vector<User*> send_all(void* packet, size_t packet_size, PacketType type, int except_fd = -1);

int create_socket_serv()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

sockaddr_in create_connection_server(in_port_t port, int sockfd)
{
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error on binding!");
    return serv_addr;
}

void start_listening(int sockfd)
{
    if (listen(sockfd, MAX_USERS) < 0) error("Error on listening start");
}

void notify_and_exit(int signal)
{
    char p = 'p';
    send_all(&p, sizeof(char), PacketType::Disconnection);
    exit(0);
}

void handle_client_disconnection(User disconnected)
{
    string message = disconnected.nickname + " отключился!"; 
    char msg[message.size()];
    strcpy(msg, message.c_str());
    printf("%s\n", msg);
    
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i]->sockfd == disconnected.sockfd)
            users.erase(users.begin() + i);
    }

    auto fail_users = send_all(msg, message.size() * sizeof(char), PacketType::ToPrint);
}

void handle_connection(User connected)
{
    string message = connected.nickname + " подключился!";
    char msg[message.size()];
    strcpy(msg, message.c_str());
    printf("%s\n", msg);

    auto fail_users = send_all(msg, message.size() * sizeof(char), PacketType::ToPrint, connected.sockfd);
    for (User* user : fail_users)
        handle_client_disconnection(*user);
}

void handle_chatmessage(User messaged, char* chat_msg)
{
    string message = messaged.nickname + ": " + chat_msg;
    char msg[message.size()];
    strcpy(msg, message.c_str());
    printf("%s\n", msg);

    auto fail_users = send_all(msg, message.size() * sizeof(char), PacketType::ToPrint, messaged.sockfd);
    for (User* user : fail_users)
        handle_client_disconnection(*user);
}

void handle_privatemessage(User messaged, char* chat_message)
{
    string message = chat_message;
    vector<string> strings;
    char delim = ' ';
    split(message, strings, delim);
    
    string name = strings[0];
    User* sendTo = new User(0);
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i]->nickname == name)
        {
            sendTo = users[i];
        }
    }
    if (sendTo->nickname == "") return; // not found

    string actual_message(message.begin() + name.size() + 1, message.end());
    char msg[14 + messaged.nickname.size() + actual_message.size()];
    sprintf(msg, "[Приватное] %s: %s", messaged.nickname.c_str(), actual_message.c_str());
    printf("%s\n", msg);

    SendPacket(msg, sendTo->sockfd, strlen(msg) * sizeof(char), PacketType::ToPrint);
}

vector<User*> send_all(void* packet, size_t packet_size, PacketType type, int except_fd)
{
    vector<User*> fail_users;
    for (User* user : users)
    {
        if (user->sockfd == except_fd) continue;
        if (!SendPacket(packet, user->sockfd, packet_size, type)) fail_users.push_back(user);
    }
    return fail_users;
}

void* recieve_packets(void* arg)
{
    User* user = (User*)arg;
    int sock = user->sockfd;
    while (true)
    {
        bool status;
        PacketType pack_type;
        size_t pack_size;
        
        char* packet = (char*)RecvPacket(status, sock, pack_size, pack_type);
        if (!status)
        {
            handle_client_disconnection(*user);
            pthread_exit(0);
        }

        switch (pack_type)
        {
        case PacketType::Connection:
            user->nickname = packet;
            handle_connection(*user);
            break;
        
        case PacketType::Disconnection:
            handle_client_disconnection(*user);
            pthread_exit(0);
            break;

        case PacketType::ChatMessage:
            handle_chatmessage(*user, packet);
            break;

        case PacketType::PrivateMsg:
            handle_privatemessage(*user, packet);
            break;

        case PacketType::ToPrint:
            break;

        }
    }
}

void* recieve_users(void*)
{
    sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    while (true)
    {
        while (users.size() >= MAX_USERS);
        int clientsock = accept(sockfd, (sockaddr*)&cli_addr, &clilen); // Принимаем клиента
        if (clientsock < 0) error("Error on accept\n"); // Если ошибка
        
        users.push_back(new User(clientsock)); // Заносим клиента в вектор connecter_clients
        pthread_t thr;
        pthread_create(&thr, 0, recieve_packets, (void*)users.back());
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        error("You must provide server port!\n");
    }
    in_port_t port = atoi(argv[1]);

    sockfd = create_socket_serv();
    create_connection_server(port, sockfd);
    start_listening(sockfd);

    printf("Server started on %i port\n", port);

    struct sigaction sa;
    sa.sa_handler = notify_and_exit;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pthread_t recv_thr;
    pthread_create(&recv_thr, 0, recieve_users, 0);

    while (true);
}