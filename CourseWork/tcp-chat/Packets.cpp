#include <cstdlib>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <vector>

const std::size_t info_bytes = 8;

enum PacketType
{
    Disconnection = -1, ToPrint = 0, Connection = 1, ChatMessage = 2, PrivateMsg = 3
};

void error(std::string msg)
{
    perror(msg.c_str());
    exit(1);
}

int countDigit(long long n)
{
    int count = 0;
    while (n != 0)
    {
        n = n / 10;
        ++count;
    }
    return count;
}

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}

std::size_t ConstructInfoPacket(void* packet, std::size_t packet_size, PacketType type)
{
    char* p = (char*)packet;
    p[0] = (char)type;
    sprintf(p + 1, "%zu", packet_size);
    int index = countDigit(packet_size) + 1;
    p[index] = '|';
    return packet_size + info_bytes;
}

// Returns parsed packet (without info bytes)
std::size_t ParseInfoPacket(void* packet, PacketType& outType)
{
    char* ch_packet = (char*)packet;
    outType = (PacketType)ch_packet[0];
    std::string sizebuf;
    for (int i = 1; i <= 7; i++)
    {
        if (ch_packet[i] == '|') break;
        sizebuf += ch_packet[i];
    }

    size_t psize = std::stoi(sizebuf);
    return psize;
}

bool SendPacket(void* packet, int socket, std::size_t packet_size, PacketType type)
{
    char* info_packet = new char[info_bytes];
    ConstructInfoPacket(info_packet, packet_size, type); 
    
    if (send(socket, info_packet, info_bytes, MSG_NOSIGNAL) <= 0) return false;
    if (send(socket, packet, packet_size, MSG_NOSIGNAL) <= 0) return false;
    return true;
}

void* RecvPacket(bool& result, int socket, std::size_t& outSize, PacketType& outType)
{
    char* info_pack = new char[info_bytes];
    if (recv(socket, info_pack, info_bytes, 0) <= 0)
    {
        result = false;
        return nullptr;
    }
    size_t packsize = ParseInfoPacket(info_pack, outType);
    char* packet = new char[packsize];
    if (recv(socket, packet, packsize, 0) <= 0)
    {
        result = false;
        return nullptr;
    }
    
    result = true;
    return packet;
}
